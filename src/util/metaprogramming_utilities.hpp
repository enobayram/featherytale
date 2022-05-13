/*
 * metaprogramming_utilities.hpp
 *
 *  Created on: Dec 22, 2015
 *      Author: enis
 */

#ifndef UTIL_METAPROGRAMMING_UTILITIES_HPP_
#define UTIL_METAPROGRAMMING_UTILITIES_HPP_

#include "metaprogramming_primitives.hpp"

template <typename T>
struct function_traits: public function_traits<decltype(&T::operator())>{};
// For generic types, directly use the result of the signature of its 'operator()'

template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(Args...)>
// we specialize for functions
{
    enum { arity = sizeof...(Args) };
    // arity is the number of arguments.

    typedef ReturnType result_type;
    typedef arg_list<Args...> args_type;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        // the i-th argument is equivalent to the i-th tuple element of a tuple
        // composed of those arguments.
    };
};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const> : public function_traits<ReturnType(Args...)>
// we specialize for pointers to member function
{
};

#endif /* UTIL_METAPROGRAMMING_UTILITIES_HPP_ */
