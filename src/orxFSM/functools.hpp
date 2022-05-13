/*
 * functools.hpp
 *
 *  Created on: Jun 1, 2015
 *      Author: eba
 */

#ifndef FUNCTOOLS_HPP_
#define FUNCTOOLS_HPP_

#include <functional>
#include <type_traits>

template <class Ret, class Arg1, class Arg2, class Arg2Cast, class F>
std::function<Ret(Arg1, Arg2)> condition_function(F f, typename std::result_of<F(Arg1,Arg2Cast)>::type *t=0 ) {
    return [f](Arg1 a1, Arg2 a2){return f(a1, static_cast<Arg2Cast>(a2));};
}

template <class Ret, class Arg1, class Arg2, class Arg2Cast, class F>
std::function<Ret(Arg1, Arg2)> condition_function(F f, typename std::result_of<F(Arg1)>::type *t=0 ) {
    return [f](Arg1 a1, Arg2){return f(a1);};
}

template <class Ret, class Arg1, class Arg2, class Arg2Cast, class F>
std::function<Ret(Arg1, Arg2)> condition_function(F f, typename std::result_of<F(Arg2Cast)>::type *t=0 ) {
    return [f](Arg1, Arg2 a2){return f(static_cast<Arg2Cast>(a2));};
}

#endif /* FUNCTOOLS_HPP_ */
