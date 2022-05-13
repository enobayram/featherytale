/*
 * behavior_common.cpp
 *
 *  Created on: Sep 10, 2015
 *      Author: eba
 */

#include "behavior_common.hpp"
#include <util/random.hpp>
#include <pancarDemo.h>
#include <gameObjects/RectangularBody.h>
#include <gameObjects/Crow.h>
#include <orxFSM/event_behaviors.hpp>
#include <analytics.h>
#include <platform.h>


orxSTATUS set_velocity_for_target_action(game_context * lvl, orxVECTOR nearby_point, orxVECTOR target) {
    Node * node = lvl->world->closestNode({nearby_point.fX, nearby_point.fY});
    Eigen::Vector2d pos = node->pos;
    orxFLOAT xn = pos.x(), xt = target.fX, yn = pos.y(), yt = target.fY, g = -get_gravity();
    if((yn - yt)/g < 0) return orxSTATUS_FAILURE;
    orxFLOAT vx = -0.707106781186548*g*sqrt((yn - yt)/g)*(xn - xt)/(yn - yt);
    node->vel = {vx,0};
    return orxSTATUS_SUCCESS;
}

orxVECTOR calculate_two_flake_pick_up_point_action(RectangularBody * cheese) {
    auto com = cheese->GetCenterOfMass();
    auto body_size = cheese->GetValue<orxVECTOR>("BodySize");
    return com + orxVECTOR{body_size.fX/2-1,-body_size.fY/2-0.2f,0};
}

void align_with_vector_action(orxOBJECT * arrow, orxVECTOR position, orxVECTOR vector) {
    orxObject_SetPosition(arrow, &position);
    orxFLOAT height = sqrt(squaredNorm(vector));
    SetHeight(arrow, height);
    orxFLOAT angle = atan2(vector.fY, vector.fX)-PI/2;
    orxObject_SetRotation(arrow, angle);
    orxObject_AddFX(arrow, "ArrowPointFX");
}

behavior_t::behavior_state spawn_sparrows_with_period_behavior::run(const orxCLOCK_INFO & _rstInfo){
    auto period = this->period*(Sparrow::totalCount+1);
    if(poisson_event_generator(_rstInfo.fDT)(period)) {
        level->CreateChild<Sparrow>(sparrow_section.c_str());
    }
    return RUNNING;
}

behavior_constructor show_end_dialog() {
    return sequence {
        pause_level($o("^"), true),
        create($o("LevelEndDialog"),"LevelEndDialogInWorld"),
        config_get($f("MenuButtonX"), $o("LevelEndDialog"), "MenuButtonX"),
        config_get($f("RestartButtonX"), $o("LevelEndDialog"), "RestartButtonX"),
        config_get($f("FacebookButtonX"), $o("LevelEndDialog"), "FacebookButtonX"),
        any {
            sequence {
                wait_for_button_click($o("LevelEndDialog"), "MenuIcon",$f("MenuButtonX")),
                set_next_screen($o("^"), $config("MenuScreen")),
            },
            sequence {
                wait_for_button_click($o("LevelEndDialog"), "RestartIcon",$f("RestartButtonX")),
                get_name($cp("ThisLevelName"), $o("^")),
                set_next_screen($o("^"), $cp("ThisLevelName")),
            },
            show_facebook_share_button($o("^"), $o("LevelEndDialog"), $f("FacebookButtonX")),
        },
        pause_level($o("^"), false),
    };
}

struct wait_for_unhappy_bird_behavior: behavior_t {
    GameLevel * level;
    handler_index<sparrow_leave_event> hi;
    bool occurred = false;
    wait_for_unhappy_bird_behavior(GameLevel * level): level(level) {
        hi = level->register_handler<sparrow_leave_event>([this](Sparrow * s) mutable {
            if(s->happiness < 0.5) occurred = true;
        });
    }
    behavior_state run(const orxCLOCK_INFO &) {
        return occurred ? SUCCEEDED : RUNNING;
    }
    ~wait_for_unhappy_bird_behavior() { level->remove_handler(hi);}
};
BEHAVIOR_LEAF(wait_for_unhappy_bird, wait_for_unhappy_bird_behavior)

behavior_constructor unhappy_bird_left(const char * result) {
    return keep_running() <<= sequence {
        $b(result) = true,
        wait_for_unhappy_bird($o("^")),
        $b(result) = false,
    };
}

wait_for_free_button_click_behavior::wait_for_free_button_click_behavior(orxOBJECT * owner, std::string section) {
    auto buttonObj = CreateChild(owner, section.c_str());
    button_ptr = buttonObj;
    button = scroll_cast<Button>(buttonObj);
    if(button) {
        button->set_handler([=]() {clicked=true;});
    }
}

behavior_t::behavior_state wait_for_free_button_click_behavior::run(const orxCLOCK_INFO & clock) {
    if(!button_ptr) return FAILED;
    if(clicked) return SUCCEEDED;
    return RUNNING;
}

wait_for_free_button_click_behavior::~wait_for_free_button_click_behavior() {
    if(button_ptr) {
        button->clear_handler();
        button->SetLifeTime(0);
    }
}

template <class ... FIELDS>
orxSTATUS send_design_event_action(FIELDS ... fields) {
    design_event({fields...});
    return orxSTATUS_SUCCESS;
}
COMPLEX_ACTION_LEAF(send_design_event, send_design_event_action)

behavior_constructor skippable(behavior_constructor behavior) {
    return collection_scope($os("TutObjs")) <<= any {
        sequence {
            behavior,
            get_name($s("LevelName"), $o("^")),
            send_design_event("Watch", "Tutorial", $cp("LevelName")),
        },
        sequence {
            wait_for_free_button_click($o("^"), "SkipIcon"),
            for_collection($o("Obj"), $os("TutObjs")) <<= destroy($o("Obj")),
        },
    };
}

behavior_t::behavior_state snow_behavior::run(const orxCLOCK_INFO& clock) {
    bool new_flake = poisson_event_generator(clock.fDT)(0.1);
    if(new_flake) level->CreateChild("SnowFlake");
    return RUNNING;
}

behavior_constructor murder_attack() {
    return collection_scope($os("Murder")) <<= sequence {
        get_controller($s("PreMurderController"), $o("Father")),
        set_controller($o("Father"), "FatherCallController"),
        wait_for_state($o("Father"), "RaiseWings"),
        add_sound($o("^"), "CrowFatherSummonsArmySound"),
        timeout(2),
        for_range($i("i"), 0 ,10) <<= sequence {
            random_in_range($f("t"),0,0.3),
            timeout($f("t")),
            create($o("ArmyCrow"), "ArmyCrow", false),
            append_object($os("Murder"),$o("ArmyCrow"))
        },
        timeout(2),
        set_controller($o("Father"), "FatherOrderController"),
        add_sound($o("^"), "CrowFatherOrdersArmySound"),
        for_collection($o("ArmyCrow"), $os("Murder")) <<= set_controller($o("ArmyCrow"), "ArmyCrowAttackController"),
        any {
            wait_for_success <<= collection_empty($os("Murder")),
            timeout(10),
        },
        for_collection($o("ArmyCrow"), $os("Murder")) <<= sequence {
            set_controller($o("ArmyCrow"),"CrowExitConverger"),
            random_in_range($f("t"),0,0.3),
            timeout($f("t")),
        },
        set_controller($o("Father"), $cp("PreMurderController")),
    };
}

BEHAVIOR_LEAF(temperature_responder, query_responder<temperature_status>)

behavior_constructor winter_event(behavior_constructor terminator, $f temperature_diff) {
    return any {
        temperature_responder($o("^"), $f("CurTempDiff")),
        sequence {
            any {
                snow($o("^")),
                keep_running() <<= count($f("CurTempDiff"),-3, temperature_diff),
                terminator,
            },
            count($f("CurTempDiff"),3, 0),
        },
    };
}

void count_beats_behavior::handle_event(Crow * crow) {
    auto name = crow->GetModelName();
    while(name) {
        if(strcmp(name, crow_type) == 0) {
            count = count+1;
            return;
        }
        name = orxConfig_GetParent(name);
    }
}

behavior_constructor bird_flock_event($i bird_count, $os bird_collection, const char * sparrow_type) {
    return sequence {
        create($o("FlockSymbol"), "SparrowFlockSymbol"),
        any {
            attach_to_object($o("FlockSymbol")),
            sequence {
                timeout($config<orxFLOAT>("FlockWarningPeriod")),
                for_range($i("i"), 0, bird_count) <<= sequence {
                    create_in_collection(bird_collection, $o(""), sparrow_type, false),
                    timeout(0.1)
                },
                timeout(1),
            }
        }
    };
}

behavior_constructor create_listed_crows($os collection) {
    return for_config_string_list($cp("__OBJ__"), "Crows") <<= sequence {
        timeout(0.2),
        create_in_collection(collection, $o(""), $s("__OBJ__"), false, true)
    };
}

void GetArrowForObject(in_out_variable<orxVECTOR> pos, in_out_variable<orxVECTOR> vec, orxOBJECT * object, bool fromTop) {
    auto objPos = GetPosition(object);
    if(fromTop) {
        pos = objPos + orxVECTOR{0,-4,0};
        vec = orxVECTOR{0,2};
    } else if(objPos.fX < 0) {
        pos = objPos + orxVECTOR{4,0,0};
        vec = orxVECTOR{-2,0};
    } else {
        pos = objPos + orxVECTOR{-4,0,0};
        vec = orxVECTOR{2,0};
    }
}
ACTION_LEAF(get_arrow_for_object, GetArrowForObject)

behavior_constructor point_at_object_in_tutorial($o object, $o arrow, bool fromTop) {
    return sequence {
        get_arrow_for_object($v("__ArrowPos__"), $v("__ArrowVec__"), object, fromTop),
        create_arrow(arrow,$v("__ArrowPos__"), $v("__ArrowVec__")),
        append_object($os("TutObjs"), arrow),
    };
}

struct wait_for_consecutive_baskets_behavior: event_behavior<score_event> {
    int curcount = 0, minscore, count;
    wait_for_consecutive_baskets_behavior(orxOBJECT * ed, int minscore = 1, int count = 1)
            :behavior_base(ed), minscore(minscore), count(count) {}
    void handle_event(const char * et, int delta) {
        if(strcmp(et,"basket") == 0) {
            if(delta >= minscore) {
                curcount++;
                if(curcount == count) returned_state = SUCCEEDED;
            }
            else curcount = 0;
        }
    }
};
BEHAVIOR_LEAF(wait_for_consecutive_baskets, wait_for_consecutive_baskets_behavior)

behavior_constructor consecutive_baskets_challenge($o level, $b challenge) {
    return sequence {
        wait_for_consecutive_baskets($o("^"), $config<orxU32>("ChallengeBasketScore"), $config<orxU32>("ChallengeBasketCount")),
        challenge = true,
        constant(behavior_t::RUNNING),
    };
}

behavior_constructor full_happiness_reached(const char * result) {
    return sequence {
        $b(result) = false,
        wait_for_success <<= full_happiness($o("^")),
        $b(result) = true,
        constant(behavior_t::RUNNING),
    };
}

behavior_constructor receive_signal_fewer_than_challenge($o level, $b challenge, const char * signal, int count) {
    return sequence {
        challenge = true,
        for_range($i(""), 0, count) <<= wait_for_signal($o("^"), signal),
        challenge = false,
        constant(behavior_t::RUNNING),
    };
}

struct wait_for_happy_birds_behavior: event_behavior<sparrow_leave_event>{
    orxS32 remaining;
    orxFLOAT min_happiness;
    wait_for_happy_birds_behavior(orxOBJECT * ed, orxU32 count, orxFLOAT min_happiness):
        behavior_base(ed), remaining(count), min_happiness(min_happiness){}
    void handle_event(Sparrow * s){
        if(s->happiness >= min_happiness) --remaining;
        if(remaining <= 0) returned_state = SUCCEEDED;
    }
};
BEHAVIOR_LEAF(wait_for_happy_birds, wait_for_happy_birds_behavior)


behavior_constructor send_birds_happy_challenge($b challenge) {
    return sequence {
        challenge = false,
        wait_for_happy_birds($o("^"),
                             $config<orxU32>("ChallengeHappyBirdCount"),
                             $config<orxFLOAT>("ChallengeBirdMinHappiness")),
        challenge = true,
        constant(behavior_t::RUNNING),
    };
}

struct wait_for_snatch_behavior: event_behavior<snatch_event> {
    using behavior_base::behavior_base;
    void handle_event() { returned_state = SUCCEEDED;}
};
BEHAVIOR_LEAF(wait_for_snatch, wait_for_snatch_behavior)

struct wait_for_harass_behavior: event_behavior<crow_harass_event> {
    using behavior_base::behavior_base;
    void handle_event(orxVECTOR) { returned_state = SUCCEEDED;}
};
BEHAVIOR_LEAF(wait_for_harass, wait_for_harass_behavior)

struct wait_for_bite_behavior: event_behavior<bite_event> {
    using behavior_base::behavior_base;
    void handle_event() { returned_state = SUCCEEDED;}
};
BEHAVIOR_LEAF(wait_for_bite, wait_for_bite_behavior)

behavior_constructor no_snatch_challenge($o level, $b challenge) {
    return sequence {
        challenge = true,
        wait_for_snatch(level),
        challenge = false,
        constant(behavior_t::RUNNING),
    };
}

behavior_constructor no_harass_challenge($o level, $b challenge) {
    return sequence {
        challenge = true,
        wait_for_harass(level),
        challenge = false,
        constant(behavior_t::RUNNING),
    };
}

behavior_constructor no_snatch_or_harass_challenge($o level, $b challenge) {
    return sequence {
        challenge = true,
        any {
            wait_for_harass(level),
            wait_for_snatch(level),
        },
        challenge = false,
        constant(behavior_t::RUNNING),
    };
}

behavior_constructor no_bite_challenge($o level, $b challenge) {
    return sequence {
        challenge = true,
        wait_for_bite(level),
        challenge = false,
        constant(behavior_t::RUNNING),
    };
}

struct crow_beat_challenge_behavior: event_behavior<crow_beaten_event> {
    orxS32 remaining;
    const char * type;
    in_out_variable<bool> challenge;
    crow_beat_challenge_behavior(GameLevel * level, in_out_variable<bool> challenge):
        behavior_base(level->GetOrxObject()), challenge(challenge) {
        type = level->GetValue<const char *>("ChallengeCrowBeatType");
        remaining = level->GetValue<orxU32>("ChallengeCrowBeatCount");
    }
    void handle_event(Crow * crow) {
        if(crow->GetModelName() == type) { // We can compare like this since they're both persisted
            --remaining;
            if(remaining<=0) challenge = true;
        }
    }
};
BEHAVIOR_LEAF(crow_beat_challenge, crow_beat_challenge_behavior)

behavior_constructor crow_beat_challenge($b challenge) {
    return crow_beat_challenge($o("^"), challenge);
}

void get_default_failure_message_action(in_out_variable<std::string> message, GameLevel * level) {
    message = Localize(level->GetValue<const char *>("FailureString"));
}
ACTION_LEAF(get_default_failure_message, get_default_failure_message_action)

behavior_constructor happiness_bar_mission() {
    return sequence {
        wait_for_success <<= zero_happiness($o("^")),
        get_default_failure_message($s("Failure"), $o("^")),
    };
}

behavior_constructor level_countdown() {
    return any {
        countdown($o("^"), $config("Timer")),
        wait_for_success <<= is_input_active("EndLevel"),
    };
}

orxSTATUS world_empty_action(game_context * level) {
    return (level->world->numberOfNodes() == 0) ? orxSTATUS_SUCCESS : orxSTATUS_FAILURE;
}

struct wait_for_few_cheese_pieces_behavior: object_behavior_mixin<wait_for_few_cheese_pieces_behavior, GameLevel> {
    orxU32 wait_until;
    wait_for_few_cheese_pieces_behavior(GameLevel * level, orxU32 wait_until): MIXIN(level), wait_until(wait_until){}
    behavior_state run_object(const orxCLOCK_INFO &) {
        return self->world->numberOfNodes() <= wait_until ? SUCCEEDED : RUNNING;
    }
};
BEHAVIOR_LEAF(wait_for_few_cheese_pieces, wait_for_few_cheese_pieces_behavior)

behavior_constructor spawn_cheese_when_finished() {
    return repeat <<= sequence {
        wait_for_few_cheese_pieces($o("^"), $config("NewCheeseAt")),
        create($o("Cheese"), $config<std::string>("CheeseType"), false),
    };
}

std::string getOnLevelString(GameLevel * level) {
    if(strcmp(level->GetValue<const char *>("LevelType"), "Story") == 0) {
        auto fmt = orxLocale_GetString("OnStory");
        return sprint_action(fmt, level->GetValue<const char *>("LevelNumber"));
    } else {
        return orxLocale_GetString((std::string("On") + level->GetModelName()).c_str());
    }
}


struct share_on_facebook_behavior: behavior_t {
    bool occurred = false;
    weak_object_ptr thumbnail_ptr;
    weak_object_ptr level_ptr;
    orxU64 viewport_id;
    share_on_facebook_behavior(GameLevel * level) {
        level_ptr = level->GetOrxObject();
        auto thumbnail = orxObject_CreateFromConfig("ShareImageBackground");//level->GetValue<const char *>("Thumbnail"));
        orxObject_SetGroupID(thumbnail, orxString_GetID("ShareImage"));
        SetHeight(thumbnail, 300);
        SetPosition(thumbnail, {0,0,0});
        thumbnail_ptr = thumbnail;
        auto display = CreateChild(thumbnail, "ShareImageScoreDisplay");
        scroll_cast<ScoreDisplay>(display)->setScore(level->getScore());
        CreateChild(thumbnail, "ShareImageScoreBackground");
        viewport_id = orxStructure_GetGUID(orxViewport_CreateFromConfig("ShareImageViewport"));
    }
    behavior_state run(const orxCLOCK_INFO &) {
        if(occurred) {
            auto viewport = (orxVIEWPORT *) orxStructure_Get(viewport_id);
            if(!viewport || !level_ptr) return FAILED;
            auto level = scroll_cast<GameLevel>(level_ptr);
            orxTEXTURE * texture;
            orxViewport_GetTextureList(viewport, 1, &texture);
            auto bitmap = orxTexture_GetBitmap(texture);
            std::string image_path = orxFile_GetApplicationSaveDirectory("ShareScore.bmp");
            orxDisplay_SaveBitmap(bitmap, image_path.c_str());
            auto desc_format = orxLocale_GetString("LevelShareDescription");
            auto at_level_string = getOnLevelString(level);
            auto description = sprint_action(desc_format, at_level_string, level->getScore());
            design_event({"Monetization", "Facebook", "ShareClicked", "LevelEnd"});
            GetPlatform()->showShareDialog({image_path, description});
            return SUCCEEDED;
        }
        else {
            occurred = true;
            return RUNNING;
        }
    }
    ~share_on_facebook_behavior() {
        if(thumbnail_ptr) orxObject_SetLifeTime(thumbnail_ptr, 0);
        if(auto viewport = (orxVIEWPORT *) orxStructure_Get(viewport_id)) {
            orxViewport_Delete(viewport);
        }
    }
};
BEHAVIOR_LEAF(share_on_facebook_internal, share_on_facebook_behavior)

behavior_constructor share_on_facebook($o level) { return share_on_facebook_internal(level);}

behavior_constructor show_facebook_share_button($o level, $o dialog, $f buttonX) {
    return repeat <<= sequence {
        wait_for_button_click(dialog, "ActiveFacebookButton", buttonX),
        share_on_facebook(level),
    };
}

behavior_constructor cheese_rain() {
    return sequence {
        collection_scope($os("Sparrows")) <<= sequence {
            any {
                sequence {
                    for_range($i("i"), 0, $config("CheeseRainSparrow", "WaveSize")) <<= sequence {
                        create($o("CheeseRainSparrow"), "CheeseRainSparrow", false),
                        append_object($os("Sparrows"), $o("CheeseRainSparrow")),
                        $f("WaitPeriod") = $config("CheeseRainSparrow", "CreationWait"),
                        timeout<$f>($f("WaitPeriod")),
                    },
                    create($o("CheeseRainClouds"), "CheeseRainClouds"),
                    timeout(1),
                    for_range($i("i"), 0, $config("CheeseRainFlake", "WaveSize")) <<= sequence {
                        create($o("Flake"),"CheeseRainFlake", false),
                        $f("WaitPeriod") = $config("CheeseRainFlake", "CreationWait"),
                        timeout($f("WaitPeriod")),
                    },
                    timeout($config<orxFLOAT>("CheeseRainSparrow", "WaitAfterRain")),
                },
                wait_for_harass($o("^")),
            },
            destroy($o("CheeseRainClouds")),
            for_collection($o("CheeseRainSparrow"), $os("Sparrows")) <<= sequence {
                set_controller($o("CheeseRainSparrow"), "SparrowExitConverger"),
                timeout(0.1),
            }
        }
    };
}

fill_score_behavior::fill_score_behavior(ScoreDisplay* self,
        orxVECTOR sourcePos, orxU32 value): MIXIN(self), sourcePos(sourcePos), value(value) {
    auto Fill100PIn = orxConfig_GetFloat("ScoreFill100PointsIn");
    auto MaxStarsPerSec = orxConfig_GetFloat("ScoreFillMaxStarsPerSec");
    if(value < 10) {
        rate = value;
        increment = 1;
    } else {
        // a * ln(value) + b = t
        // a * ln(10)    + b = 1
        // a * ln(100)   + b = Fill100PIn
        double a = (Fill100PIn - 1) / log(10);
        double b = 1 - a * log(10);
        double t = (a * log(value) + b);
        rate = value/t;
        // rate/increment = StarsPerSec
        increment = ceil(rate/MaxStarsPerSec);
    }
}

behavior_t::behavior_state fill_score_behavior::run_object(const orxCLOCK_INFO& clock) {
    expected = min(expected + clock.fDT*rate, (orxFLOAT)value);
    if(expected==(orxFLOAT) value) {
        self->addScore(1, sourcePos, false, value-created);
        created = value;
        return SUCCEEDED;
    } else {
        if(expected - created >= increment) {
            self->addScore(1, sourcePos, false, increment);
            created += increment;
        }
        return RUNNING;
    }
}
