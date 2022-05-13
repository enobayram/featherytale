/*
 * Crow.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: seliz
 */

#include <random>
#include <cmath>
#include <cstring>

#include <orx_utilities.h>
#include <util/probabilistic_choice.hpp>
#include <scroll_ext/InMemoryTexture.h>
#include "Crow.h"
#include "scroll_ext/config_utils.hpp"
#include "pancarDemo.h"
#include "util/random.hpp"
#include <orxFSM/orxFSM.h>
#include <util/event_utils.hpp>
#include <util/orxVECTORoperators.h>
#include "Sparrow.h"
#include "screens/GameLevel.h"

INSTANTIATE_STATE_MACHINE(Crow);

template<class T, int S> int array_size(T(&)[S]) { return S;}
const char * stay_animations[] = {"Stay", "Stay2"};

double interp(double b, double e, double phase) {
    return b*(1-phase) + e*phase;
}

struct crow_punched {
    using signature = void(orxVECTOR);
};

bool is_scary(Crow * crow) {
    return crow->GetValue<bool>("Scary");
}

orxFLOAT scariness_handler() {
    return 1;
}

void caw(Crow & self) {
    self.AddSound(self.GetValue<const char *>("CawSound"));
}

template <class STATE>
struct ScaryState: public state_base<Crow, STATE> {
    event_listener_guard<scariness_status> scariness_guard;
    ScaryState(Crow & self) {
        if(is_scary(&self)) {
            scariness_guard.register_handler(
                    *self.level,
                    get_weak_accessor<GameLevel>(self.level->GetOrxObject()),
                    scariness_handler);
        }
    }
};

ORX_STATE(Crow, Stay) {
    Stay(Crow & self) {
        self.SetTaggedAnim("Stay");
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        auto pose_period = ::GetValue<orxFLOAT>("Balance", "CrowPosePeriod");
        if(poisson_event_generator(_rstInfo.fDT)(pose_period)) {
            std::uniform_int_distribution<> dist(0,array_size(stay_animations)-1);
            self.SetTaggedAnim(stay_animations[dist(gen)]);
        }
        return {};
    }
};

ORX_STATE(Crow, FlyAtDistance) {
    orxFLOAT phase = 0;
    orxVECTOR origin;
    orxVECTOR target;
    using constructor_arg = orxVECTOR;
    FlyAtDistance(Crow & self, orxVECTOR target):origin(self.GetPosition()), target(target) {
        self.SetTaggedAnim("Flap");
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(phase+_rstInfo.fDT, 1.0f);
        orxVECTOR newpos;
        newpos.fX = interp(origin.fX, target.fX,phase);
        newpos.fY = interp(origin.fY, target.fY,phase);
        newpos.fZ = interp(origin.fZ, target.fZ,phase);
        self.SetPosition(newpos);
        return {};
    }
};

const char * diveAnim = "Dive";
const double drop = -3;

void divePos(Crow & crow, const orxVECTOR & org, orxFLOAT orgHeight, const orxVECTOR & trg, orxFLOAT trgHeight, orxFLOAT phase) {
    orxVECTOR pos;
    pos.fX = interp(org.fX , trg.fX , phase);
    pos.fY = 4*drop*phase*phase + (trg.fY-4*drop-org.fY)*phase + org.fY;
    pos.fZ = interp(org.fZ , trg.fZ , phase);
    crow.SetPosition(pos);
    auto height = interp(orgHeight, trgHeight, phase);
    crow.SetHeight(height);
}

ORX_STATE(Crow, Dive) {
    orxVECTOR origin;
    orxFLOAT originHeight;
    orxVECTOR target;
    orxFLOAT targetHeight;
    orxFLOAT phase = 0;
    using constructor_arg = orxVECTOR;
    Dive(Crow & self, orxVECTOR target):origin(self.GetPosition()), originHeight(self.GetSize().fY) {
        target.fZ = self.GetValue<orxFLOAT>("HoverDepth");
        this->target = target;
        targetHeight = originHeight*1.5;
        self.SetTaggedAnim(diveAnim);
        self.poked = {};
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(phase+_rstInfo.fDT/ ::GetValue<orxFLOAT>("Balance", "CrowDiveDuration"), 1.0f);
        divePos(self, origin, originHeight, target, targetHeight,phase);
        if(self.GetPosition().fZ <= self.GetValue<orxFLOAT>("PokeDepth")) self.SetTaggedAnim("Flap");
        return {};
    }
};

ORX_STATE(Crow, Hover, ScaryState<Hover>) {
    Hover(Crow & self):ScaryState(self) {
        self.SetTaggedAnim("Flap");
    }
};

ORX_STATE(Crow, Stunned) {
    orxFLOAT phase = 0;
    Stunned(Crow & self) {
        self.SetTaggedAnim("Stunned", true);
        self.AddSound("CrowStunnedSound");
        orxObject_AddUniqueFX(self.GetOrxObject(),"CrowStunFX");
        auto debris = self.CreateChild("CrowStunDebris", false, false);
        orxVECTOR crow_scale = self.GetScale(), crow_pos = self.GetPosition();
        orxObject_SetScale(debris, &crow_scale);
        orxObject_SetPosition(debris, &crow_pos);
        self.level->fire_event<crow_beaten_event>(&self);
        SendSignal(&self, "BEATEN");
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(phase+_rstInfo.fDT*2, 1.0f);
        return {};
    }
};

ORX_STATE(Crow, Harassing) {
    orxFLOAT phase = 0;
    Harassing(Crow & self) {
        self.SetTaggedAnim("Dive");
        auto exclamation = self.CreateChild("CrowHarassEffect");
        orxObject_Detach(exclamation);
        caw(self);
        self.level->fire_event<crow_harass_event>(self.GetPosition());
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(phase+_rstInfo.fDT*2, 1.0f);
        return {};
    }
};

ORX_STATE(Crow, FlyOnScreen, ScaryState<FlyOnScreen>) {
    orxFLOAT phase = 0;
    orxVECTOR origin;
    orxVECTOR target;
    FlyOnScreen(Crow & self, orxVECTOR target):ScaryState(self), origin(self.GetPosition()), target(target) {
        self.SetTaggedAnim("Flap");
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(phase+_rstInfo.fDT/::GetValue<orxFLOAT>("Balance", "CrowFlyToBaitDuration"), 1.0f);
        orxVECTOR newpos;
        newpos.fX = interp(origin.fX, target.fX,phase);
        newpos.fY = interp(origin.fY, target.fY,phase);
        newpos.fZ = interp(origin.fZ, target.fZ,phase);
        self.SetPosition(newpos);
        return {};
    }
};

orxOBJECT * createSnatchedFlakes(Crow & crow, orxU32 flake_count) {
    ConfigSectionGuard guard(crow.GetModelName());
    orxU32 max_flakes = orxConfig_GetListCounter("SnatchedFlakes");
    auto flake_name = orxConfig_GetListString("SnatchedFlakes", min(max_flakes, flake_count)-1);
    orxOBJECT * result = crow.CreateChild(flake_name, false, false);
    auto crow_pos = crow.GetPosition();
    crow_pos.fZ += -0.001;
    orxObject_SetPosition(result, &crow_pos);
    orxObject_Attach(result, crow.GetOrxObject());
    return result;
}

ORX_STATE(Crow, Home) {
    orxFLOAT phase=0;
    orxVECTOR origin;
    orxFLOAT originHeight;
    orxVECTOR target;
    orxFLOAT targetHeight;
    orxOBJECT * snatched_pieces = orxNULL;
    using constructor_arg = orxU32;
    Home(Crow & self, orxU32 flake_count) {
        self.GetPosition(origin);
        target = self.GetValue<orxVECTOR>("StayPositions");
        originHeight = self.GetSize().fY;
        targetHeight = self.GetValue<orxFLOAT>("Height");
        if(flake_count == 0) {
            self.SetTaggedAnim("Fly");
        } else {
            self.SetTaggedAnim("Snatch");
            snatched_pieces = createSnatchedFlakes(self, flake_count);
        }
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(phase+_rstInfo.fDT/3, 1.0f);
        divePos(self, origin, originHeight, target, targetHeight,phase);
        return {};
    }
    void ExitState(Crow & self) {
        if(snatched_pieces) orxObject_SetLifeTime(snatched_pieces, 0);
    }
};

ORX_STATE(Crow, Enter) {
    orxFLOAT phase = 0;
    orxVECTOR transitionOrigin;
    orxVECTOR landingSpot;
    Enter(Crow & self): Enter(self, self.GetValue<orxVECTOR>("StayPositions")) {}
    Enter(Crow & self, const orxVECTOR & landingSpot_): landingSpot(landingSpot_) {
        self.SetTaggedAnim("Dive",true);
        self.GetPosition(transitionOrigin);
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(1.0f, phase+_rstInfo.fDT);
        self.SetPosition(linearInterpolate(transitionOrigin, landingSpot, phase));
        return {};
    }
};

ORX_STATE(Crow, Exit) {
    orxFLOAT phase = 0;
    orxVECTOR transitionOrigin;
    orxVECTOR transitionTarget;
    using constructor_arg = orxU32;
    Exit(Crow & self, orxU32 flake_count) {
        if(flake_count) {
            self.SetTaggedAnim("Snatch", true);
            transitionTarget = self.GetValue<orxVECTOR>("ExitPositionWithFood");
            createSnatchedFlakes(self, flake_count);
        } else {
            self.SetTaggedAnim("Flap", true);
            transitionTarget = self.GetValue<orxVECTOR>("ExitPosition");
        }
        self.GetPosition(transitionOrigin);
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(1.0f, phase+_rstInfo.fDT);
        self.SetPosition(linearInterpolate(transitionOrigin, transitionTarget, phase));
        if(phase == 1) self.SetLifeTime(0);
        return {};
    }
};

InMemoryTexture crow_father_partmap;

void setRandomGuardPose(Crow & crow) {
    orxU32 numOfPoses = crow.GetValue<orxU32>("NumOfGuardPoses");
    orxU32 poseNo = std::uniform_int_distribution<orxU32>(1,numOfPoses)(gen);
    auto pose = ("Guard" + to_string(poseNo));
    crow.SetTaggedAnim(pose.c_str(), true);
}

enum hit_code {
    MISS, HEAD, BEAK, BELLY, FEET, BLOCK=-1
};

hit_code colorToHitCode(orxU32 col) {
    switch(col) {
    case 0xFF0000FF: return BLOCK;
    case 0xFF00FF00: return HEAD;
    case 0xFF7DFF00: return BEAK;
    case 0xFFFF0000: return BELLY;
    case 0xFFFF7D00: return FEET;
    default: return MISS;
    }
}

void guardPart(Crow & crow, uint partCode) {
    auto table = crow.GetValue<const char *>("GuardTable");
    orxU32 frameNum = GetValue<orxU32>(table, to_string(partCode).c_str());
    crow.SetTaggedAnim((("Guard") + to_string(frameNum)).c_str(), true);
}

ORX_STATE(Crow, Combat) {
    orxFLOAT last_hit = 0;
    orxU32 damage = 1;
    enum {
        MISSED, BLOCKED, WAITING
    } substate = WAITING;
    bool missed = false;
    bool blocked = false;
    void punch(Crow & self, orxVECTOR pokeInBody) {
        self.fire_event<crow_punched>(pokeInBody);
        auto pos = self.GetPosition();
        auto fightOrigin = self.GetValue<orxVECTOR>("FightOrigin");
        if(pos.fX<fightOrigin.fX  && pos.fY< fightOrigin.fY) self.AddFX("CrowPunchRecoilFXTopLeft");
        if(pos.fX>=fightOrigin.fX && pos.fY< fightOrigin.fY) self.AddFX("CrowPunchRecoilFXTopRight");
        if(pos.fX<fightOrigin.fX  && pos.fY>=fightOrigin.fY) self.AddFX("CrowPunchRecoilFXBottomLeft");
        if(pos.fX>=fightOrigin.fX && pos.fY>=fightOrigin.fY) self.AddFX("CrowPunchRecoilFXBottomRight");
    }
    void ExitState(Crow & self) {
        self.RemoveFX("CrowPunchRecoilFXTopLeft");
        self.RemoveFX("CrowPunchRecoilFXTopRight");
        self.RemoveFX("CrowPunchRecoilFXBottomLeft");
        self.RemoveFX("CrowPunchRecoilFXBottomRight");
    }
    Combat(Crow & self) {
        setRandomGuardPose(self);
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        if(self.poked) {
            ConfigSectionGuard guard(self.getCurrentFrameName());
            orxVECTOR corner, pivot;
            orxConfig_GetVector("Pivot", &pivot);
            orxConfig_GetVector("TextureCorner", &corner);
            auto origin = pivot + corner;
            auto & pokep = self.poked.val;
            auto pokeOnTexture = pokep + origin;
            hit_code hit_code;
            if(crow_father_partmap.inTexture(pokeOnTexture.fX, pokeOnTexture.fY)) {
                hit_code = colorToHitCode(crow_father_partmap.get(pokeOnTexture.fX, pokeOnTexture.fY));
            } else hit_code = MISS;
            switch(hit_code) {
            case MISS:
                SendSignal(&self,"MISSED");
                substate = MISSED;
                break;
            case BLOCK:
                SendSignal(&self, "BLOCKED");
                substate = BLOCKED;
                break;
            default:
                damage += 1;
                punch(self,pokep);
                guardPart(self, hit_code);
                break;
            }
            self.poked = {};
            last_hit = 0;
        } else {
            substate = WAITING;
            last_hit += _rstInfo.fDT;
        }
        return {};
    }
};

ORX_STATE(Crow, Bite) {
    orxFLOAT phase = 0;
    Bite(Crow & self) {
        self.SetTaggedAnim("Bite", true);
        self.AddSound("CrowBiteSound");
        orxObject_Detach(self.CreateChild("CrowBiteVisual"));
        self.level->changeHappiness(-self.GetValue<orxFLOAT>("BiteHappinessPenalty"));
        self.level->fire_event<bite_event>();
        SendSignal(&self,"BITTEN");
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        phase = std::min(phase+_rstInfo.fDT*3.4f, 1.0f);
        return {};
    }
};

ORX_STATE(Crow, RaiseWings) {
    RaiseWings(Crow & self) {
        self.SetTaggedAnim("RaiseWings");
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        return {};
    }
};

ORX_STATE(Crow, Order) {
    Order(Crow & self) {
        self.SetTaggedAnim("Order");
    }
    constructor Update(Crow & self, const orxCLOCK_INFO &_rstInfo) {
        return {};
    }
};


void Crow::OnNewAnim(const orxSTRING _zOldAnim, const orxSTRING _zNewAnim, orxBOOL _bCut){
    if(std::strcmp(_zNewAnim,GetAnimName(diveAnim))==0 && std::strcmp(_zNewAnim,_zOldAnim) != 0) {
        caw(*this);
    }
}

orxU32 snatchFood(Crow & crow) {
    orxVECTOR pos;
    crow.GetPosition(pos);
    auto nodes = crow.level->world->nodesAt({pos.fX,pos.fY}, 1);
    orxU32 result = nodes.size();
    for(auto node:nodes) crow.level->world->removeNode(node);
    crow.level->changeHappiness(-0.02*nodes.size());
    crow.level->fire_event<snatch_event>();
    return result;
}

Node * getRandomNode(World & world) {
    if(world.getNodes().empty()) return NULL;
    int nodesSize = world.getNodes().size();
    int nodeIndex = std::uniform_int_distribution<>(0,nodesSize-1)(gen);
    for(auto & node: world.getNodes()) {
        if(nodeIndex--) continue;
        else return &node;
    }
    return NULL;
}


gesture * Crow::clicked(trail & t) {
    if(t.traces.size()==1 && GetPosition().fZ<=GetValue<orxFLOAT>("PokeDepth")) {
        auto trace = t.traces.back();
        poke(toObjectFrame(GetOrxObject(), {float(trace.x), float(trace.y), 0}));
        return new null_gesture();
    } else return nullptr;
}

void Crow::poke(orxVECTOR pokepos) {
	poked = pokepos;
}

void Crow::Update(const orxCLOCK_INFO &_rstInfo) {

	state_machine_mixin::Update(_rstInfo);

	orxVECTOR pos;
	orxCOLOR params;
	GetColor(params);
	params.fAlpha = std::max(GetPosition(pos).fZ, 0.0f);
	SetColor(params,false);
	for ( orxSTRUCTURE * child = orxObject_GetChild(GetOrxObject());
	      child;
	      child = orxObject_GetSibling(orxOBJECT(child)) )
	{
	    orxObject_SetColor(orxOBJECT(child), &params);
	}

}

Crow::Crow() {
    register_handler<crow_punched>([&](orxVECTOR punch_pos){
        AddSound("CrowPunchSound");
        auto visual = CreateChild("CrowPunchVisual");
        orxObject_SetPosition(visual, &punch_pos);
        orxObject_Detach(visual);
        SendSignal(this, "PUNCHED");
    });
}

const transition_table<Crow>& Crow::getTransitionTable() {
    static transition_table<Crow> table {
        only_if<Enter, Stay>(TransitionComplete<Enter>),

        any_time<Stay, Dive>(),
        any_time<FlyAtDistance, Dive>(),

        only_if<Dive, Hover>(TransitionComplete<Dive>),
        any_time<Dive, Home>(),

        only_if<Home, Stay>(TransitionComplete<Home>),
        only_if<Home, FlyAtDistance>(TransitionComplete<Home>),

        any_time<Hover, FlyOnScreen>(),
        any_time<Hover, Home>(),

        any_time<Hover, Combat>(),
        any_time<FlyOnScreen, Combat>(),
        any_time<Harassing, Combat>(),
        any_time<Combat, Home>(),
        any_time<Combat, Hover>(),
        any_time<Combat, FlyOnScreen>(),
        any_time<Combat, Stunned>(),

        any_time<Combat, Bite>(),
        only_if<Bite, Hover>(TransitionComplete<Bite>),
        only_if<Bite, FlyOnScreen>(TransitionComplete<Bite>),
        only_if<Bite, Home>(TransitionComplete<Bite>),

        any_time<FlyOnScreen, Home>(),
        any_time<FlyOnScreen, Hover>(),

        any_time<Dive, Stunned>(),
        any_time<Hover, Stunned>(),
        any_time<FlyOnScreen, Stunned>(),
        any_time<Harassing, Stunned>(),

        only_if<Stunned, Home>(TransitionComplete<Stunned>),

        only_if<Dive, Harassing>(TransitionComplete<Dive>),
        any_time<Hover, Harassing>(),
        any_time<FlyOnScreen, Harassing>(),

        only_if<Harassing, Home>(TransitionComplete<Harassing>),
        only_if<Harassing, Hover>(TransitionComplete<Harassing>),

        any_time<Enter, Exit>(),
        any_time<Stay, Exit>(),
        any_time<FlyAtDistance, Exit>(),
        any_time<Dive, Exit>(),
        any_time<Hover, Exit>(),
        any_time<Home, Exit>(),
        any_time<FlyOnScreen, Exit>(),
        any_time<Combat, Exit>(),
        only_if<Stunned, Exit>(TransitionComplete<Stunned>),
        any_time<RaiseWings, Exit>(),
        any_time<Order, Exit>(),

        any_time<Stay, RaiseWings>(),
        any_time<RaiseWings, Stay>(),
        any_time<RaiseWings, Order>(),
        any_time<Order, Stay>(),

    };
    return table;
}

struct Crow::Controller: public controller_mixin<Crow, Crow::Controller> {
    using controller_mixin::decide;

    bool can_harass = false;
    bool can_snatch = false;
    bool can_fight = false;
    bool temporary = false;
    orxFLOAT retreat_probability;
    orxFLOAT hover_period;
    orxFLOAT block_period;
    orxFLOAT snatch_period;
    orxFLOAT harass_period;
    orxFLOAT fight_period;
    orxFLOAT dive_period;
    orxU32 stun_damage;

    Controller(const char * section): controller_mixin(section) {
        ConfigSectionGuard guard(section);
        can_harass = orxConfig_GetBool("CanHarass");
        can_snatch = orxConfig_GetBool("CanSnatch");
        can_fight = orxConfig_GetBool("CanFight");
        temporary = orxConfig_GetBool("Temporary");
        retreat_probability = orxConfig_GetFloat("RetreatProbability");
        hover_period = orxConfig_GetFloat("HoverPeriod");
        block_period = orxConfig_GetFloat("BlockPeriod");
        snatch_period = orxConfig_GetFloat("SnatchPeriod");
        harass_period = orxConfig_GetFloat("HarassPeriod");
        fight_period = orxConfig_GetFloat("FightPeriod");
        dive_period = 1/((can_snatch ? 1/snatch_period : 0) +
                         (can_harass ? 1/harass_period : 0) +
                         (can_fight  ? 1/fight_period  : 0) );
        stun_damage = orxConfig_GetU32("StunDamage");
    }
    optional<orxVECTOR> choose_bait(Crow & self) {
        auto targetFood = getRandomNode(*self.level->world);
        if(targetFood) {
            orxVECTOR target;
            target.fX = targetFood->pos.x();
            target.fY = targetFood->pos.y();
            target.fZ = self.GetValue<orxFLOAT>("HoverDepth");
            return optional<orxVECTOR>{target};
        }
        return optional<orxVECTOR>{};
    }

    optional<orxVECTOR> choose_sparrow(Crow & self) {
        auto level_object = orxOBJECT(orxObject_GetParent(self.GetOrxObject()));
        auto sparrows = FindAllInOwnedChildren<Sparrow>(level_object);
        if(sparrows.empty()) return optional<orxVECTOR>{};
        else {
            auto sparrow_index = std::uniform_int_distribution<size_t>(0,sparrows.size()-1)(gen);
            auto sPos = sparrows[sparrow_index]->GetPosition();
            return optional<orxVECTOR>(orxVECTOR{sPos.fX,sPos.fY,self.GetValue<orxFLOAT>("HoverDepth")});
        }
    }

    bool any_sparrows_nearby(Crow & self) {
        auto level_object = orxOBJECT(orxObject_GetParent(self.GetOrxObject()));
        auto sparrows = FindAllInOwnedChildren<Sparrow>(level_object);
        for(auto sparrow: sparrows) {
            if(squaredNorm(sparrow->GetPosition()-self.GetPosition()) < 9) return true;
        }
        return false;
    }

    optional<orxVECTOR> pickActionPoint(Crow & self) {
        auto fight_pos = GetValue<orxVECTOR>("FightPosition");
        fight_pos.fZ = self.GetValue<orxFLOAT>("HoverDepth");
        return make_choice<optional<orxVECTOR>>({
            {choose_sparrow(self), can_harass ? 1/harass_period : 0},
            {choose_bait(self), can_snatch ? 1/snatch_period : 0},
            {fight_pos, can_fight ? 1 / fight_period : 0}
        },gen);
    }

    guarded<Dive> diveToAction(Crow & self, bool dive) {
        if(!dive) return optional<orxVECTOR>{};
        return pickActionPoint(self);
    }

    decision decide(Crow & self, Stay & curState, const orxCLOCK_INFO &_rstInfo) {
        bool dive = poisson_event_generator(_rstInfo.fDT)(dive_period);
        return from(curState) >> diveToAction(self, dive);
    }

    decision decide(Crow & self, FlyAtDistance & curState, const orxCLOCK_INFO &_rstInfo) {
        bool dive = poisson_event_generator(_rstInfo.fDT)(dive_period);
        return from(curState) >> diveToAction(self, curState.phase==1 && dive);
    }

    decision decide(Crow & self, Dive & curState, const orxCLOCK_INFO &_rstInfo) {
        if(curState.phase == 1) return attempt_action(self,curState);
        else return react_to_poke(self, curState);
    }

    decision decide(Crow & self, Home & curState, const orxCLOCK_INFO &_rstInfo) {
        return from(curState) >> guarded<Stay>(true);
    }

    decision decide(Crow & self, Hover & curState, const orxCLOCK_INFO &_rstInfo) {
        bool should_act = poisson_event_generator(_rstInfo.fDT)(hover_period);
        bool should_retreat = should_act && std::bernoulli_distribution(retreat_probability)(gen);
        return react_to_poke(self, curState) >>
            retreat(self, should_retreat?optional<bool>{0}:optional<bool>{}) >>
            guarded<FlyOnScreen>(should_act?pickActionPoint(self):optional<orxVECTOR>{})
        ;
    }

    decision decide(Crow & self, FlyOnScreen & curState, const orxCLOCK_INFO &_rstInfo) {
        if(curState.phase == 1) return attempt_action(self,curState);
        else return react_to_poke(self, curState);
    }

    template<class Payload>
    struct retreat_t {
        Controller & cnt; Crow & self; Payload payload;
        retreat_t(Controller & cnt, Crow & self, Payload payload):cnt(cnt), self(self), payload(payload) {}
        template <class SrcState>
        evaluation<SrcState> && operator()(evaluation<SrcState> && ctx) {
            if(cnt.temporary) return std::move(ctx) >> guarded<Exit>(payload);
            else return std::move(ctx) >> guarded<Home>(payload);
        }
    };

    template<class Payload>
    retreat_t<Payload> retreat(Crow & self, Payload payload) {
        return retreat_t<Payload>(*this, self, payload);
    }

    decision decide(Crow & self, Stunned & curState, const orxCLOCK_INFO &_rstInfo) {
        return from(curState) >> retreat(self, optional<bool>{0});
    }

    decision decide(Crow & self, Harassing & curState, const orxCLOCK_INFO &_rstInfo) {
        return react_to_poke(self, curState) >> guarded<Hover>(true);
    }

    decision decide(Crow & self, Enter & curState, const orxCLOCK_INFO &_rstInfo) {
        return from(curState) >> guarded<Stay>(true);
    }

    decision decide(Crow & self, Combat & curState, const orxCLOCK_INFO &_rstInfo) {
        auto subst = curState.substate;
        bool escaped = curState.last_hit >= block_period;
        return from(curState)
            >> guarded<Stunned>(curState.damage >= stun_damage)
            >> guarded<Bite>(subst == Combat::BLOCKED)
            >> guarded<FlyOnScreen>([&]{
                    if(escaped ||
                       subst == Combat::BLOCKED ||
                       subst == Combat::MISSED) {
                        if(escaped) SendSignal(&self,"ESCAPED");
                        return optional<orxVECTOR>(GetValue<orxVECTOR>("FightPosition"));
                    } else return optional<orxVECTOR>{};
            })
            ;
    }

    decision decide(Crow & self, Bite & curState, const orxCLOCK_INFO &_rstInfo) {
        return from(curState) >> guarded<FlyOnScreen>(optional<orxVECTOR>{GetValue<orxVECTOR>("FightPosition")});
    }

    template <class SrcState>
    evaluation<SrcState> react_to_poke(Crow & self, SrcState & curState) {
        auto punch_if_poked = [&] {
            if(self.poked) {
                self.fire_event<crow_punched>(self.poked.val);
                self.poked = {};
                return true;
            } else return false;
        };
        return from(curState) >>
            guarded<Combat>(punch_if_poked) >>
            guarded<Stunned>(punch_if_poked);
    }

    template <class SrcState>
    evaluation<SrcState> attempt_action(Crow & self, SrcState & curState) {
        return from(curState) >>
            guarded<Harassing>([&]{return can_harass && any_sparrows_nearby(self);}) >>
            retreat(self, [&](){
                orxU32 flake_count = 0;
                if(can_snatch) flake_count = snatchFood(self);
                return flake_count ? optional<orxU32>{flake_count} : optional<orxU32>{};
            }) >>
            guarded<Hover>(true) >>
            retreat(self, optional<bool>{0});
    }

    template <class SrcState>
    decision decide(Crow & self, SrcState & curState, const orxCLOCK_INFO &_rstInfo) {
        return from(curState) >> guarded<Stay>(true) >> guarded<Hover>(true) >> guarded<Home>(optional<bool>{false});
    }
};
REGISTER_CONTROLLER(Crow::Controller, "crow")

void Crow::SetContext(ExtendedMonomorphic& context) {
    setLevel(dynamic_cast<GameLevel *>(&context));
}
