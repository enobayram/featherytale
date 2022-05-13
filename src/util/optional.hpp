/*
 * optional.hpp
 *
 *  Created on: Jun 2, 2015
 *      Author: eba
 */

#ifndef OPTIONAL_HPP_
#define OPTIONAL_HPP_

template <class Value>
struct optional {
    Value val;
    bool contains;
    optional(const Value & val): val(val), contains(true) {}
    optional():val(), contains(false){}
    operator bool() { return contains;}
    Value & operator *() {return val;}
    Value * operator ->() {return &val;}
};

#endif /* OPTIONAL_HPP_ */
