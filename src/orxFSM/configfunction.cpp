/*
 * configfunction.cpp
 *
 *  Created on: Jan 3, 2016
 *      Author: eba
 */

#include <algorithm>
#include <map>
#include <cstdlib>
#include <orxFSM/configfunction.h>

#include <scroll_ext/config_utils.hpp>

config_function::config_function(const char* section) {
    ConfigSectionGuard guard(section);
    xvalues = GetList<orxFLOAT>(nullptr, "XValues");
    yvalues = GetList<orxFLOAT>(nullptr, "YValues");
    auto interp = GetValue<const char *>(nullptr, "Interpolation");

    if(strcmp(interp, "Step") == 0) {
        interpolation = STEP;
    } else {
        interpolation = LINEAR;
    }

    extrapolate_left = orxConfig_GetBool("ExtrapolateLeft");
    extrapolate_right = orxConfig_GetBool("ExtrapolateRight");

    orxASSERT(std::is_sorted(xvalues.begin(), xvalues.end()))
    orxASSERT(xvalues.size() == yvalues.size())
}

config_function * config_function::create_from_section(const char* section) {
    return new config_function(section);
}

float config_function::apply(float xVal) {
    auto upper = std::upper_bound(xvalues.begin(), xvalues.end(), xVal);
    if(upper == xvalues.end()) return yvalues.back();
    if(upper == xvalues.begin()) return yvalues.front();
    size_t i = upper - xvalues.begin();
    switch(interpolation) {
    case STEP:
        return yvalues[i-1];
    case LINEAR: {
        float xl = xvalues[i-1], xr = xvalues[i];
        float yl = yvalues[i-1], yr = yvalues[i];
        float wl = (xr-xVal)/(xr-xl);
        return yl*wl + yr*(1-wl);
    }
    default:
        orxASSERT(false && "invalid value for interpolation")
        return 0;
    }
}

bool config_function::in_domain(float xVal) {
    if(!extrapolate_left && xVal < xvalues.front()) return false;
    if(!extrapolate_right && xVal > xvalues.back()) return false;
    return true;
}
