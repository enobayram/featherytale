/*
 * weakly_pointable.cpp
 *
 *  Created on: Aug 9, 2015
 *      Author: enis
 */

#include "weakly_pointable.hpp"

hash_key_type get_next_key() {
    static hash_key_type next_key{0x2ll << 60ll};
    return next_key++;
}


