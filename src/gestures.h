/*
 * gestures.h
 *
 *  Created on: Mar 20, 2015
 *      Author: seliz
 */

#ifndef GESTURES_H_
#define GESTURES_H_

#include <vector>
#include <base/orxType.h>

#include <Eigen/Core>

#include <orx.h>

#include <physicalElements/World.h>

typedef unsigned trail_id;

struct trace {
    double x;
    double y;
    double t;
};

inline trace make_trace(const Eigen::Vector2d & v, double t) {
	return trace{v.x(), v.y(), t};
}

inline orxVECTOR traceToOrxVector(const trace & t) {
    return {(float)t.x, (float)t.y, 0};
}

class gesture;

struct trail {
    std::vector<trace> traces;
    gesture * attached_gesture = nullptr;
    bool active = true;
};

class point_field {

};

class gesture {
public:
    virtual orxSTATUS time_update(double time) = 0;
    virtual ~gesture();
};

class force_gesture: public gesture {
    ExternalForce *ext_force;
	World & world;
	trail * t;
public:
    force_gesture(ExternalForce * force, World & world, trail * t);
    virtual orxSTATUS time_update(double time);
    ~force_gesture();
};

class null_gesture: public gesture {
    virtual orxSTATUS time_update(double time) {return orxSTATUS_FAILURE;};
};

#endif /* GESTURES_H_ */
