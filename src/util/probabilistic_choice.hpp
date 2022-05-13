/*
 * probabilistic_choice.hpp
 *
 *  Created on: May 16, 2015
 *      Author: eba
 */

#ifndef PROBABILISTIC_CHOICE_HPP_
#define PROBABILISTIC_CHOICE_HPP_

#include <random>
#include <utility>
#include <initializer_list>
#include <functional>

template <class Item, class Generator>
Item make_choice(const std::initializer_list<std::pair<Item, double>> & alternatives, Generator & gen) {
    double total_weight = 0;
    for(auto & alt: alternatives) {
        total_weight += alt.second;
    }
    double choice = std::uniform_real_distribution<>(0,total_weight)(gen);
    for(auto &alt: alternatives) {
        choice -= alt.second;
        if(choice<=0) {
            return alt.first;
        }
    }
    return alternatives.begin()->first;
}

template <class Generator>
void random_action(std::initializer_list<std::pair<std::function<void()>,double>> actions, Generator & gen) {
    auto action = make_choice(actions, gen);
    action();
}

#endif /* PROBABILISTIC_CHOICE_HPP_ */
