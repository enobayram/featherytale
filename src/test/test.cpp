/*
 * test.cpp
 *
 *  Created on: Feb 14, 2016
 *      Author: eba
 */

#include <orxFSM/behavior.hpp>
#include <orxFSM/orx_behaviors.hpp>
#include <orxFSM/behavior_combinators.hpp>
#include <orxFSM/simple_behaviors.hpp>
#include <scroll_ext/ExtendedObject.h>

#include <pancarDemo.h>
#include <persistence.h>

void createTestCursorIfTestMode() {
    auto data = fetch_persistent<test_data>("Test");
    if(data.test_mode) {
        orxObject_CreateFromConfig("TestCursor");
    }
}

void send_touch_event(orxSYSTEM_EVENT type, orxFLOAT x, orxFLOAT y, orxU32 id) {
    orxSYSTEM_EVENT_PAYLOAD payload;
    payload.stTouch.dTime = orxSystem_GetTime();
    payload.stTouch.fPressure = 1;
    payload.stTouch.fX = x;
    payload.stTouch.fY = y;
    payload.stTouch.u32ID = id;

    orxEVENT event;
    event.eType = orxEVENT_TYPE_SYSTEM;
    event.eID = type;
    event.hRecipient = 0;
    event.hSender = 0;
    event.pContext = 0;
    event.pstPayload = &payload;
    orxEvent_Send(&event);
}

orxVIEWPORT * GetCursorViewport();

struct imitate_click_behavior: object_behavior_mixin<imitate_click_behavior> {
    static orxU32 next_click_id;
    orxU32 clickID;
    orxVIEWPORT * viewport;
    void send_event(orxSYSTEM_EVENT type) {
        auto pos = GetPosition(self);
        orxVECTOR screenPos;
        orxRender_GetScreenPosition(&pos, viewport, &screenPos);
        send_touch_event(type, screenPos.fX, screenPos.fY, clickID);
    }
    imitate_click_behavior(orxOBJECT * self): MIXIN(self) {
        clickID = next_click_id++;
        viewport = GetCursorViewport();
        send_event(orxSYSTEM_EVENT_TOUCH_BEGIN);
        orxObject_SetTargetAnim(self, "HandTouchAnim");
    }
    behavior_state run_object(const orxCLOCK_INFO & clock) {
        send_event(orxSYSTEM_EVENT_TOUCH_MOVE);
        return RUNNING;
    }
    ~imitate_click_behavior() {
        if(self_ptr) {
            send_event(orxSYSTEM_EVENT_TOUCH_END);
            orxObject_SetTargetAnim(self, "HandHoverAnim");
        }
    }
};
orxU32 imitate_click_behavior::next_click_id = 0x1000000;
BEHAVIOR_LEAF(imitate_click, imitate_click_behavior)

orxVECTOR getCursorPosition(const char * name) {
    auto manager = pancarDemo::GetInstance().registered_manager;
    auto viewCam = orxViewport_GetCamera(manager->viewport);
    auto name_persistent = Persist(name);
    for(orxU32 gIdx=0; gIdx<orxCamera_GetGroupIDCounter(viewCam); gIdx++) {
        auto gId = orxCamera_GetGroupID(viewCam, gIdx);
        for(auto obj = orxObject_GetNext(nullptr, gId); obj; obj = orxObject_GetNext(obj, gId)) {
            if(orxObject_GetName(obj) == name_persistent) {
                auto pos = GetPosition(obj);
                orxVECTOR screenPos;
                orxRender_GetScreenPosition(&pos, manager->viewport, &screenPos);
                orxVECTOR cursorPos;
                orxRender_GetWorldPosition(&screenPos, GetCursorViewport(), &cursorPos);
                cursorPos.fZ = 0;
                return cursorPos;
            }
        }
    }
    return {0,0,0};
}
RETURNING_ACTION_LEAF(get_cursor_position, getCursorPosition)

orxSTATUS isCurrentScreenName(const char * name) {
    bool result = strcmp(pancarDemo::GetInstance().level->GetScreenName(), name) == 0;
    return result ? orxSTATUS_SUCCESS : orxSTATUS_FAILURE;
}
ACTION_LEAF(is_current_screen_name, isCurrentScreenName)

class TestCursor: public behavior_context_mixin<TestCursor> {
    SCROLL_BIND_CLASS("TestCursor")
public:
    TestCursor() {GetCursorViewport();}
};

auto click = any {
    timeout(0.4),
    imitate_click($o("^"))
};

behavior_constructor move_to_object(const char * name) {
    return sequence {
        get_cursor_position($v("Target"), name),
        move($o("^"), $v("Target"), 400)
    };
}

behavior_constructor click_object(const char * name) {
    return sequence {
        move_to_object(name),
        click,
        timeout(0.1),
    };
}

behavior_constructor wait_for_screen(const char * name) {
    return wait_for_success <<= is_current_screen_name(name);
}

auto test_cursor_behavior = sequence {
    timeout(0.1),
    screenshot_capture(),
    wait_for_screen("OpeningMenuScreen"),
    screenshot_capture(),
    click_object("SandboxButton"),
    screenshot_capture(),
    click_object("CallCheeseIcon"),
    screenshot_capture(),
    for_range($i("i"), 0, 3) <<= click_object("CallCrowIcon"),
    screenshot_capture(),
    destroy($o("^")),
};
REGISTER_BEHAVIOR(test_cursor_behavior, "test_cursor")
