#include "latent_log_linear.hh"
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
	_pass_data(const LLLMStruct &st,
		   double lambda,
		   size_t label_count,
		   size_t feat_count,
		   size_t latent_count,
		   const DataSet &data) :
		st(st),
		lambda(lambda),
		label_count(label_count),
		feat_count(feat_count),
		latent_count(latent_count),
		best_dev_correst(0),
		best_x(label_count * feat_count),
		data(data) {
	}

	const LLLMStruct &st;

	double lambda;

	size_t label_count;
	size_t feat_count;
	size_t latent_count;

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
	size_t latent_count = td.latent_count;
	size_t width = latent_count * feat_count;
	// n = latent_count * feat_count * label_count
	const DataSet &data = td.data;

	// initialize with L2 regularization terms
	double f = 0.5 * lambda * vv_dot_prod(x, x + n, x);
	vv_mul_by(g, x, x + n, lambda);
	// per-example sum
	for (auto i = data.train.begin(); i != data.train.end(); ++i) {
		// per-label score \sum_h exp(psi(x, y, h))

		// exp(psi(x, y, h))
		vec_t exp_psi(label_count * latent_count);
		for (size_t j = 0; j < label_count; ++j) {
			for (size_t h = 0; h < latent_count; ++h) {
				double psi = sv_dot_prod(i->sparse.begin(),
							 i->sparse.end(),
							 x + j * width + h * feat_count);
				exp_psi[j * latent_count + h] = exp(psi);
			}
		}
		// update ref-label terms
		size_t ref_label = i->label;
		// -log(\sum_h exp(psi(x, y, h)))
		double sum_exp_psi_ref = accumulate(exp_psi.begin() + ref_label * latent_count,
						    exp_psi.begin() + (ref_label + 1) * latent_count,
						    0.0);
		f -= log(sum_exp_psi_ref);

		// for each h, - exp_psi(x,y,h)/sum_exp_psi_ref * \nabla psi(x,y,h)
		for (size_t h = 0; h < latent_count; ++h)
			vs_sub_eq_mul_by(g + ref_label * width + h * feat_count,
					 i->sparse.begin(),
					 i->sparse.end(),
					 exp_psi[ref_label * latent_count + h] / sum_exp_psi_ref);

		// update Z (partition function) terms
		// Z = \sum_{y,h} exp(psi(x,y,h))
		double Z = accumulate(exp_psi.begin(),
				      exp_psi.end(),
				      0.0);
		// +log Z
		f += log(Z);
		// for each (y,h), + exp_psi(x,y,h)/Z * \nabla psi(x,y,h)
		for (size_t j = 0; j < label_count; ++j)
			for (size_t h = 0; h < latent_count; ++h)
				vs_add_eq_mul_by(g + j * width + h * feat_count,
						 i->sparse.begin(),
						 i->sparse.end(),
						 exp_psi[j * latent_count + h] / Z);
	}

	return f;
}

static size_t _test(const vector<DataPoint> &points, const double *x,
		    size_t feat_count, size_t label_count, size_t latent_count)
{
	size_t correct = 0;
	size_t width = feat_count * latent_count;
	vec_t scores(label_count);
	for (auto i = points.begin(); i != points.end(); ++i) {
		for (size_t j = 0; j != label_count; ++j) {
			scores[j] = 0;
			for (size_t h = 0; h != latent_count; ++h) {
				double psi = sv_dot_prod(i->sparse.begin(), i->sparse.end(),
							 x + j * width + h * feat_count);
				scores[j] += exp(psi);
			}
		}
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
	size_t latent_count = td.latent_count;
	const DataSet &data = td.data;

// #if DEBUG
// 	double train_correct = _test(data.train, x, feat_count, label_count, latent_count);
// 	cerr << "\ttr_err=" << (data.train.size() - train_correct) / data.train.size();
// #endif

	if (data.dev.size()) {
		double dev_correct = _test(data.dev, x, feat_count, label_count, latent_count);
		cerr << "\tde_err=" << (data.dev.size() - dev_correct) / data.dev.size();
		if (dev_correct > td.best_dev_correst) {
			td.best_dev_correst = dev_correct;
			copy(x, x + n, td.best_x.begin());
			cerr << " [best]";
		}
	}

// #if DEBUG
// 	if (data.test.size()) {
// 		double test_correct = _test(data.test, x, feat_count, label_count, latent_count);
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


void LatentLogLinearModel::load_line(const string &line, size_t h, size_t lineno)
{
	istringstream strin(line);

	string feat_name;
	strin >> feat_name;

	// if (feat_id_map.count(feat_name))
	// 	throw runtime_error("Duplicate feature at line " +
	// 			    to_string(static_cast<long long>(lineno)) +
	// 			    ": " + feat_name);
	// assign feat_id
	size_t feat_id;
	if (feat_id_map.count(feat_name) == 0) {
		feat_id = feat_id_map.size();
		feat_id_map[feat_name] = feat_id;
	} else
		feat_id = feat_id_map[feat_name];
	// read weights
	for (size_t i = 0; i != label_count; ++i)
		strin >> weights[i * feat_count * latent_count + h * feat_count + feat_id];
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

void LatentLogLinearModel::load_model(istream &input)
{
	string line;
	getline(input, line);
	try {
		latent_count = stoi(line);
	} catch (invalid_argument &e) {
		cerr << "Error: first line of the model is not latent_count" << endl;
		exit(1);
	}

	streampos begin = input.tellg();

	// first pass: obtain `label_count` and `feat_count`
	getline(input, line);
	label_count = _num_of_cols(line) - 1;
	feat_count = 1;
	while (getline(input, line))
		++feat_count;
	if (feat_count % latent_count != 0)
		throw runtime_error("Wrong number of features");
	feat_count /= latent_count;
	weights.resize(label_count * feat_count * latent_count);

	// second pass: read weights and build feat_id_map
	input.clear();
	input.seekg(begin);
	feat_id_map.clear();
	size_t lineno = 2;
	for (size_t i = 0; i != latent_count; ++i) {
		for (size_t j = 0; j != feat_count; ++j) {
			getline(input, line);
			load_line(line, i, lineno++);
		}
	}
}

void LatentLogLinearModel::dump_model(ostream &output)
{
	size_t width = latent_count * feat_count;
	output << latent_count << '\n';
	for (size_t h = 0; h != latent_count; ++h) {
		// always output bias first
		output << "**BIAS**";
		size_t bias_feat_id = feat_id_map["**BIAS**"];
		for (size_t i = 0; i != label_count; ++i)
			output << (i == 0 ? '\t' : ' ')
			       << setiosflags(ios_base::fixed) << setprecision(20)
			       << weights[i * width + h * feat_count + bias_feat_id];
		output << '\n';
		// temporarily remove bias from feat_id_map
		feat_id_map.erase("**BIAS**");
		// other features
		for (auto i = feat_id_map.begin(); i != feat_id_map.end(); ++i) {
			output << i->first;
			for (size_t j = 0; j != label_count; ++j)
				output << (j == 0 ? '\t' : ' ')
				       << setiosflags(ios_base::fixed) << setprecision(20)
				       << weights[j * width + h * feat_count + i->second];
			output << '\n';
		}
		// restore bias to feat_id_map
		feat_id_map["**BIAS**"] = bias_feat_id;
	}
}

void LatentLogLinearModel::train(const DataSet &data, const LLLMStruct &st,
				 size_t latent_count, double lambda)
{
	label_count = data.label_count;
	feat_count = data.feat_count;
	this->latent_count = latent_count;
	this->st = st;
	feat_id_map = data.feat_id_map;

	// compute parameter dimension
	size_t n = label_count * feat_count * latent_count;

	// parameter stoarge during optimization
	double *x = lbfgs_malloc(n);
	// information needed for _evaluate
	_pass_data td(st, lambda, label_count, feat_count, latent_count, data);

	// TODO: user-supplied optimization parameters
	lbfgs_parameter_t params;
	lbfgs_parameter_init(&params);
	// actual optimization
	int ret = lbfgs(n, // num of vars
			x, // storage of vars
			NULL, // returned objective value; not needed
			_evaluate, // evaluating objective and gradient
			_progress, // progress report
			&td, // pass data
			&params); // lbfgs parameters; NULL for default
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


size_t LatentLogLinearModel::predict(const sparse_t &sparse, vec_t &probs) const
{
	compute_scores(sparse, probs);

	double sum = accumulate(probs.begin(), probs.end(), 0.0);
	for (auto i = probs.begin(); i != probs.end(); ++i)
		*i /= sum;

	size_t best_label = max_element(probs.begin(), probs.end()) - probs.begin();

	return best_label;
}

void LatentLogLinearModel::compute_scores(const sparse_t &sparse, vec_t &result) const
{
	// initialize result to 0
	result.clear();
	result.resize(label_count);

	size_t width = feat_count * latent_count;

	for (size_t i = 0; i != label_count; ++i) {
		result[i] = 0;
		for (size_t j = 0; j != latent_count; ++j) {
			double psi = sv_dot_prod(sparse.begin(), sparse.end(),
						 weights.begin() + i * width + j * feat_count);
			result[i] += exp(psi);
		}
	}
}

sparse_t LatentLogLinearModel::build_sparse(istream &input, bool binary) const
{
	if (binary)
		return sparse_from_binary(input, feat_id_map);
	else
		return sparse_from_real(input, feat_id_map);
}

size_t LatentLogLinearModel::test(const vector<DataPoint> &points) const
{
	size_t correct = 0;
	vec_t probs;
	for (auto i = points.begin(); i != points.end(); ++i)
		correct += (predict(i->sparse, probs) == i->label);
	return correct;
}
