/*
 * SimpleScreen.cpp
 *
 *  Created on: Nov 8, 2015
 *      Author: eba
 */

#include <gameObjects/screens/SimpleScreen.h>
#include <gesturemanager.h>
#include <util/view_utilities.h>

SimpleScreen::SimpleScreen() {
}

SimpleScreen::~SimpleScreen() {
}

void SimpleScreen::ExtOnCreate() {
    viewport = createAdjustedViewport(GetValue<const char * >("Viewport"));
    manager.reset(new game_gesture_manager(viewport));
}

void SimpleScreen::SetOutputTexture(orxTEXTURE* texture) {
    orxViewport_SetTextureList(viewport, 1, &texture);
}

void SimpleScreen::EndScreen() {
    SetLifeTime(0);
    orxViewport_Delete(viewport);
}

const char* SimpleScreen::GetNextScreen() {
    return next_screen;
}

void SimpleScreen::SetNextScreen(const char* screen) {
    next_screen = screen;
}

gesture_manager& SimpleScreen::GetGestureManager() {
    return *manager;
}

orxVIEWPORT* createAdjustedViewport(const char* section) {
    orxVIEWPORT * viewport = orxViewport_CreateFromConfig(section);
    auto desired = ::GetValue<orxVECTOR>(section, "CentralRegion");
    adjustFrustumForArea(viewport, desired.fX, desired.fY);
    return viewport;
}
