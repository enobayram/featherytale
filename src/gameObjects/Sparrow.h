/*
 * Sparrow.h
 *
 *  Created on: Feb 26, 2015
 *      Author: seliz
 */

#ifndef GAMEOBJECTS_SPARROW_H_
#define GAMEOBJECTS_SPARROW_H_

#include <Scroll.h>
#include <scroll_ext/ExtendedObject.h>
#include <scroll_ext/eventdispatcher.h>
#include "HappinessIndicator.h"
#include <orxFSM/orxFSM_header.h>
#include <orxFSM/behavior_header.hpp>

class game_context;
struct crow_harass_event;
struct temperature_parameters;

class Sparrow: public state_machine_mixin<Sparrow> {
    double get_bored_duration = 0;
    bool scared = false;
    event_listener_guard<crow_harass_event> harass_listener;
    orxFLOAT scare_per_second_coeff;
public:
    std::unique_ptr<temperature_parameters> temp_params;
    Sparrow();
    ORX_STATES(FlyToFeedingSpot, ExpectFood, Jump, FlyAway,Wait,Tweet,WaitForFood,Chew,Land)
    game_context * level = nullptr;
    SCROLL_BIND_CLASS("Sparrow")
    
	virtual ~Sparrow();
    virtual void Update(const orxCLOCK_INFO &_rstInfo);
    virtual void ExtOnCreate();
    void setLevel(game_context * level) { this->level = level;}
    virtual void OnDelete();
    virtual void OnNewAnim(const orxSTRING _zOldAnim, const orxSTRING _zNewAnim, orxBOOL _bCut);

    HappinessIndicator * indicator=nullptr;
    static int totalCount;
    double happiness = 0;
    double boredom = 0;
    bool getBored = true;
    struct Controller;
    static const transition_table<Sparrow> & getTransitionTable();
    void SetContext(ExtendedMonomorphic & context);
    void SetGetBored(bool getBored) {this->getBored = getBored;}
    void GetScared();
};

ACTION_LEAF(set_get_bored, &Sparrow::SetGetBored)

#endif /* GAMEOBJECTS_SPARROW_H_ */
