/*
 * Crow.h
 *
 *  Created on: Feb 12, 2015
 *      Author: seliz
 */

#ifndef GAMEOBJECTS_CROW_H_
#define GAMEOBJECTS_CROW_H_

#include <scroll_ext/ExtendedObject.h>
#include <scroll_ext/eventdispatcher.h>

#include <gameObjects/aspects.h>
#include "orxFSM/orxFSM_header.h"

class Node;

class GameLevel;

class Crow: public state_machine_mixin<Crow>, public click_handler, public event_dispatcher {
	optional<orxVECTOR> poked;
public:
	Crow();
    ORX_STATES(Enter, Stay, FlyAtDistance, Dive,
               Hover, Stunned, Harassing, FlyOnScreen,
               Home, Exit, Combat, Bite, RaiseWings, Order);
    GameLevel * level = nullptr;
    SCROLL_BIND_CLASS("Crow")
	void poke(orxVECTOR pokepos);
    virtual void Update(const orxCLOCK_INFO &_rstInfo);
    virtual void OnNewAnim(const orxSTRING _zOldAnim, const orxSTRING _zNewAnim, orxBOOL _bCut);
    void setLevel(GameLevel * level) { this->level = level;}
    void SetContext(ExtendedMonomorphic & context);
    gesture * clicked(trail &);
    static const transition_table<Crow> & getTransitionTable();
    struct Controller;
};

#endif /* GAMEOBJECTS_CROW_H_ */
