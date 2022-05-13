/*
 * tuple_operators.h
 *
 *  Created on: Apr 6, 2013
 *      Author: eba
 */

#ifndef TUPLE_OPERATORS_H_
#define TUPLE_OPERATORS_H_

#include <tuple>
#include <utility>

namespace tuple_operators_internal {

using std::get;

template<class Tuple1, class Tuple2>
struct tuple_manipulator {
	template <int I>
	static void add_assign(Tuple1 & op1, const Tuple2 & op2) {
		get<I>(op1)+=get<I>(op2);
		if(I>0)	add_assign<I?I-1:0>(op1,op2);
	}
};

}

// MSVC2012 doesn't like this...
//template <class Tuple1, class... T2s>
//Tuple1 && operator+=(Tuple1 && op1, const std::tuple<T2s...> & op2) {
//	tuple_operators_internal::tuple_manipulator<Tuple1 &&,std::tuple<T2s...>>::template add_assign<sizeof...(T2s)-1>(op1,op2);
//	return std::forward<Tuple1>(op1);
//}

//We only need it for the two argument case, so a workaround:
template <class Tuple1, class ... Elements>
Tuple1 && operator+=(Tuple1 && op1, const std::tuple<Elements...> & op2) {
	// FIXME  add_assign<1> is a terrible workaround, it should be add_assign<length of op1 and op2>
	tuple_operators_internal::tuple_manipulator<Tuple1 &&,std::tuple<Elements...>>::template add_assign<sizeof...(Elements)-1>(op1,op2);
	return std::forward<Tuple1>(op1);
}


#endif /* TUPLE_OPERATORS_H_ */
