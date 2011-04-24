#ifndef _MAXENT_HH
#define _MAXENT_HH

#include <iostream>

#include "data.hh"

class MaxEntModel
{
public:
	void load_model(std::istream &input);
	void load_line(const std::string &line, size_t lineno);
	void dump_model(std::ostream &output);

	size_t predict(const sparse_t &sparse, vec_t &probs) const;
	void compute_scores(const sparse_t &sparse, vec_t &result) const;

	sparse_t build_sparse(std::istream &input, bool binary=true) const;

	void train(const DataSet &data, double lambda);


	size_t label_count, feat_count;
	vec_t weights;
	feat_id_map_t feat_id_map;
};

#endif
