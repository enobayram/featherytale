/*
 * configfunction.h
 *
 *  Created on: Jan 3, 2016
 *      Author: eba
 */

#ifndef ORXFSM_CONFIGFUNCTION_H_
#define ORXFSM_CONFIGFUNCTION_H_

#include <vector>

class config_function {
    enum {STEP, LINEAR} interpolation;
    std::vector<float> xvalues, yvalues;
    bool extrapolate_left, extrapolate_right;
    config_function(const char * section);
public:
    static config_function * create_from_section(const char * section);
    float apply(float xVal);
    bool in_domain(float xVal);
};

#endif /* ORXFSM_CONFIGFUNCTION_H_ */
