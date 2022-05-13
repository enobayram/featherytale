/*
 * behavior_common.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: enis
 */

#ifndef BEHAVIORS_BEHAVIOR_COMMON_HPP_
#define BEHAVIORS_BEHAVIOR_COMMON_HPP_

#include <gameObjects/aspects.hpp>
#include <orxFSM/behavior.hpp>
#include <orxFSM/behavior_combinators.hpp>

#include <orxFSM/orxFSM_header.h>

#include <orxFSM/simple_behaviors.hpp>
#include <orxFSM/orx_behaviors.hpp>
#include <orxFSM/event_behaviors.hpp>

#include <gameObjects/screens/GameLevel.h>
#include <gameObjects/CountdownTimer.h>
#include <gameObjects/Sparrow.h>
#include <gameObjects/Score.h>

class RectangularBody;

orxSTATUS set_velocity_for_target_action(game_context * lvl, orxVECTOR nearby_point, orxVECTOR target);
ACTION_LEAF(set_velocity_for_target, set_velocity_for_target_action)

orxVECTOR calculate_two_flake_pick_up_point_action(RectangularBody * cheese);
RETURNING_ACTION_LEAF(calculate_two_flake_pick_up_point, calculate_two_flake_pick_up_point_action)

void align_with_vector_action(orxOBJECT * arrow, orxVECTOR position, orxVECTOR vector);
ACTION_LEAF(align_with_vector, align_with_vector_action)

template<class ARROW, class POS, class VEC>
behavior_constructor create_arrow(ARROW arrow, POS pos, VEC vec) {
    return sequence {
        create(arrow, "Arrow"),
        align_with_vector(arrow, pos, vec)
    };
}

orxSTATUS world_empty_action(game_context * level);
ACTION_LEAF(world_empty, world_empty_action)

behavior_constructor spawn_cheese_when_finished();

struct spawn_sparrows_with_period_behavior: behavior_t {
    GameLevel * level;
    in_out_variable<orxFLOAT> period;
    std::string sparrow_section;
    spawn_sparrows_with_period_behavior(
            GameLevel * level,
            in_out_variable<orxFLOAT> period,
            std::string sparrow_section = GetBindName<Sparrow>()):
        level(level), period(period), sparrow_section(sparrow_section){}
    behavior_state run(const orxCLOCK_INFO & _rstInfo);
};
BEHAVIOR_LEAF(spawn_sparrows_with_period, spawn_sparrows_with_period_behavior)

template <class LEVEL, class SECTION = const char *>
behavior_constructor spawn_sparrows_randomly(LEVEL level, SECTION section = GetBindName<Sparrow>(), const char * periodkey = "SparrowSpawnPeriod") {
    return sequence {
        $f(periodkey) = $config<orxFLOAT>(periodkey),
                spawn_sparrows_with_period(level, $f(periodkey), section)
    };
}

behavior_constructor show_end_dialog();

inline orxSTATUS zero_happiness_action(GameLevel * level) {
    return (level->getHappiness() == 0) ? orxSTATUS_SUCCESS : orxSTATUS_FAILURE;
}
ACTION_LEAF(zero_happiness, zero_happiness_action)

inline orxSTATUS full_happiness_action(GameLevel * level) {
    return (level->getHappiness() == 1) ? orxSTATUS_SUCCESS : orxSTATUS_FAILURE;
}
ACTION_LEAF(full_happiness, full_happiness_action)

behavior_constructor unhappy_bird_left(const char * result);

class wait_for_free_button_click_behavior : public behavior_t {
    weak_object_ptr button_ptr;
    Button * button;
    bool clicked = false;
public:
    wait_for_free_button_click_behavior(orxOBJECT * owner, std::string section) ;
    behavior_state run(const orxCLOCK_INFO & clock);
    ~wait_for_free_button_click_behavior();
};
BEHAVIOR_LEAF(wait_for_free_button_click, wait_for_free_button_click_behavior)

behavior_constructor skippable(behavior_constructor behavior);

class snow_behavior: public behavior_t {
    GameLevel * level;
public:
    snow_behavior(GameLevel * level):level(level) {}
    behavior_state run(const orxCLOCK_INFO & clock);
};
BEHAVIOR_LEAF(snow, snow_behavior)

behavior_constructor murder_attack();

behavior_constructor winter_event(behavior_constructor terminator, $f temperature_diff);

class fill_score_behavior: public object_behavior_mixin<fill_score_behavior, ScoreDisplay> {
    orxVECTOR sourcePos;
    orxU32 value;
    orxFLOAT rate;
    orxU32 increment;
    orxU32 created = 0;
    orxFLOAT expected = 0;
public:
    fill_score_behavior(ScoreDisplay * self, orxVECTOR sourcePos, orxU32 value);
    behavior_state run_object(const orxCLOCK_INFO & clock);
};
BEHAVIOR_LEAF(fill_score, fill_score_behavior)

struct count_eats_behavior: event_behavior<eat_event> {
    in_out_variable<orxU32> count;
    count_eats_behavior(orxOBJECT * ed, in_out_variable<orxU32> count):behavior_base(ed), count(count){}
    void handle_event() {count = orxU32(count) + 1;}
};
BEHAVIOR_LEAF(count_eats,count_eats_behavior)

struct count_beats_behavior: event_behavior<crow_beaten_event> {
    in_out_variable<orxU32> count;
    const char * crow_type;
    count_beats_behavior(orxOBJECT *ed, in_out_variable<orxU32> count, const char * crow_type = "Crow")
        :behavior_base(ed), count(count), crow_type(crow_type){}
    void handle_event(Crow *);
};
BEHAVIOR_LEAF(count_beats,count_beats_behavior)

behavior_constructor bird_flock_event($i bird_count, $os bird_collection, const char * sparrow_type = "Sparrow");

behavior_constructor create_listed_crows($os collection = $os("__non_existent__"));

behavior_constructor point_at_object_in_tutorial($o object, $o arrow, bool fromTop = false);

behavior_constructor consecutive_baskets_challenge($o level, $b challenge);

behavior_constructor full_happiness_reached(const char * result);

behavior_constructor receive_signal_fewer_than_challenge($o level, $b challenge, const char * signal, int count);

behavior_constructor send_birds_happy_challenge($b challenge);

behavior_constructor no_snatch_challenge($o level, $b challenge);
behavior_constructor no_harass_challenge($o level, $b challenge);
behavior_constructor no_snatch_or_harass_challenge($o level, $b challenge);
behavior_constructor no_bite_challenge($o level, $b challenge);

behavior_constructor crow_beat_challenge($b challenge);

behavior_constructor happiness_bar_mission();

behavior_constructor level_countdown();

behavior_constructor show_facebook_share_button($o level, $o dialog, $f buttonX);

behavior_constructor share_on_facebook($o level);

behavior_constructor cheese_rain();

#endif /* BEHAVIORS_BEHAVIOR_COMMON_HPP_ */
