/*
 * Simulator.h
 *
 *  Created on: Nov 2, 2011
 *      Author: eba
 */

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include <thread>
#include <atomic>

#include <scroll_ext/eventdispatcher.h>
#include "physicalElements.h"

struct energy_change_event {
    using signature = void(const Vector2d & pos, double amount, bool dissipated, bool radial);
};

struct bond_tear_event {
    using signature = void(Bond *);
};

class Simulator: public event_dispatcher {
	std::atomic<bool> running;
	World & world;
	void simulate();
	std::atomic<double> simulationTarget;
	friend class callable;
	std::atomic<bool> stopped;
	std::atomic<bool> shouldPause;
	double worldTop=0, worldBottom=0, worldLeft=0, worldRight=0;
public:
	Simulator(World & world);
	~Simulator();
	void init();
	void start();
	void stop();
	void pause();
	void resume();
	void iterate();
	void simulateUntil(double time);
};

double get_gravity();

#endif /* SIMULATOR_H_ */
