/*
 * platform.cpp
 *
 *  Created on: Feb 26, 2016
 *      Author: eba
 */

#include "platform.h"

Platform * platform; // a global platform pointer

void SetPlatform(Platform * p) {
  platform = p;
}
Platform * GetPlatform() {
  return platform;
}
