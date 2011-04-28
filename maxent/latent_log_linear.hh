#ifndef _LATENT_LOG_LINEAR_HH
#define _LATENT_LOG_LINEAR_HH

#include <iostream>
#include <vector>
#include <string>

#include "data.hh"

struct LLLMStruct
{
	bool h, y, hx, hy, xy, hxy;
};

enum LLLMStructIndex {
	I_H = 0,
	I_Y,
	I_HX,
	I_HY,
	I_XY,
	I_HXY
};

class LatentLogLinearModel
{
public:
	void load_model(std::istream &input);
	void dump_model(std::ostream &output) const;

	void load_counts(std::istream &input);
	void load_st(std::istream &input);
	void load_w_h(std::istream &input);
	void load_w_y(std::istream &input);
	void load_w_hx(std::istream &input);
	void load_w_hy(std::istream &input);
	void load_w_xy(std::istream &input);
	void load_w_hxy(std::istream &input);

	void dump_counts(std::ostream &output) const;
	void dump_st(std::ostream &output) const;
	void dump_w_h(std::ostream &output) const;
	void dump_w_y(std::ostream &output) const;
	void dump_w_hx(std::ostream &output) const;
	void dump_w_hy(std::ostream &output) const;
	void dump_w_xy(std::ostream &output) const;
	void dump_w_hxy(std::ostream &output) const;

	void train(const DataSet &data, const LLLMStruct &st, size_t latent_count, double lambda);
	size_t predict(const sparse_t &sparse, vec_t &probs) const;
	size_t test(const std::vector<DataPoint> &points) const;
	void compute_scores(const sparse_t &sparse, vec_t &result) const;

	sparse_t build_sparse(std::istream &input, bool binary=true) const;

	// needs valid label_count, feat_count, latent_count and st
	size_t compute_weight_size() const;
	void set_weight_size(size_t n);

	size_t query_or_assign_feat_id(const std::string &feat_name);

	LLLMStruct st;
	size_t label_count, feat_count, latent_count;
	vec_t weights;
	vec_t::iterator w[6];
	feat_id_map_t feat_id_map;
};

#endif
