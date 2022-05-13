/*
 * geometry.hpp
 *
 *  Created on: May 23, 2015
 *      Author: eba
 */

#ifndef GEOMETRY_HPP_
#define GEOMETRY_HPP_

#define _USE_MATH_DEFINES
#include <cstdlib>
#include <cmath>

inline double atan2_dt(double y, double dy_dt,  double x, double dx_dt) {
    double partialX = -y/(x*x+y*y)*dx_dt;
    double partialY = x/(x*x+y*y)*dy_dt;
    return partialX+partialY;
}

// Normalize an angle between -2pi -> 2pi to 0 -> 2pi
inline double positiveAngle(double angle) {
    assert(angle>= -2*M_PI);
    assert(angle<= 2*M_PI);
    if(angle<0) return angle+ 2*M_PI;
    else return angle;
}

// Normalize an angle between -3pi -> 3pi to -pi -> pi
inline double normalizeAngle(double angle) {
    assert(angle>= -3*M_PI);
    assert(angle<= 3*M_PI);
    if(angle>M_PI) return angle-2*M_PI;
    else if(angle<-M_PI) return angle + 2*M_PI;
    else return angle;
}

// Normalize an angle between -5pi -> 5pi to -pi -> pi
inline double normalizeAngle2(double angle) {
    assert(angle>= -5*M_PI);
    assert(angle<= 5*M_PI);
    if(angle>M_PI) {
        if(angle>3*M_PI) return angle-4*M_PI;
        return angle-2*M_PI;
    }
    else if(angle<-M_PI) {
        if(angle<-3*M_PI) return angle + 4*M_PI;
        return angle + 2*M_PI;
    }
    else return angle;
}

// Return the angle in the middle of angle1 and angle2 clockwise from angle1
inline double midAngleClockwise(double angle1, double angle2) {
    double difference = angle2-angle1;
    if(difference<0) difference += 2*M_PI;
    return normalizeAngle(angle1+difference/2);
}

#endif /* GEOMETRY_HPP_ */
