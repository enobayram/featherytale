/*
 * algorithms.hpp
 *
 *  Created on: Mar 28, 2015
 *      Author: eba
 */

#ifndef ALGORITHMS_HPP_
#define ALGORITHMS_HPP_

#include <utility>

#include "World.h"

template<class ...Ts>
bool always_true(Ts...ts) {return true;}

// TODO, this function could be much more efficient if it used the Node Cell Grid to prune.
template<class PointContainer,
         class PointFilter = bool(*)(Node &)>
auto nearestNode(const PointContainer & to_points, World & world, double max_distance, PointFilter filter = always_true)
-> std::pair<Node *, decltype(std::begin(to_points)) >  {
    auto result = std::make_pair<Node *>(nullptr, std::end(to_points));
    double minSquared = max_distance*max_distance;
    for(Node & node: world.getNodes()) {
        if(!filter(node)) continue;
        for(auto point_it = std::begin(to_points); point_it!=std::end(to_points); ++point_it) {
            double distSquared = (node.pos-*point_it).squaredNorm();
            if(distSquared<minSquared) {
                result = std::make_pair(&node, point_it);
                minSquared = distSquared;
            }
        }
    }
    return result;
}



#endif /* ALGORITHMS_HPP_ */
