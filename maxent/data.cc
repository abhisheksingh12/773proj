#include "data.hh"

#include <algorithm>
#include <sstream>
#include <stdexcept>


using namespace std;

sparse_t sparse_from_binary(istream &input, const feat_id_map_t &feat_id_map)
{
	string feat_name;
	sparse_t ret;
	while (input >> feat_name) {
		// ignore unknown feature
		auto it = feat_id_map.find(feat_name);
		if (it != feat_id_map.end())
			ret[it->second] = 1;
	}
	// bias
	ret[feat_id_map.find("**BIAS**")->second] = 1;
	return ret;
}

sparse_t sparse_from_real(istream &input, const feat_id_map_t &feat_id_map)
{
	string feat_name;
	double value;
	sparse_t ret;
	while (input >> feat_name >> value) {
		// ignore unknown feature
		auto it = feat_id_map.find(feat_name);
		if (it != feat_id_map.end())
			ret[it->second] = value;
	}
	// bias
	ret[feat_id_map.find("**BIAS**")->second] = 1;
	return ret;
}

DataPoint::DataPoint(size_t label, const sparse_t &sparse) :
	label(label), sparse(sparse)
{
}

void DataPoint::restore(ostream &output, const feat_id_map_t &feat_id_map, bool is_binary) const
{
	output << label;
	for (auto i = feat_id_map.begin(); i != feat_id_map.end(); ++i) {
		auto it = sparse.find(i->second);
		if (it != sparse.end()) {
			output << '\t' << i->first;
			if (!is_binary)
				output << ' ' << it->second;
		}
	}
	output << '\n';
}

DataSet::DataSet(istream &input, bool is_binary) :
	is_binary(is_binary)
{
	// **BIAS** is always included
	feat_id_map["**BIAS**"] = 0;

	string line;

	// first pass: build `feat_id_map_t`; get `label_count` and
	// `feat_count`
	size_t max_label = 0;
	while (getline(input, line) && line != "DEV" && line != "TEST") {
		istringstream strin(line);
		size_t label;
		strin >> label;
		max_label = max(max_label, label);
		update_map(strin, is_binary);
	}
	label_count = max_label + 1;
	feat_count = feat_id_map.size();

	// second pass: actually reading data
	input.clear();
	input.seekg(0);
	// keep adding training data until meet DEV
	while (getline(input, line) && line != "DEV")
		add_line(line, train, is_binary);

	// keep adding dev data until meet TEST
	while (getline(input, line) && line != "TEST")
		add_line(line, dev, is_binary);

	// add test data
	while (getline(input, line))
		add_line(line, test, is_binary);
}

void DataSet::add_line(const std::string &line, vector<DataPoint> &dest, bool is_binary)
{
	istringstream strin(line);
	size_t label;
	strin >> label;
	if (is_binary)
		dest.push_back(DataPoint(label, sparse_from_binary(strin, feat_id_map)));
	else
		dest.push_back(DataPoint(label, sparse_from_real(strin, feat_id_map)));
}

void DataSet::update_map(istream &input, bool is_binary)
{
	if (is_binary)
		update_map_from_binary(input);
	else
		update_map_from_real(input);
}

void DataSet::update_map_from_binary(istream &input)
{
	string feat_name;
	while (input >> feat_name)
		if (feat_id_map.find(feat_name) == feat_id_map.end())
			feat_id_map[feat_name] = feat_id_map.size();
}

void DataSet::update_map_from_real(istream &input)
{
	string feat_name;
	double value;
	while (input >> feat_name >> value)
		if (feat_id_map.find(feat_name) == feat_id_map.end())
			feat_id_map[feat_name] = feat_id_map.size();
}

ostream &operator<<(ostream &output, const DataSet &data)
{
	for (auto i = data.train.begin(); i != data.train.end(); ++i)
		i->restore(output, data.feat_id_map, data.is_binary);
	if (data.dev.size()) {
		output << "DEV\n";
		for (auto i = data.dev.begin(); i != data.dev.end(); ++i)
			i->restore(output, data.feat_id_map, data.is_binary);
	}
	if (data.test.size()) {
		output << "TEST\n";
		for (auto i = data.test.begin(); i != data.test.end(); ++i)
			i->restore(output, data.feat_id_map, data.is_binary);
	}
	return output;
}


