/*
 * random.hpp
 *
 *  Created on: May 23, 2015
 *      Author: eba
 */

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>

extern std::mt19937 gen;

class poisson_event_generator {
    double dt;
public:
    poisson_event_generator(double dt): dt(dt) {}
    bool operator()(double period) {
        return std::exponential_distribution<>(1/period)(gen)<dt;
    }
};


#endif /* RANDOM_HPP_ */
