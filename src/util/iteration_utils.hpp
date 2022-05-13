/*
 * iteration_utils.hpp
 *
 *  Created on: Dec 26, 2015
 *      Author: eba
 */

#ifndef UTIL_ITERATION_UTILS_HPP_
#define UTIL_ITERATION_UTILS_HPP_

#include <orx.h>

template <class FUNC>
void iterate_children(orxOBJECT * obj, FUNC f) {
    for ( orxOBJECT * child = orxObject_GetOwnedChild(obj);
          child;
          child = orxObject_GetOwnedSibling(orxOBJECT(child)) ) {
        f(child);
    }
}

template <class FUNC>
void iterate_object_tree(orxOBJECT * obj, FUNC f) {
    f(obj);
    iterate_children(obj, [&f](orxOBJECT * child) {
        iterate_object_tree(child, f);
    });
}

#endif /* UTIL_ITERATION_UTILS_HPP_ */
