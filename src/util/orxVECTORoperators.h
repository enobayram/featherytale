/*
 * orxVECTORoperators.h
 *
 *  Created on: May 31, 2015
 *      Author: eba
 */

#ifndef ORXVECTOROPERATORS_H_
#define ORXVECTOROPERATORS_H_

#include <math/orxVector.h>
#include <ostream>

inline orxVECTOR operator+(const orxVECTOR & v1, const orxVECTOR & v2) {
    return {v1.fX+v2.fX, v1.fY+v2.fY, v1.fZ+v2.fZ};
}

inline orxVECTOR operator-(const orxVECTOR & v1, const orxVECTOR & v2) {
    return {v1.fX-v2.fX, v1.fY-v2.fY, v1.fZ-v2.fZ};
}

inline orxVECTOR operator*(const orxVECTOR & v, orxFLOAT s) {
    return {v.fX*s, v.fY*s, v.fZ*s};
}

inline orxVECTOR operator/(const orxVECTOR & v, orxFLOAT s) {
    return {v.fX/s, v.fY/s, v.fZ/s};
}

inline orxFLOAT dot(const orxVECTOR &v1, const orxVECTOR & v2) {
    return v1.fX*v2.fX+v1.fY*v2.fY+v1.fZ*v2.fZ;
}

inline orxVECTOR piecewiseMult(const orxVECTOR & v1, const orxVECTOR & v2) {
    return {v1.fX*v2.fX, v1.fY*v2.fY, v1.fZ*v2.fZ};
}

inline orxFLOAT squaredNorm(const orxVECTOR &v) {
    return dot(v,v);
}

inline orxVECTOR normalized(const orxVECTOR & in) {
    orxVECTOR result;
    orxVector_Normalize(&result, &in);
    return result;
}

inline std::ostream & operator << (std::ostream & os, const orxVECTOR & v) {
    return os << "(" << v.fX << ", " << v.fY << ", " << v.fZ << ")";
}

inline orxVECTOR linearInterpolate(const orxVECTOR & src, const orxVECTOR & dst, double phase) {
    orxVECTOR result;
    for(auto ptr: {&orxVECTOR::fX ,&orxVECTOR::fY ,&orxVECTOR::fZ} ) {
        result.*ptr = src.*ptr * (1-phase) + dst.*ptr * phase;
    }
    return result;
}

#endif /* ORXVECTOROPERATORS_H_ */
