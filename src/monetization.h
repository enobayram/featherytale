/*
 * monetization.h
 *
 *  Created on: Mar 19, 2016
 *      Author: eba
 */

#ifndef MONETIZATION_H_
#define MONETIZATION_H_

#include <platform.h>

void init_monetization();
void update_monetization();
void exit_monetization();

bool RequestTransition(const char * level_name);

void purchaseAdFree();
void purchaseConsumable(const char * item_name);

#endif /* MONETIZATION_H_ */
