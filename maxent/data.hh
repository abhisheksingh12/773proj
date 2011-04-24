#ifndef _DATA_HH
#define _DATA_HH

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <unordered_map>


typedef std::vector<double> vec_t;
typedef std::unordered_map<size_t, double> sparse_t;
typedef std::unordered_map<std::string, size_t> feat_id_map_t;


// always produce **BIAS** to 1 even though it does not appear in
// input or it is set to some other value.
sparse_t sparse_from_binary(std::istream &input, const feat_id_map_t &feat_id_map);
sparse_t sparse_from_real(std::istream &input, const feat_id_map_t &feat_id_map);

class DataPoint
{
public:
	DataPoint(size_t label, const sparse_t &sparse);
	void restore(std::ostream &output, const feat_id_map_t &feat_id_map, bool is_binary) const;

	size_t label;
	sparse_t sparse;
};

class DataSet
{
public:
	DataSet(std::istream &input, bool is_binary=true);

	void add_line(const std::string &line, std::vector<DataPoint> &dest, bool is_binary);
	void update_map(std::istream &input, bool is_binary);

	void update_map_from_binary(std::istream &input);
	void update_map_from_real(std::istream &input);

	friend std::ostream &operator<<(std::ostream &, const DataSet &);

	std::vector<DataPoint> train, dev, test;
	feat_id_map_t feat_id_map;
	size_t label_count;
	size_t feat_count;
	bool is_binary;
};

#endif

