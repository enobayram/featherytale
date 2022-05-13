/*
 * Sparrow.cpp
 *
 *  Created on: Feb 26, 2015
 *      Author: seliz
 */

#include <initializer_list>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <tuple>
#include <random>
#include <array>
#include <set>

#include <gameObjects/Sparrow.h>
#include <pancarDemo.h>
#include <physicalElements/algorithms.hpp>
#include <util/probabilistic_choice.hpp>
#include <util/random.hpp>
#include <util/orxVECTORoperators.h>
#include <scroll_ext/config_utils.hpp>
#include <orx_utilities.h>
#include <util/orx_eigen.h>
#include <orxFSM/orxFSM.h>
#include "screens/GameLevel.h"
#include <util/event_utils.hpp>

INSTANTIATE_STATE_MACHINE(Sparrow);

int Sparrow::totalCount = 0;

using namespace std;
using SparrowState = state_t<Sparrow>;
using SparrowStateConstructor = SparrowState::constructor;

template <class STATE>
SparrowStateConstructor GetDefaultConstructor() {
    return [](Sparrow & self){return new STATE(self);};
}

void Sparrow::OnDelete() {
    totalCount--;
}
Sparrow::~Sparrow() {
	// TODO Auto-generated destructor stub
}

const char * tweetAnim = "Tweet";

void Sparrow::OnNewAnim(const orxSTRING _zOldAnim, const orxSTRING _zNewAnim, orxBOOL _bCut){
    if(std::strcmp(_zNewAnim,tweetAnim)==0) {
        AddSound("TweetSound");
    }
}

const float JUMPGRAVITY = 30;
orxVECTOR jumpPoint(orxVECTOR & origin, const orxVECTOR & v0, double t) {
    orxVECTOR result;
    result.fZ = origin.fZ;
    result.fX = origin.fX + v0.fX * t;
    result.fY = origin.fY + 0.5*JUMPGRAVITY*t*t + v0.fY * t;
    return result;
}

const char * mouthClosedAnim = "StayMouthClosed";

template <size_t N, class T>
constexpr size_t size_of_array(T(&)[N]) {return N;}

optional<marker> getMouth(Sparrow & sparrow, const char * frame_name) {
	return sparrow.getTranslatedMarker(frame_name,"MouthMarker");
}

optional<marker> getCurrentMouth(Sparrow & sparrow) {
    return getMouth(sparrow, sparrow.getCurrentFrameName());
}

const char *CatchPoses[] = {
        "ExpectFoodFrame0",
        "ExpectFoodFrame1",
        "ExpectFoodFrame2",
        "ExpectFoodFrame3"
};

typedef std::array<orxVECTOR, size_of_array(CatchPoses)> mouth_points_t;

mouth_points_t getMouthPoints(Sparrow & sparrow) {
    mouth_points_t result;
    for (size_t i=0; i<result.size(); i++){
        result[i] = getMouth(sparrow, CatchPoses[i])->center;
    };
    return result;
}

auto small_piece = [](Node & node){return node.getBonds().size()<=1;};

Node * get_flake_in_mouth(Sparrow & sparrow) {
	optional<marker> mouth = getMouth(sparrow,sparrow.getCurrentFrameName());
	if(!mouth) return nullptr;
    Vector2d points[] = {toEigen2d(mouth->center)};
    Node * closest = nearestNode(points, *sparrow.level->world,
            mouth->radius,small_piece).first;
	return closest;
}

double mouth_radius(Sparrow & sparrow) {
    return sparrow.getTranslatedMarker("ExpectFoodFrame0", "MouthMarker")->radius;
}

bool has_food_nearby(Sparrow & sparrow) {
    auto points = toEigen2d(getMouthPoints(sparrow));
    Node * closest = nearestNode(points, *sparrow.level->world, mouth_radius(sparrow)*2, small_piece).first;
    return closest != nullptr;
}

std::pair<Node *, unsigned> getBestCatch(Sparrow & sparrow, const mouth_points_t & mouth_points) {
    auto eigen_points = toEigen2d(mouth_points);
    auto closest = nearestNode(eigen_points, *sparrow.level->world, mouth_radius(sparrow)*2, small_piece);
    return std::make_pair(closest.first, closest.second - &eigen_points[0]);
}

Node * getFoodBehindBird(Sparrow & sparrow) {
    auto pos = sparrow.GetPosition();
    Vector2d posCont[] = {{pos.fX,pos.fY}};
    orxVECTOR size;
    orxObject_GetSize(sparrow.GetOrxObject(), &size);
    auto scale = sparrow.GetScale();
    auto closest = nearestNode(posCont,*sparrow.level->world, scale.fX * size.fX/2, small_piece);
    return closest.first;
}

void consume(game_context * level, Node * flake) {
	if(!level->world->getForcesOnNode(flake).empty()) {
		level->addScore("handfeed", ::GetValue<orxU32>("Balance", "ScoreHandFeeding"), toOrxVector(flake->pos));
	} else {
		auto score = orxU32(flake->vel.norm()/::GetValue<orxFLOAT>("Balance", "ScoreVelocityDenominator"));
		auto max = GetValue<orxU32>("Balance", "ScoreMaximumFromVelocity");
		if(score) level->addScore("basket", min(score,max),toOrxVector(flake->pos));
		else level->addScore("justeat", 1, toOrxVector(flake->pos));
	}
	level->world->removeNode(flake);
	level->fire_event<eat_event>();
}



ORX_STATE(Sparrow,ExpectFood) {
    ExpectFood(Sparrow &) {}
    mouth_points_t mouth_points;
    std::pair<Node *, unsigned> best_catch;
    string catch_anim;
    constructor Update(Sparrow & self, const orxCLOCK_INFO &_rstInfo) {
        mouth_points = getMouthPoints(self);
        best_catch = getBestCatch(self, mouth_points);

        if(best_catch.first) {
            catch_anim = "ExpectFood"+to_string(best_catch.second);
            orxObject_SetTargetAnim(self.GetOrxObject(), catch_anim.c_str());
        }
        return {};
    }
    bool IsInClosestPose(Sparrow & self) {
        return self.getCurrentAnimName() == catch_anim;
    }
};

ORX_STATE(Sparrow,Jump) {
    orxFLOAT phase = 0;
    orxFLOAT targetPhase = 0;
    orxVECTOR jumpVelocity;
    orxVECTOR transitionOrigin;
    Jump(Sparrow & self, orxVECTOR target) {
        auto pos = transitionOrigin = self.GetPosition();
        orxVECTOR delta = target - orxVECTOR{pos.fX, pos.fY,0};
        targetPhase = pow(4*(squaredNorm(delta)/(JUMPGRAVITY*JUMPGRAVITY)),0.25);
        jumpVelocity =  {delta.fX/targetPhase, -JUMPGRAVITY* targetPhase/2 + delta.fY/targetPhase,0};
        phase = 0;
        self.SetAnim("Jump", true);
        self.AddSound("SparrowJumpSound");
    }

    constructor Update(Sparrow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(targetPhase, phase+_rstInfo.fDT);
        self.SetPosition(jumpPoint(transitionOrigin, jumpVelocity, phase));
        if(phase == targetPhase) self.scared = false;
        return {};
    }
};

ORX_STATE(Sparrow,FlyToFeedingSpot) {
    orxFLOAT phase = 0;
    orxVECTOR transitionOrigin;
    orxVECTOR landingSpot;
    FlyToFeedingSpot(Sparrow & self): FlyToFeedingSpot(self, self.GetValue<orxVECTOR>("LandingSpot")) {}
    FlyToFeedingSpot(Sparrow & self, const orxVECTOR & landingSpot_): landingSpot(landingSpot_) {
        self.SetAnim("Fly");
        self.GetPosition(transitionOrigin);
    }
    constructor Update(Sparrow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(1.0f, phase+_rstInfo.fDT);
        self.SetPosition(linearInterpolate(transitionOrigin, landingSpot, phase));
        return {};
    }
};

ORX_STATE(Sparrow,FlyAway) {
    orxFLOAT phase = 0;
    orxVECTOR transitionOrigin;
    orxVECTOR transitionTarget;
    FlyAway(Sparrow & self) {
        orxObject_SetCurrentAnim(self.GetOrxObject(), "Fly");
        CreateSoundObject("SparrowFlyAwaySound", self.GetPosition());
        phase=0;
        self.GetPosition(transitionOrigin);
        transitionTarget = self.GetValue<orxVECTOR>("LeaveTowards");
        if(self.indicator) self.level->changeHappiness((self.happiness-0.5)/10);
        self.level->fire_event<sparrow_leave_event>(&self);
    }
    constructor Update(Sparrow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(1.0f, phase+_rstInfo.fDT);
        self.SetPosition(linearInterpolate(transitionOrigin, transitionTarget, phase));
        if(phase == 1) self.SetLifeTime(0);
        return {};
    }
};

ORX_STATE(Sparrow, Wait) {
    Wait(Sparrow & self) {
        orxObject_SetTargetAnim(self.GetOrxObject(), "StayMouthClosed");
    }
};

ORX_STATE(Sparrow,Tweet) {
    Tweet(Sparrow & self) {
        orxObject_SetTargetAnim(self.GetOrxObject(), tweetAnim);
    }
};

ORX_STATE(Sparrow,WaitForFood) {
    WaitForFood(Sparrow & self) {
        orxObject_SetTargetAnim(self.GetOrxObject(), "StayMouthOpen");
    }
};


ORX_STATE(Sparrow,Chew) {
    Chew(Sparrow & self) {
    }
    constructor Update(Sparrow & self, const orxCLOCK_INFO &_rstInfo) {
        if(orxObject_IsCurrentAnim(self.GetOrxObject(), "Chew")) self.SetAnim("StayMouthClosed");
        return {};
    }
};

ORX_STATE(Sparrow,Land) {
    Land(Sparrow & self) {
        orxObject_SetTargetAnim(self.GetOrxObject(), mouthClosedAnim);
        self.AddSound("SparrowLandSound");
    }
};

orxVECTOR findEscapePoint(Sparrow & self) {
    auto curpos = self.GetPosition();
    while(true) {
        auto newpos = self.GetValue<orxVECTOR>("LandingSpot");
        if(squaredNorm(newpos-curpos) > 36) return newpos;
    }
    return {};
}

struct Sparrow::Controller: public controller_mixin<Sparrow, Sparrow::Controller> {
    using controller_mixin::decide;
    orxFLOAT minX, maxX, correctionPeriod, openMouthPeriod, stayMouthOpenPeriod;
    orxFLOAT tweetPeriod, jumpToFoodBehindPeriod, foodNoticePeriod;
    Controller(const char * section):controller_mixin(section){
        ConfigSectionGuard guard(section);
        minX = orxConfig_GetFloat("AllowedMinX");
        maxX = orxConfig_GetFloat("AllowedMaxX");
        correctionPeriod = orxConfig_GetFloat("CorrectionPeriod");
        openMouthPeriod = orxConfig_GetFloat("OpenMouthPeriod");
        stayMouthOpenPeriod = orxConfig_GetFloat("StayMouthOpenPeriod");
        tweetPeriod = orxConfig_GetFloat("TweetPeriod");
        jumpToFoodBehindPeriod = orxConfig_GetFloat("JumpToFoodBehindPeriod");
        foodNoticePeriod = orxConfig_GetFloat("FoodNoticePeriod");
    }
    template <class SrcState>
    evaluation<SrcState> escape_if_scared(Sparrow & self, SrcState & curState) {
        return from(curState) >>
                guarded<FlyAway>(self.happiness<=0 && self.scared) >>
                guarded<Jump>([&]() {
                    if(self.scared) return optional<orxVECTOR>(findEscapePoint(self));
                    else return optional<orxVECTOR>{};
                });
    }

    optional<orxVECTOR> calculateCorrectiveJump(Sparrow & self) {
        optional<orxVECTOR> result;
        if(self.GetPosition().fX < minX) {
            result = self.GetPosition();
            result->fX += GetValue<orxFLOAT>("CorrectionJump");
        }
        if(self.GetPosition().fX > maxX) {
            result = self.GetPosition();
            result->fX -= GetValue<orxFLOAT>("CorrectionJump");
        }
        return result;
    }

    decision decide(Sparrow & self, Wait & curState, const orxCLOCK_INFO &_rstInfo) {
        auto poisson_event = poisson_event_generator(_rstInfo.fDT);
        optional<orxVECTOR> corrective_jump;
        if(poisson_event(correctionPeriod)) corrective_jump = calculateCorrectiveJump(self);
        return escape_if_scared(self, curState) >>
            guarded<FlyAway>(self.boredom >= 1 || self.happiness >= 1) >>
            guarded<Jump>(corrective_jump) >>
            guarded<ExpectFood>([&]{return has_food_nearby(self);}) >>
            guarded<Tweet>( poisson_event(tweetPeriod)) >>
            guarded<WaitForFood>(poisson_event(openMouthPeriod)) >>
            guarded<Jump>([&] {
                if(poisson_event(jumpToFoodBehindPeriod)) {
                    Node * flake_behind = getFoodBehindBird(self);
                    if(flake_behind!=nullptr) {
                        return optional<orxVECTOR>({
                            float(flake_behind->pos.x()) - self.getScaledMarker("ExpectFoodFrame0","MouthMarker")->center.fX
                            , self.GetPosition().fY, 0});
                    }
                }
                return optional<orxVECTOR>{};
            })
        ;
    }

    decision decide(Sparrow & self, WaitForFood & curState, const orxCLOCK_INFO &_rstInfo) {
        auto poisson_event = poisson_event_generator(_rstInfo.fDT);
        optional<orxVECTOR> corrective_jump;
        if(poisson_event(correctionPeriod)) corrective_jump = calculateCorrectiveJump(self);
        return escape_if_scared(self, curState) >>
            guarded<Chew>( true ) >>
            guarded<Jump>(corrective_jump) >>
            guarded<Wait>(poisson_event(stayMouthOpenPeriod)) >>
            guarded<FlyAway>(self.boredom >= 1) >>
            guarded<ExpectFood>(poisson_event(foodNoticePeriod) && has_food_nearby(self))
        ;
    }

    decision decide(Sparrow & self, ExpectFood & st, const orxCLOCK_INFO &_rstInfo ) {
        return escape_if_scared(self, st) >>
            guarded<Chew>(true) >>
            guarded<Wait>(st.best_catch.first == nullptr) >>
            guarded<Jump>([&]{
                optional<marker> current_mouth = getCurrentMouth(self);
                if(st.best_catch.first == nullptr || !current_mouth || !st.IsInClosestPose(self)) return optional<orxVECTOR>{};
                return optional<orxVECTOR> {
                    {self.GetPosition().fX+(float(st.best_catch.first->pos.x())-current_mouth->center.fX),self.GetPosition().fY}
                };
            })
        ;
    }

    decision decide(Sparrow & self, Land & st, const orxCLOCK_INFO &_rstInfo ) {return escape_if_scared(self, st) >> guarded<Wait>(true);}
    decision decide(Sparrow &, Chew & st, const orxCLOCK_INFO &_rstInfo ) {return from(st) >> guarded<Wait>(true);}
    decision decide(Sparrow &, Tweet & st, const orxCLOCK_INFO &_rstInfo ) {return from(st) >> guarded<Wait>(true);}
    decision decide(Sparrow &, FlyToFeedingSpot & st, const orxCLOCK_INFO &_rstInfo ) {return from(st) >> guarded<Land>(true);}
    decision decide(Sparrow & self, Jump & st, const orxCLOCK_INFO &_rstInfo ) {
        return from(st) >>
            guarded<FlyAway>(self.happiness<=0 && self.scared) >>
            guarded<ExpectFood>(true)
        ;
    }
};
REGISTER_CONTROLLER(Sparrow::Controller, "sparrow")

bool has_flake_in_mouth(Sparrow & self) {
    return get_flake_in_mouth(self)!=nullptr;
}

void consume_flake_in_mouth(Sparrow & self) {
    Node *flake = get_flake_in_mouth(self);
    orxASSERT(flake);
    if(flake) {
        consume(self.level, flake);
        auto chewAnim = GetValue<const char *>(self.getCurrentFrameName(), "ChewAnim");
        orxObject_SetCurrentAnim(self.GetOrxObject(), chewAnim);
        self.AddSound("SparrowEatSound");
    }
}

std::function<bool(Sparrow&)> AnimIs(const char * name) {
    return [name](Sparrow & self){return self.IsAnim(name,true);};
}

std::function<void(Sparrow&)> IncreaseHappiness(orxFLOAT delta) {
    return [delta](Sparrow &self) {self.happiness = min(1.0,self.happiness + delta);};
}

bool JumpComplete(Sparrow::Jump & s) {return s.phase == s.targetPhase;}

action_t<Sparrow> SetAnim(const char * name, bool current) {
    return [=](Sparrow & self) {self.SetAnim(name,current);};
}

const transition_table<Sparrow> & Sparrow::getTransitionTable() {
    static transition_table<Sparrow> table{
        any_time<Wait, FlyAway>(),
        any_time<Wait, ExpectFood>(),
        any_time<Wait, Tweet>(),
        any_time<Wait, Jump>(),
        any_time<Wait, WaitForFood>(),

        any_time<WaitForFood, Jump>(),
        any_time<WaitForFood, Wait>(),
        any_time<WaitForFood, ExpectFood>(),
        any_time<WaitForFood, FlyAway>(),
        only_if<WaitForFood, Chew>(has_flake_in_mouth, consume_flake_in_mouth),

        only_if<Land, Wait>(AnimIs(mouthClosedAnim)),
        any_time<Land, Jump>(),

        only_if<Chew,Wait>(AnimIs("StayMouthClosed"), IncreaseHappiness(0.2)),

        only_if<Tweet, Wait>(AnimIs(mouthClosedAnim)),

        only_if<FlyToFeedingSpot, Land>(TransitionComplete<FlyToFeedingSpot>),

        only_if<Jump, ExpectFood>(JumpComplete, ::SetAnim("LeanToFood2", true)),
        any_time<Jump, FlyAway>(),

        any_time<ExpectFood, Wait>(),
        any_time<ExpectFood, Jump>(),
        only_if<ExpectFood, Chew>(has_flake_in_mouth, consume_flake_in_mouth)
    };
    return table;
}

struct temperature_parameters {
    float optimum_temp;
    float tolerance;
    float penalty_per_sec_celcius;
    float init_penalty_per_celcius;
    float heating_bonus_per_sec;
    float max_healing_happiness;
    float max_temperature_unhappiness;
};

orxFLOAT getTemperatureDiscomfort(orxFLOAT current_temp, temperature_parameters & params) {
    orxFLOAT optim = params.optimum_temp;
    orxFLOAT tolerance = params.tolerance;
    return std::max(std::abs(current_temp-optim)-tolerance, 0.0f);

}

void Sparrow::Update(const orxCLOCK_INFO &_rstInfo) {
    if(getBored) {
        boredom = min(1.0,boredom + _rstInfo.fDT / get_bored_duration);
    }

    orxFLOAT frame_temp = level->query<temperature_status>(temperature_status::ONFRAME);
    auto discomfort = getTemperatureDiscomfort(frame_temp, *temp_params);
    if(happiness > temp_params->max_temperature_unhappiness) {
        happiness = happiness- (temp_params->penalty_per_sec_celcius * discomfort * _rstInfo.fDT);
    }

    orxFLOAT outside_temp = level->query<temperature_status>(temperature_status::OUTSIDE);
    auto outside_discomfort = getTemperatureDiscomfort(outside_temp, *temp_params);
    if(outside_discomfort != 0 && discomfort == 0) {
        if(happiness < temp_params->max_healing_happiness) {
            happiness += temp_params->heating_bonus_per_sec * _rstInfo.fDT;
        }
    }

    auto scariness = level->query<scariness_status>();
    if(scariness) {
        happiness -= scariness * scare_per_second_coeff * _rstInfo.fDT;
        scared = true;
    }

    state_machine_mixin::Update(_rstInfo);

    if(indicator) {
        indicator->setHappiness(happiness);
        indicator->setPhase(boredom);
    }
}

void Sparrow::ExtOnCreate() {
    totalCount++;
    state_machine_mixin::ExtOnCreate();
    indicator = FindOwnedChild<HappinessIndicator>();
    happiness = orxConfig_GetFloat("InitHappiness");
    if(indicator) {
        indicator->setHappiness(happiness);
        indicator->setPhase(boredom);
    }
    getBored = GetValue<bool>("GetsBored", false);
    get_bored_duration = GetValue<orxFLOAT>("GetBoredDuration", false);
    scare_per_second_coeff = orxConfig_GetFloat("ScarePerSecondCoeff");
    temperature_parameters params;
    params.optimum_temp = orxConfig_GetFloat("TempOptim");
    params.tolerance = orxConfig_GetFloat("TempTol");
    params.penalty_per_sec_celcius = orxConfig_GetFloat("TempPPSC");
    params.init_penalty_per_celcius = orxConfig_GetFloat("TempIPPC");
    params.heating_bonus_per_sec = orxConfig_GetFloat("TempHBPS");
    params.max_healing_happiness = orxConfig_GetFloat("TempMHH");
    params.max_temperature_unhappiness = orxConfig_GetFloat("TempMTU");
    temp_params.reset(new temperature_parameters(params));
}

orxFLOAT getTemperatureAdjustment(Sparrow & sparrow) {
    ConfigSectionGuard guard(sparrow.GetModelName());
    orxFLOAT current_temp = sparrow.level->query<temperature_status>(temperature_status::OUTSIDE);
    orxFLOAT temperature_discomfort = getTemperatureDiscomfort(current_temp, *sparrow.temp_params);
    return -temperature_discomfort * sparrow.temp_params->init_penalty_per_celcius;
}

void Sparrow::SetContext(ExtendedMonomorphic& context) {
    setLevel(context.GetAspect<game_context>());

    harass_listener.register_handler(*level, get_weak_accessor<event_dispatcher>(context.GetOrxObject()),
            [this](orxVECTOR pos){
        if(squaredNorm(GetPosition() - pos) < 9) GetScared();
    });
    happiness += getTemperatureAdjustment(*this);
}

void Sparrow::GetScared() {
    scared = true;
    happiness = happiness - 0.3;
}

Sparrow::Sparrow() {
}
