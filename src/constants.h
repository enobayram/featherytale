/*
 * constants.h
 *
 *  Created on: Nov 2, 2011
 *      Author: eba
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include<cmath>

const double PI = 3.14159265359;

const int WIDTH = 800;
const int HEIGHT = 600;
const int BPP = 4;
const int DEPTH = 32;
const int RENDERDELAY = 20;
const int FORCERADIUS = 30;
const double BONDLENGTH = 1;
const double NODESIZE = 0.2;

const int GHOSTCOUNT = 6;
const double GHOSTLENGTH = BONDLENGTH*1.8/2;
const double BONDGHOSTDISTANCE = BONDLENGTH*0.9/(2*sqrt(3));
const double MINTRIANGLEDISTANCE = BONDLENGTH*0.7/sqrt(3);
const double NEWNODEDISTANCE = MINTRIANGLEDISTANCE*1.2;

const int GRIDSIZEX = 100;
const int GRIDSIZEY = GRIDSIZEX;
const double CELLWIDTH = 2;

const double MINX = -GRIDSIZEX/2 * CELLWIDTH;
const double MAXX = GRIDSIZEX/2 * CELLWIDTH;
const double MINY = -GRIDSIZEY/2 * CELLWIDTH;
const double MAXY = GRIDSIZEY/2 * CELLWIDTH;

#endif /* CONSTANTS_H_ */
