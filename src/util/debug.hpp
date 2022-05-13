/*
 * debug.hpp
 *
 *  Created on: May 23, 2015
 *      Author: eba
 */

#ifndef DEBUG_HPP_
#define DEBUG_HPP_

#include <iostream>

class DebugOut {
public:
    template<class T>
    DebugOut operator<<(T && in) {
#ifdef DEBUG_MAPPED_TEXTURE
        std::cout<<std::forward<T>(in);
#endif
        return *this;
    }
};


#endif /* DEBUG_HPP_ */
