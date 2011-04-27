#include "maxent.hh"
#include "vec.hh"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <ios>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <set>
#include <unordered_set>

#include <lbfgs.h>

using namespace std;

#define DEBUG 1

struct _pass_data
{
	_pass_data(double lambda,
		   size_t label_count,
		   size_t feat_count,
		   const DataSet &data) :
		lambda(lambda),
		label_count(label_count),
		feat_count(feat_count),
		best_dev_correst(0),
		best_x(label_count * feat_count),
		data(data) {
	}

	double lambda;
	size_t label_count;
	size_t feat_count;
	size_t best_dev_correst;
	vec_t best_x;
	const DataSet &data;
};

static double _evaluate(void *instance, // user data
			const double *x, // current values of variables
			double *g, // put gradient vector here
			const int n, // number of variables
			const double step) // current step of line search
{
	const _pass_data &td = *static_cast<_pass_data *>(instance);
	double lambda = td.lambda;
	size_t label_count = td.label_count;
	size_t feat_count = td.feat_count;
	const DataSet &data = td.data;

	// initialize with L2 regularization terms
	double f = 0.5 * lambda * vv_dot_prod(x, x + n, x);
	vv_mul_by(g, x, x + n, lambda);
	// per-example sum
	for (auto i = data.train.begin(); i != data.train.end(); ++i) {
		// per-label score (w . phi)
		vec_t score_per_label(label_count);
		// TODO: use transform when g++ supports lambda
		for (size_t j = 0; j < label_count; ++j)
			score_per_label[j] = sv_dot_prod(i->sparse.begin(),
							 i->sparse.end(),
							 x + j * feat_count);
		// update ref-label terms
		size_t ref_label = i->label;
		f -= score_per_label[ref_label];
		vs_sub_eq(g + ref_label * feat_count,
			  i->sparse.begin(),
			  i->sparse.end());

		// update Z (partition function) terms
		for (size_t j = 0; j < label_count; ++j)
			score_per_label[j] = exp(score_per_label[j]);
		double Z = accumulate(score_per_label.begin(),
				      score_per_label.end(),
				      0.0);
		f += log(Z);
		for (size_t j = 0; j < label_count; ++j)
			vs_add_eq_mul_by(g + j * feat_count,
					 i->sparse.begin(),
					 i->sparse.end(),
					 score_per_label[j] / Z);
	}

	return f;
}

static size_t _test(const vector<DataPoint> &points, const double *x,
		    size_t feat_count, size_t label_count)
{
	size_t correct = 0;
	vec_t scores(label_count);
	for (auto i = points.begin(); i != points.end(); ++i) {
		for (size_t j = 0; j != label_count; ++j)
			scores[j] = sv_dot_prod(i->sparse.begin(), i->sparse.end(),
						x + j * feat_count);
		size_t best_label = max_element(scores.begin(), scores.end()) - scores.begin();
		correct += best_label == i->label;
	}
	return correct;
}

static int _progress(void *instance, // same as _evaluate
		     const double *x, // same as _evaluate
		     const double *g, // same as _evaluate
		     const double fx, // objective
		     const double xnorm, // norm of variables
		     const double gnorm, // norm of gradient
		     const double step, // line search step
		     int n, // number of variables
		     int k, // iteration count
		     int ls) // number of evaluations called
{
        cerr << "it " << k
	     << "\tfx=" << fx
	     << "\txnorm=" << xnorm
	     << "\tgnorm=" << gnorm;

	_pass_data &td = *static_cast<_pass_data *>(instance);
	size_t label_count = td.label_count;
	size_t feat_count = td.feat_count;
	const DataSet &data = td.data;

// #if DEBUG
// 	double train_correct = _test(data.train, x, feat_count, label_count);
// 	cerr << "\ttr_err=" << (data.train.size() - train_correct) / data.train.size();
// #endif

	if (data.dev.size()) {
		double dev_correct = _test(data.dev, x, feat_count, label_count);
		cerr << "\tde_err=" << (data.dev.size() - dev_correct) / data.dev.size();
		if (dev_correct > td.best_dev_correst) {
			td.best_dev_correst = dev_correct;
			copy(x, x + n, td.best_x.begin());
			cerr << " [best]";
		}
	}

// #if DEBUG
// 	if (data.test.size()) {
// 		double test_correct = _test(data.test, x, feat_count, label_count);
// 		cerr << "\tte_err=" << (data.test.size() - test_correct) / data.test.size();
// 	}
// #endif

	cerr << endl;

// #if DEBUG
// 	if (n < 100) {
// 		cerr << "x = " << endl;
// 		for (size_t i = 0; i < label_count; ++i) {
// 			cerr << '\t';
// 			for (size_t j = 0; j < feat_count; ++j)
// 				cerr << (j == 0 ? "" : " ") << x[i * feat_count + j];
// 			cerr << endl;
// 		}
// 		cerr << "g = " << endl;
// 		for (size_t i = 0; i < label_count; ++i) {
// 			cerr << '\t';
// 			for (size_t j = 0; j < feat_count; ++j)
// 				cerr << (j == 0 ? "" : " ") << g[i * feat_count + j];
// 			cerr << endl;
// 		}
// 	}
// #endif

	return 0;
}


void MaxEntModel::load_line(const string &line, size_t lineno)
{
	istringstream strin(line);

	string feat_name;
	strin >> feat_name;

	if (feat_id_map.count(feat_name))
		throw runtime_error("Duplicate feature at line " +
				    to_string(static_cast<long long>(lineno)) +
				    ": " + feat_name);
	// assign feat_id
	size_t feat_id = feat_id_map.size();
	feat_id_map[feat_name] = feat_id;
	// read weights
	for (size_t i = 0; i != label_count; ++i)
		strin >> weights[i * feat_count + feat_id];
}

static inline size_t _num_of_cols(const string &line)
{
	istringstream strin(line);
	size_t ret = 0;
	string buf;
	while (strin >> buf)
		++ret;
	return ret;
}

void MaxEntModel::load_model(istream &input)
{
	string line;

	// first pass: obtain `label_count` and `feat_count`
	getline(input, line);
	label_count = _num_of_cols(line) - 1;
	feat_count = 1;
	while (getline(input, line))
		++feat_count;
	weights.resize(label_count * feat_count);

	// second pass: read weights and build feat_id_map
	input.clear();
	input.seekg(0);
	feat_id_map.clear();
	size_t lineno = 1;
	while (getline(input, line))
		load_line(line, lineno++);
}

void MaxEntModel::dump_model(ostream &output)
{
	// always output bias first
	output << "**BIAS**";
	size_t bias_feat_id = feat_id_map["**BIAS**"];
	for (size_t i = 0; i != label_count; ++i)
		output << (i == 0 ? '\t' : ' ')
		       << setiosflags(ios_base::fixed) << setprecision(20)
		       << weights[i * feat_count + bias_feat_id];
	output << '\n';
	// temporarily remove bias from feat_id_map
	feat_id_map.erase("**BIAS**");
	// other features
	for (auto i = feat_id_map.begin(); i != feat_id_map.end(); ++i) {
		output << i->first;
		for (size_t j = 0; j != label_count; ++j)
			output << (j == 0 ? '\t' : ' ')
			       << setiosflags(ios_base::fixed) << setprecision(20)
			       << weights[j * feat_count + i->second];
		output << '\n';
	}
	// restore bias to feat_id_map
	feat_id_map["**BIAS**"] = bias_feat_id;
}

void MaxEntModel::train(const DataSet &data, double lambda)
{
	label_count = data.label_count;
	feat_count = data.feat_count;
	feat_id_map = data.feat_id_map;

	// compute parameter dimension
	size_t n = label_count * feat_count;
	// parameter stoarge during optimization
	double *x = lbfgs_malloc(n);
	// information needed for _evaluate
	_pass_data td(lambda, label_count, feat_count, data);

	// TODO: user-supplied optimization parameters
	// actual optimization
	int ret = lbfgs(n, // num of vars
			x, // storage of vars
			NULL, // returned objective value; not needed
			_evaluate, // evaluating objective and gradient
			_progress, // progress report
			&td, // pass data
			NULL); // lbfgs parameters; NULL for default
	if (ret)
		cerr << "Warning: lbfgs terminated with error code " << ret << endl;
	// store weights
	weights.resize(n);
	if (data.dev.size() && td.best_dev_correst) {
		cerr << "using best weights on dev (de_err="
		     << 1 - static_cast<double>(td.best_dev_correst) / data.dev.size()
		     << ")"
		     << endl;
		copy(td.best_x.begin(), td.best_x.end(), weights.begin());
	}
	else
		copy(x, x + n, weights.begin());
	// free temporary parameter stoarge
	lbfgs_free(x);
}


size_t MaxEntModel::predict(const sparse_t &sparse, vec_t &probs) const
{
	compute_scores(sparse, probs);

	for (auto i = probs.begin(); i != probs.end(); ++i)
		*i = exp(*i);

	double sum = accumulate(probs.begin(), probs.end(), 0.0);
	for (auto i = probs.begin(); i != probs.end(); ++i)
		*i /= sum;

	size_t best_label = max_element(probs.begin(), probs.end()) - probs.begin();

	return best_label;
}

void MaxEntModel::compute_scores(const sparse_t &sparse, vec_t &result) const
{
	// initialize result to 0
	result.clear();
	result.resize(label_count);

	for (size_t i = 0; i != label_count; ++i)
		result[i] = sv_dot_prod(sparse.begin(), sparse.end(),
					weights.begin() + i * feat_count);
}

sparse_t MaxEntModel::build_sparse(istream &input, bool binary) const
{
	if (binary)
		return sparse_from_binary(input, feat_id_map);
	else
		return sparse_from_real(input, feat_id_map);
}

size_t MaxEntModel::test(const vector<DataPoint> &points) const
{
	size_t correct = 0;
	vec_t probs;
	for (auto i = points.begin(); i != points.end(); ++i)
		correct += (predict(i->sparse, probs) == i->label);
	return correct;
}
