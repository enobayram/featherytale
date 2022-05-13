/*
 * gesturemanager.h
 *
 *  Created on: Mar 26, 2015
 *      Author: seliz
 */

#ifndef GESTUREMANAGER_H_
#define GESTUREMANAGER_H_

#include <map>
#include <list>
#include <memory>

#include <gestures.h>

class gesture_manager {
public:
    orxVIEWPORT * viewport;
    virtual void trail_update(trace element, trail_id id) =0;
    virtual void trail_start(trace element, trail_id id) =0;
    virtual void trail_end(trace element, trail_id id) =0;
    virtual void time_update(double time) =0;
    virtual ~gesture_manager();
    gesture_manager(orxVIEWPORT * viewport): viewport(viewport) {}
};

class game_gesture_manager : public gesture_manager {
	std::list<std::unique_ptr<gesture>> gestures;
	std::map<trail_id, std::unique_ptr<trail>> trail_map;
public:
    void trail_update(trace element, trail_id id);
    void trail_start(trace element, trail_id id);
    void trail_end(trace element, trail_id id);
    void time_update(double time);
    game_gesture_manager(orxVIEWPORT * viewport): gesture_manager(viewport) {}
private:
    void remove_gesture(gesture *);
};

#endif /* GESTUREMANAGER_H_ */
