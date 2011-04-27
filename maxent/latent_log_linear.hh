#ifndef _LATENT_LOG_LINEAR_HH
#define _LATENT_LOG_LINEAR_HH

#include <iostream>
#include <vector>

#include "data.hh"

// Simplest model:
// h -- X
// |  /
// | /
// Y

struct LLLMStruct
{
	bool h, x, y, hx, hy, xy, hxy;
};

class LatentLogLinearModel
{
public:
	void load_model(std::istream &input);
	void load_line(const std::string &line, size_t h, size_t lineno);
	void dump_model(std::ostream &output);

	size_t predict(const sparse_t &sparse, vec_t &probs) const;
	size_t test(const std::vector<DataPoint> &points) const;
	void compute_scores(const sparse_t &sparse, vec_t &result) const;

	sparse_t build_sparse(std::istream &input, bool binary=true) const;

	void train(const DataSet &data, const LLLMStruct &st, size_t latent_count, double lambda);

	LLLMStruct st;
	size_t label_count, feat_count, latent_count;
	vec_t weights;
	feat_id_map_t feat_id_map;
};

#endif
