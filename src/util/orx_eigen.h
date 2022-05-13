/*
 * orx_eigen.h
 *
 *  Created on: May 31, 2015
 *      Author: eba
 */

#ifndef ORX_EIGEN_H_
#define ORX_EIGEN_H_

#include <array>

#include <Eigen/Core>

inline Eigen::Vector2d toEigen2d(orxVECTOR v) {
    return {v.fX,v.fY};
}

inline Eigen::Vector2f toEigen2f(orxVECTOR v) {
    return {v.fX,v.fY};
}

template <size_t N>
inline std::array<Eigen::Vector2d,N> toEigen2d(const std::array<orxVECTOR,N> & vs) {
    std::array<Eigen::Vector2d,N> result;
    for(int i=0; i<N; i++) result[i] = toEigen2d(vs[i]);
    return result;
}

template <class T>
inline orxVECTOR toOrxVector(const Eigen::Matrix<T,2,1> & in) {
    return {orxFLOAT(in.x()), orxFLOAT(in.y()), 0};
}

#endif /* ORX_EIGEN_H_ */
