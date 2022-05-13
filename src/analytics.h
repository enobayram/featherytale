/*
 * analytics.h
 *
 *  Created on: Feb 25, 2016
 *      Author: eba
 */

#ifndef ANALYTICS_H_
#define ANALYTICS_H_

#include <vector>

void init_analytics();
void start_analytics_session();
void update_analytics();
void exit_analytics();

enum progression_event_type {
    P_START, P_COMPLETE, P_FAIL
};
void progression_event(progression_event_type type, const char * level, int score = -1);

enum resource_event_type {
    R_SOURCE, R_SINK
};
void resource_event(resource_event_type type, double amount, const char * currency,
                    const char * itemType, const char * itemId);

void design_event(const std::vector<const char *> & fields);

#endif /* ANALYTICS_H_ */
