#include "vec.hh"
#include <vector>
#include <iostream>
#include <ios>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>

using namespace std;

static inline size_t _num_of_cols(const string &line)
{
	istringstream strin(line);
	size_t ret = 0;
	string buf;
	while (strin >> buf)
		++ret;
	return ret;
}

template <typename It>
void output(It begin, It end)
{
	for (It i = begin; i != end; ++i)
		cout << (i == begin ? "" : " ") << *i;
	cout << endl;
}

int main()
{
	double a[] = {1,2,3};
	vector<double> b;
	b.push_back(0.1);
	b.push_back(1.2);
	b.push_back(2.3);

	map<size_t, double> c;
	c[0] = 1.5;
	c[2] = 1.6;

	cout << vv_dot_prod(a, a + 3, b.begin()) << endl;
	cout << sv_dot_prod(c.begin(), c.end(), a) << endl;

	{
		vector<double> tmp(b.begin(), b.end());
		vs_add_eq_mul_by(tmp.begin(),
				 c.begin(), c.end(),
				 5);
		output(tmp.begin(), tmp.end());
	}

	{
		vector<double> tmp(b.begin(), b.end());
		vs_sub_eq(tmp.begin(),
			  c.begin(), c.end());
		output(tmp.begin(), tmp.end());
	}

	{
		vector<double> tmp(b.begin(), b.end());
		vv_mul_by(tmp.begin(),
			  tmp.begin(), tmp.end(),
			  0.5);
		output(tmp.begin(), tmp.end());
	}

	cout << _num_of_cols("This is a\t12345.test") << endl;

	return 0;
}

