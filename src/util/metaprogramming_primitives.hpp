/*
 * metaprogramming_primitives.hpp
 *
 *  Created on: Aug 9, 2015
 *      Author: enis
 */

#ifndef UTIL_METAPROGRAMMING_PRIMITIVES_HPP_
#define UTIL_METAPROGRAMMING_PRIMITIVES_HPP_

template<size_t ...>
struct num_range {};

template <class ...>
struct arg_list{};

template <class T>
struct type_carrier{};



#endif /* UTIL_METAPROGRAMMING_PRIMITIVES_HPP_ */
