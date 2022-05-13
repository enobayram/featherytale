/*
 * gesturemanager.cpp
 *
 *  Created on: Mar 26, 2015
 *      Author: seliz
 */

#include <tuple>
#include <vector>
#include <algorithm>

//#include <boost/geometry.hpp>
//#include <boost/geometry/index/rtree.hpp>
//#include <boost/geometry/geometries/geometries.hpp>
//#include <boost/geometry/geometries/register/point.hpp>
#include <boost/range/adaptor/map.hpp>

#include <gesturemanager.h>
#include <pancarDemo.h>
#include <orx_utilities.h>
#include <util/orx_eigen.h>
#include <gameObjects/aspects.h>
#include <scroll_ext/ExtendedObject.h>

//namespace bgi = boost::geometry::index;
//
//BOOST_GEOMETRY_REGISTER_POINT_3D(trace,double,boost::geometry::cartesian_tag,x,y,t);
//
//typedef bgi::rtree<std::pair<trace,trail_id>, bgi::quadratic<4>> rtree;

gesture_manager::~gesture_manager() {}

void game_gesture_manager::trail_update(trace element, trail_id id){
    if(!trail_map.count(id)) return;
	assert(trail_map[id]->active);
	trail & t = *trail_map[id];
	t.traces.push_back(element);
}

void game_gesture_manager::trail_start(trace element, trail_id id){
	assert(trail_map.find(id) == trail_map.end());
	trail * newT = new trail();
	trail_map[id].reset(newT);
	newT->traces.push_back(element);
}

void game_gesture_manager::trail_end(trace element, trail_id id){
    if(!trail_map.count(id)) return;
    assert(trail_map[id]->active);
	trail_map[id]->active = false;
}

std::string crow = "Crow";

bool isGroupVisible(orxCAMERA * camera, orxU32 groupID) {
    for(int i=orxCamera_GetGroupIDCounter(camera)-1; i>=0; i--) {
        if(groupID==orxCamera_GetGroupID(camera,i)) return true;
    }
    return false;
}

gesture * GetClickGesture(orxCAMERA * camera, trail & t) {
    trace trc = t.traces.back();
    orxVECTOR trcPt {(orxFLOAT)trc.x,(orxFLOAT) trc.y, -1};

    click_handler* handler = nullptr;

    orxOBOX pickBox;
    orxVECTOR boxSize{0.1,0.1,3}, boxPivot{0,0,0};
    orxOBox_2DSet(&pickBox, &trcPt, &boxPivot, &boxSize, 0);
    orxBANK * pickBank = orxObject_CreateNeighborList(&pickBox);
    if(pickBank) {
        std::vector<orxOBJECT *> objects;
        objects.reserve(orxBank_GetCounter(pickBank));
        for(size_t i=0; i<orxBank_GetCounter(pickBank); i++) {
            auto obj = *((orxOBJECT **) orxBank_GetAtIndex(pickBank, i));
            if(!isGroupVisible(camera, orxObject_GetGroupID(obj))) continue;
            objects.push_back(obj);
        }
        orxObject_DeleteNeighborList(pickBank);
        std::sort(objects.begin(), objects.end(), [](orxOBJECT *o1, orxOBJECT *o2) { return GetWorldPosition(o1).fZ < GetWorldPosition(o2).fZ;});
        for(auto obj : objects) {
            if((handler = GetAspect<click_handler>(obj))) {
                gesture * new_gesture = handler->clicked(t);
                if(new_gesture) return new_gesture;
            }
        }
    }
    return nullptr;
}

void game_gesture_manager::time_update(double time){
	for(auto && t: trail_map | boost::adaptors::map_values ) {
		if(!t->attached_gesture && t->active) {
			auto click_gesture = GetClickGesture(orxViewport_GetCamera(viewport), *t);
			if(click_gesture) {
	            gestures.emplace_front(click_gesture);
	            t->attached_gesture = gestures.front().get();
			}
		}
	}

	std::list<gesture*> to_remove;

	for(auto &g : gestures) {
		if(g->time_update(time) == orxSTATUS_FAILURE) {
			to_remove.push_back(g.get());
		}
	}

	for(auto g: to_remove) {
		remove_gesture(g);
	}
}

void game_gesture_manager::remove_gesture(gesture *g) {
	gestures.remove_if([g](const std::unique_ptr<gesture> & g2) {return g2.get() ==g;});
}

std::map<orxU32, trail_id> touch_map;

trail_id next_id = 0;

orxSTATUS orxFASTCALL touchHandler(const orxEVENT *_pstEvent) {
    if(_pstEvent->eID == orxSYSTEM_EVENT_TOUCH_BEGIN ||
            _pstEvent->eID == orxSYSTEM_EVENT_TOUCH_END   ||
            _pstEvent->eID == orxSYSTEM_EVENT_TOUCH_MOVE) {

        gesture_manager * man = pancarDemo::GetInstance().registered_manager;

        auto payload = (const orxSYSTEM_EVENT_PAYLOAD *) _pstEvent->pstPayload;
        auto mouseWorldPos = getInWorld(man->viewport, {payload->stTouch.fX, payload->stTouch.fY,0});
        Vector2d mouseWorldVec = mouseWorldPos ? toEigen2d(*mouseWorldPos) : Vector2d{0,0};
        trace trc = make_trace(mouseWorldVec, orxSystem_GetTime());//payload->stTouch.dTime);

        if(_pstEvent->eID == orxSYSTEM_EVENT_TOUCH_END ) {
            if(man) man->trail_end(trc, touch_map[payload->stTouch.u32ID]);
            touch_map.erase(payload->stTouch.u32ID);
        }

        if(!mouseWorldPos) return orxSTATUS_SUCCESS;
        if(_pstEvent->eID == orxSYSTEM_EVENT_TOUCH_BEGIN ) {
        	touch_map[payload->stTouch.u32ID] = next_id;
        	if(man) man->trail_start(trc, next_id);
        	next_id++;
        }
        if(_pstEvent->eID == orxSYSTEM_EVENT_TOUCH_MOVE ) {
            if(man) man->trail_update(trc, touch_map[payload->stTouch.u32ID]);
        }

    }
    return orxSTATUS_SUCCESS;
}
