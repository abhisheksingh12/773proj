#ifndef _VEC_HH
#define _VEC_HH

#include <cstring> // for size_t

// vector arithmetics


// dot product
template <typename It1, typename It2>
double vv_dot_prod(It1 u_begin, It1 u_end,
		   It2 v_begin)
{
	double ret = 0;
	for (; u_begin != u_end; ++u_begin, ++v_begin)
		ret += (*u_begin) * (*v_begin);
	return ret;
}

template <typename SIt, typename It>
double sv_dot_prod(SIt u_begin, SIt u_end,
		   It v_begin)
{
	double ret = 0;
	for (; u_begin != u_end; ++u_begin) {
		size_t offset = u_begin->first;
		double value = u_begin->second;
		ret += value * v_begin[offset];
	}
	return ret;
}


// addition
template <typename It, typename SIt>
void vs_add_eq_mul_by(It u_begin,
		      SIt v_begin, SIt v_end,
		      double n)
{
	for (; v_begin != v_end; ++v_begin) {
		size_t offset = v_begin->first;
		double value = v_begin->second;
		u_begin[offset] += value * n;
	}
}


// subtraction
template <typename It, typename SIt>
void vs_sub_eq(It u_begin,
	       SIt v_begin, SIt v_end)
{
	for (; v_begin != v_end; ++v_begin) {
		size_t offset = v_begin->first;
		double value = v_begin->second;
		u_begin[offset] -= value;
	}
}


// multiplication
template <typename It1, typename It2>
void vv_mul_by(It1 u_begin,
	       It2 v_begin, It2 v_end,
	       double n)
{
	for (; v_begin != v_end; ++u_begin, ++v_begin)
		(*u_begin) = (*v_begin) * n;
}


#endif
