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

static istream &getline_no_comment(istream &input, string &line)
{
	do
		getline(input, line);
	while (line.size() && line[0] == '#');
	return input;
}

string lbfgs_ret(int ret)
{
	switch (ret) {
	case LBFGS_SUCCESS:
		return "LBFGS_SUCCESS";
	case LBFGS_STOP:
		return "LBFGS_STOP";
	case LBFGS_ALREADY_MINIMIZED:
		return "LBFGS_ALREADY_MINIMIZED";
	case LBFGSERR_UNKNOWNERROR:
		return "LBFGSERR_UNKNOWNERROR";
	case LBFGSERR_LOGICERROR:
		return "LBFGSERR_LOGICERROR";
	case LBFGSERR_OUTOFMEMORY:
		return "LBFGSERR_OUTOFMEMORY";
	case LBFGSERR_CANCELED:
		return "LBFGSERR_CANCELED";
	case LBFGSERR_INVALID_N:
		return "LBFGSERR_INVALID_N";
	case LBFGSERR_INVALID_N_SSE:
		return "LBFGSERR_INVALID_N_SSE";
	case LBFGSERR_INVALID_X_SSE:
		return "LBFGSERR_INVALID_X_SSE";
	case LBFGSERR_INVALID_EPSILON:
		return "LBFGSERR_INVALID_EPSILON";
	case LBFGSERR_INVALID_TESTPERIOD:
		return "LBFGSERR_INVALID_TESTPERIOD";
	case LBFGSERR_INVALID_DELTA:
		return "LBFGSERR_INVALID_DELTA";
	case LBFGSERR_INVALID_LINESEARCH:
		return "LBFGSERR_INVALID_LINESEARCH";
	case LBFGSERR_INVALID_MINSTEP:
		return "LBFGSERR_INVALID_MINSTEP";
	case LBFGSERR_INVALID_MAXSTEP:
		return "LBFGSERR_INVALID_MAXSTEP";
	case LBFGSERR_INVALID_FTOL:
		return "LBFGSERR_INVALID_FTOL";
	case LBFGSERR_INVALID_WOLFE:
		return "LBFGSERR_INVALID_WOLFE";
	case LBFGSERR_INVALID_GTOL:
		return "LBFGSERR_INVALID_GTOL";
	case LBFGSERR_INVALID_XTOL:
		return "LBFGSERR_INVALID_XTOL";
	case LBFGSERR_INVALID_MAXLINESEARCH:
		return "LBFGSERR_INVALID_MAXLINESEARCH";
	case LBFGSERR_INVALID_ORTHANTWISE:
		return "LBFGSERR_INVALID_ORTHANTWISE";
	case LBFGSERR_INVALID_ORTHANTWISE_START:
		return "LBFGSERR_INVALID_ORTHANTWISE_START";
	case LBFGSERR_INVALID_ORTHANTWISE_END:
		return "LBFGSERR_INVALID_ORTHANTWISE_END";
	case LBFGSERR_OUTOFINTERVAL:
		return "LBFGSERR_OUTOFINTERVAL";
	case LBFGSERR_INCORRECT_TMINMAX:
		return "LBFGSERR_INCORRECT_TMINMAX";
	case LBFGSERR_ROUNDING_ERROR:
		return "LBFGSERR_ROUNDING_ERROR";
	case LBFGSERR_MINIMUMSTEP:
		return "LBFGSERR_MINIMUMSTEP";
	case LBFGSERR_MAXIMUMSTEP:
		return "LBFGSERR_MAXIMUMSTEP";
	case LBFGSERR_MAXIMUMLINESEARCH:
		return "LBFGSERR_MAXIMUMLINESEARCH";
	case LBFGSERR_MAXIMUMITERATION:
		return "LBFGSERR_MAXIMUMITERATION";
	case LBFGSERR_WIDTHTOOSMALL:
		return "LBFGSERR_WIDTHTOOSMALL";
	case LBFGSERR_INVALIDPARAMETERS:
		return "LBFGSERR_INVALIDPARAMETERS";
	case LBFGSERR_INCREASEGRADIENT:
		return "LBFGSERR_INCREASEGRADIENT";
	}
	return "UNKNOWN";
}


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
	const DataSet &data = td.data;
	const LLLMStruct &st = td.st;

	size_t width = latent_count * feat_count;

	size_t offset_h = 0;
	size_t offset_y = offset_h + (st.h ? latent_count : 0);
	size_t offset_hx = offset_y + (st.y ? label_count : 0);
	size_t offset_hy = offset_hx + (st.hx ? latent_count * feat_count : 0);
	size_t offset_xy = offset_hy + (st.hy ? latent_count * label_count : 0);
	size_t offset_hxy = offset_xy + (st.xy ? label_count * feat_count : 0);

	// initialize with L2 regularization terms
	double f = 0.5 * lambda * vv_dot_prod(x, x + n, x);
	vv_mul_by(g, x, x + n, lambda);
	// Per-example sum
	for (auto i = data.train.begin(); i != data.train.end(); ++i) {
		// per-label score \sum_h exp(psi(x, y, h))

		// exp(psi(x, y, h))
		vec_t exp_psi(label_count * latent_count);
		for (size_t j = 0; j < label_count; ++j) {
			double without_h = 0;
			if (st.y)
				without_h += x[offset_y + j];
			if (st.xy)
				without_h += sv_dot_prod(i->sparse.begin(), i->sparse.end(),
							 x + offset_xy + j * feat_count);
			for (size_t h = 0; h < latent_count; ++h) {
				double psi = without_h;
				if (st.h)
					psi += x[offset_h + h];
				if (st.hx)
					psi += sv_dot_prod(i->sparse.begin(), i->sparse.end(),
							   x + offset_hx + h * feat_count);
				if (st.hy)
					psi += x[offset_hy + j * latent_count + h];
				if (st.hxy)
					psi += sv_dot_prod(i->sparse.begin(), i->sparse.end(),
							   x + offset_hxy + j * width + h * feat_count);
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
		for (size_t h = 0; h < latent_count; ++h) {
			double ratio = exp_psi[ref_label * latent_count + h] / sum_exp_psi_ref;
			if (st.h)
				g[offset_h + h] -= ratio;
			if (st.y)
				g[offset_y + ref_label] -= ratio;
			if (st.hx)
				vs_sub_eq_mul_by(g + offset_hx + h * feat_count,
						 i->sparse.begin(),
						 i->sparse.end(),
						 ratio);
			if (st.hy)
				g[offset_hy + ref_label * latent_count + h] -= ratio;
			if (st.xy)
				vs_sub_eq_mul_by(g + offset_xy + ref_label * feat_count,
						 i->sparse.begin(),
						 i->sparse.end(),
						 ratio);
			if (st.hxy)
				vs_sub_eq_mul_by(g + offset_hxy + ref_label * width + h * feat_count,
						 i->sparse.begin(),
						 i->sparse.end(),
						 ratio);
		}

		// update Z (partition function) terms
		// Z = \sum_{y,h} exp(psi(x,y,h))
		double Z = accumulate(exp_psi.begin(),
				      exp_psi.end(),
				      0.0);
		// +log Z
		f += log(Z);
		// for each (y,h), + exp_psi(x,y,h)/Z * \nabla psi(x,y,h)
		for (size_t j = 0; j < label_count; ++j)
			for (size_t h = 0; h < latent_count; ++h) {
				double ratio = exp_psi[j * latent_count + h] / Z;
				if (st.h)
					g[offset_h + h] += ratio;
				if (st.y)
					g[offset_y + j] += ratio;
				if (st.hx)
					vs_add_eq_mul_by(g + offset_hx + h * feat_count,
							 i->sparse.begin(),
							 i->sparse.end(),
							 ratio);
				if (st.hy)
					g[offset_hy + j * latent_count + h] += ratio;
				if (st.xy)
					vs_add_eq_mul_by(g + offset_xy + j * feat_count,
							 i->sparse.begin(),
							 i->sparse.end(),
							 ratio);
				if (st.hxy)
					vs_add_eq_mul_by(g + offset_hxy + j * width + h * feat_count,
							 i->sparse.begin(),
							 i->sparse.end(),
							 ratio);
			}
	}

	return f;
}

static size_t _test(const vector<DataPoint> &points, const double *x,
		    size_t feat_count, size_t label_count, size_t latent_count,
		    const LLLMStruct &st)
{
	size_t offset_h = 0;
	size_t offset_y = offset_h + (st.h ? latent_count : 0);
	size_t offset_hx = offset_y + (st.y ? label_count : 0);
	size_t offset_hy = offset_hx + (st.hx ? latent_count * feat_count : 0);
	size_t offset_xy = offset_hy + (st.hy ? latent_count * label_count : 0);
	size_t offset_hxy = offset_xy + (st.xy ? label_count * feat_count : 0);

	size_t correct = 0;
	size_t width = feat_count * latent_count;
	vec_t scores(label_count);
	for (auto i = points.begin(); i != points.end(); ++i) {
		for (size_t j = 0; j != label_count; ++j) {
			scores[j] = 0;
			double without_h = 0;
			if (st.y)
				without_h += x[offset_y + j];
			if (st.xy)
				without_h += sv_dot_prod(i->sparse.begin(), i->sparse.end(),
							 x + offset_xy + j * feat_count);
			for (size_t h = 0; h < latent_count; ++h) {
				double psi = without_h;
				if (st.h)
					psi += x[offset_h + h];
				if (st.hx)
					psi += sv_dot_prod(i->sparse.begin(), i->sparse.end(),
							   x + offset_hx + h * feat_count);
				if (st.hy)
					psi += x[offset_hy + j * latent_count + h];
				if (st.hxy)
					psi += sv_dot_prod(i->sparse.begin(), i->sparse.end(),
							   x + offset_hxy + j * width + h * feat_count);
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
	const LLLMStruct &st = td.st;

// #if DEBUG
// 	double train_correct = _test(data.train, x, feat_count, label_count, latent_count);
// 	cerr << "\ttr_err=" << (data.train.size() - train_correct) / data.train.size();
// #endif

	if (data.dev.size()) {
		double dev_correct = _test(data.dev, x, feat_count, label_count, latent_count, st);
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


size_t LatentLogLinearModel::query_or_assign_feat_id(const string &feat_name)
{
	size_t feat_id;
	if (feat_id_map.count(feat_name) == 0) {
		feat_id = feat_id_map.size();
		feat_id_map[feat_name] = feat_id;
	} else
		feat_id = feat_id_map[feat_name];
	return feat_id;
}

void LatentLogLinearModel::load_counts(istream &input)
{
	string line;
	getline_no_comment(input, line);
	try {
		label_count = stoi(line);
	} catch (invalid_argument &e) {
		throw runtime_error("can't read label count");
	}

	getline_no_comment(input, line);
	try {
		latent_count = stoi(line);
	} catch (invalid_argument &e) {
		throw runtime_error("can't read latent count");
	}

	getline_no_comment(input, line);
	try {
		feat_count = stoi(line);
	} catch (invalid_argument &e) {
		throw runtime_error("can't read feat count");
	}
}

void LatentLogLinearModel::load_st(istream &input)
{
	string line;
	getline_no_comment(input, line);
	if (line.size() != 11)
		throw runtime_error("invalid st line: " + line);
	for (size_t i = 0; i < 11; i += 2)
		if (line[i] != '0' && line[i] != '1')
			throw runtime_error("invalid st line: " + line);
	for (size_t i = 1; i < 11; i += 2)
		if (line[i] != ' ' && line[i] != '\t')
			throw runtime_error("invalid st line: " + line);
	istringstream strin(line);
	strin >> st.h >> st.y >> st.hx >> st.hy >> st.xy >> st.hxy;
}

void LatentLogLinearModel::load_w_h(istream &input)
{
	if (st.h) {
		string line;
		getline_no_comment(input, line);
		istringstream strin(line);
		for (size_t i = 0; i < latent_count; ++i)
			strin >> w[I_H][i];
	}
}

void LatentLogLinearModel::load_w_y(istream &input)
{
	if (st.y) {
		string line;
		getline_no_comment(input, line);
		istringstream strin(line);
		for (size_t i = 0; i < label_count; ++i)
			strin >> w[I_Y][i];
	}
}

void LatentLogLinearModel::load_w_hx(istream &input)
{
	if (st.hx) {
		string line;
		for (size_t i = 0; i < feat_count; ++i) {
			getline_no_comment(input, line);
			istringstream strin(line);

			string feat_name;
			strin >> feat_name;

			size_t feat_id = query_or_assign_feat_id(feat_name);

			for (size_t j = 0; j < latent_count; ++j)
				strin >> w[I_HX][j * feat_count + feat_id];
		}
	}
}

void LatentLogLinearModel::load_w_hy(istream &input)
{
	if (st.hy) {
		string line;
		for (size_t i = 0; i < latent_count; ++i) {
			getline_no_comment(input, line);
			istringstream strin(line);
			for (size_t j = 0; j < label_count; ++j)
				strin >> w[I_HY][j * latent_count + i];
		}
	}
}

void LatentLogLinearModel::load_w_xy(istream &input)
{
	if (st.xy) {
		string line;
		for (size_t i = 0; i < feat_count; ++i) {
			getline_no_comment(input, line);
			istringstream strin(line);

			string feat_name;
			strin >> feat_name;

			size_t feat_id = query_or_assign_feat_id(feat_name);

			for (size_t j = 0; j < label_count; ++j)
				strin >> w[I_XY][j * feat_count + feat_id];
		}
	}
}

void LatentLogLinearModel::load_w_hxy(istream &input)
{
	if (st.hxy) {
		string line;
		size_t width = feat_count * latent_count;
		for (size_t i = 0; i < latent_count; ++i)
			for (size_t j = 0; j < feat_count; ++j) {
				getline_no_comment(input, line);
				istringstream strin(line);

				string feat_name;
				strin >> feat_name;

				size_t feat_id = query_or_assign_feat_id(feat_name);

				for (size_t k = 0; k < label_count; ++k)
					strin >> w[I_HXY][k * width + i * feat_count + feat_id];
			}
	}
}

void LatentLogLinearModel::load_model(istream &input)
{
	load_counts(input);
	load_st(input);
	set_weight_size(compute_weight_size());
	load_w_h(input);
	load_w_y(input);
	load_w_hx(input);
	load_w_hy(input);
	load_w_xy(input);
	load_w_hxy(input);
	// make sure there is an id for **BIAS**
	query_or_assign_feat_id("**BIAS**");
}

void LatentLogLinearModel::dump_counts(ostream &output) const
{
	output << "# label\n"
	       << label_count << '\n'
	       << "# latent\n"
	       << latent_count << '\n'
	       << "# feat\n"
	       << feat_count << '\n';
}

void LatentLogLinearModel::dump_st(ostream &output) const
{
	output << "# h y hx hy xy hxy\n"
	       << st.h << ' '
	       << st.y << ' '
	       << st.hx << ' '
	       << st.hy << ' '
	       << st.xy << ' '
	       << st.hxy << '\n';
}

void LatentLogLinearModel::dump_w_h(ostream &output) const
{
	if (st.h) {
		output << "# w_h\n";
		for (size_t i = 0; i != latent_count; ++i)
			output << (i == 0 ? "" : " ")
			       << w[I_H][i];
		output << '\n';
	}
}

void LatentLogLinearModel::dump_w_y(ostream &output) const
{
	if (st.y) {
		output << "# w_y\n";
		for (size_t i = 0; i != label_count; ++i)
			output << (i == 0 ? "" : " ")
			       << w[I_Y][i];
		output << '\n';
	}
}

void LatentLogLinearModel::dump_w_hx(ostream &output) const
{
	if (st.hx) {
		output << "# w_hx\n";
		// always output bias first
		output << "**BIAS**";
		size_t bias_feat_id = feat_id_map.find("**BIAS**")->second;
		for (size_t i = 0; i != latent_count; ++i)
			output << (i == 0 ? '\t' : ' ')
			       << w[I_HX][i * feat_count + bias_feat_id];
		output << '\n';
		// other features
		for (auto i = feat_id_map.begin(); i != feat_id_map.end(); ++i) {
			if (i->second == bias_feat_id)
				continue;
			output << i->first;
			for (size_t j = 0; j != latent_count; ++j)
				output << (j == 0 ? '\t' : ' ')
				       << w[I_HX][j * feat_count + i->second];
			output << '\n';
		}
	}
}

void LatentLogLinearModel::dump_w_hy(ostream &output) const
{
	if (st.hy) {
		output << "# w_hy\n";
		for (size_t i = 0; i != latent_count; ++i) {
			for (size_t j = 0; j != label_count; ++j)
				output << (j == 0 ? "" : " ")
				       << w[I_HY][j * latent_count + i];
			output << '\n';
		}
	}
}

void LatentLogLinearModel::dump_w_xy(ostream &output) const
{
	if (st.xy) {
		output << "# w_xy\n";
		// always output bias first
		output << "**BIAS**";
		size_t bias_feat_id = feat_id_map.find("**BIAS**")->second;
		for (size_t i = 0; i != label_count; ++i)
			output << (i == 0 ? '\t' : ' ')
			       << w[I_XY][i * feat_count + bias_feat_id];
		output << '\n';
		// other features
		for (auto i = feat_id_map.begin(); i != feat_id_map.end(); ++i) {
			if (i->second == bias_feat_id)
				continue;
			output << i->first;
			for (size_t j = 0; j != label_count; ++j)
				output << (j == 0 ? '\t' : ' ')
				       << w[I_XY][j * feat_count + i->second];
			output << '\n';
		}
	}
}

void LatentLogLinearModel::dump_w_hxy(ostream &output) const
{
	if (st.hxy) {
		output << "# w_hxy\n";
		size_t width = feat_count * latent_count;
		for (size_t h = 0; h != latent_count; ++h) {
			// always output bias first
			output << "**BIAS**";
			size_t bias_feat_id = feat_id_map.find("**BIAS**")->second;
			for (size_t i = 0; i != label_count; ++i)
				output << (i == 0 ? '\t' : ' ')
				       << w[I_HXY][i * width + h * feat_count + bias_feat_id];
			output << '\n';
			// other features
			for (auto i = feat_id_map.begin(); i != feat_id_map.end(); ++i) {
				if (i->second == bias_feat_id)
					continue;
				output << i->first;
				for (size_t j = 0; j != label_count; ++j)
					output << (j == 0 ? '\t' : ' ')
					       << w[I_HXY][j * width + h * feat_count + i->second];
				output << '\n';
			}
		}
	}
}

void LatentLogLinearModel::dump_model(ostream &output) const
{
	output << setiosflags(ios_base::fixed) << setprecision(20);
	dump_counts(output);
	dump_st(output);
	dump_w_h(output);
	dump_w_y(output);
	dump_w_hx(output);
	dump_w_hy(output);
	dump_w_xy(output);
	dump_w_hxy(output);
}

size_t LatentLogLinearModel::compute_weight_size() const
{
	size_t n = 0;
	if (st.h) n += latent_count;
	if (st.y) n += label_count;
	if (st.hx) n += latent_count * feat_count;
	if (st.hy) n += latent_count * label_count;
	if (st.xy) n += feat_count * label_count;
	if (st.hxy) n += feat_count * latent_count * label_count;
	return n;
}

void LatentLogLinearModel::set_weight_size(size_t n)
{
	weights.resize(n);
	w[I_H] = weights.begin();
	w[I_Y] = w[I_H] + (st.h ? latent_count : 0);
	w[I_HX] = w[I_Y] + (st.y ? label_count : 0);
	w[I_HY] = w[I_HX] + (st.hx ? latent_count * feat_count : 0);
	w[I_XY] = w[I_HY] + (st.hy ? latent_count * label_count : 0);
	w[I_HXY] = w[I_XY] + (st.xy ? label_count * feat_count : 0);
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
	size_t n = compute_weight_size();

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
		cerr << "Warning: lbfgs terminated with error " << lbfgs_ret(ret) << endl;
	// store weights
	set_weight_size(n);
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
		double without_h = 0;
		if (st.y)
			without_h += w[I_Y][i];
		if (st.xy)
			without_h += sv_dot_prod(sparse.begin(), sparse.end(),
						 w[I_XY] + i * feat_count);
		for (size_t j = 0; j != latent_count; ++j) {
			double psi = without_h;
			if (st.h)
				psi += w[I_H][j];
			if (st.hx)
				psi += sv_dot_prod(sparse.begin(), sparse.end(),
						   w[I_HX] + j * feat_count);
			if (st.hy)
				psi += w[I_HY][i * latent_count + j];
			if (st.hxy)
				psi += sv_dot_prod(sparse.begin(), sparse.end(),
						   w[I_HXY] + i * width + j * feat_count);
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
