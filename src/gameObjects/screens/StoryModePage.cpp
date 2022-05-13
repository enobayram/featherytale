/*
 * StoryModePage.cpp
 *
 *  Created on: Dec 6, 2015
 *      Author: eba
 */

#include <game_state.h>
#include <orx_utilities.h>
#include <gameObjects/screens/StoryModePage.h>
#include <gameObjects/Button.h>
#include <sstream>

#include <orxFSM/simple_behaviors.hpp>

const int rows = 2;
orxFLOAT rowYs[rows] = {-100, 105};

const int cols = 4;
orxFLOAT colXs[cols] = {-255, -92, 92, 255};

enum ScreenType {Level, Cutscene};

bool completed(const level_data & history) {
    for(auto t:history.score_levels) if(!t) return false;
    for(auto t:history.challenges) if(!t) return false;
    return true;
}

orxOBJECT * CreateLevelThumbnail(StoryModePage * page, const char * level_name) {
    ConfigSectionGuard guard(level_name);
    if(is_level_locked(level_name)) {
        auto thumbnail = page->CreateChild("LockedLevelThumbnail");
        auto lock_text = FindOwnedChild(thumbnail, "ThumbnailHeartNeededText");
        std::stringstream ss;
        ss << getTotalHearts() << " / " << orxConfig_GetU32("MinHearts");
        orxObject_SetTextString(lock_text, ss.str().c_str());
        return thumbnail;
    } else if(is_level_playable(level_name)) {
        auto history = fetchLevelData(level_name);
        const char * thumbnail_type = completed(history) ? "GoldenLevelThumbnail" : "AvailableLevelThumbnail";
        auto thumbnail = page->CreateChild<Button>(thumbnail_type);
        const char * trophy_full = "ThumbnailTrophyFull";
        const char * trophy_empty = "ThumbnailTrophyEmpty";
        for(int i=0; i<3; i++) {
            auto trophy_section = history.score_levels[i]?trophy_full:trophy_empty;
            auto trophy = thumbnail->CreateChild(trophy_section);
            orxVECTOR pos{float(i-1)*40, 50, -0.01f};
            orxObject_SetPosition(trophy, &pos);
        }
        for(int i=0; i<2; i++) {
            auto trophy_section = history.challenges[i]?trophy_full:trophy_empty;
            auto trophy = thumbnail->CreateChild(trophy_section);
            orxVECTOR pos{float(i-0.5)*40, 80, -0.01f};
            orxObject_SetPosition(trophy, &pos);
        }
        auto text = thumbnail->CreateChild("ThumbnailText");
        auto level_display_name = sprint_action(orxLocale_GetString("LevelNameFormat"), orxConfig_GetU32("LevelNumber"));
        orxObject_SetTextString(text, level_display_name.c_str());
        thumbnail->set_handler([page, level_name]{
                page->SetNextScreen(level_name);
        });
        thumbnail->CreateChild(orxConfig_GetString("Thumbnail"));

        return thumbnail->GetOrxObject();
    } else /*dependent not finished*/ {
        return page->CreateChild("DisabledLevelThumbnail");
    }
}

orxOBJECT * CreateCutsceneThumbnail(StoryModePage * page, const char * screen) {
    ConfigSectionGuard guard(screen);
    if(!is_level_playable(screen)) return page->CreateChild("DisabledCutsceneThumbnail");
    auto thumbnail = page->CreateChild<Button>("AvailableCutsceneThumbnail");
    thumbnail->set_handler([page, screen]{
        page->SetNextScreen(screen);
    });
    thumbnail->CreateChild(orxConfig_GetString("Thumbnail"));
    auto text = thumbnail->CreateChild("ThumbnailText");
    auto cutscene_display_name = sprint_action(orxLocale_GetString("CutSceneNameFormat"), orxConfig_GetU32("CutSceneNumber"));
    orxObject_SetTextString(text, cutscene_display_name.c_str());

    return thumbnail->GetOrxObject();
}

void create_change_page_button(StoryModePage * page, const char * page_key, const char * button_name){
    if(orxConfig_HasValue(page_key)) {
        auto button = page->CreateChild<Button>(button_name);
        const char * target_page = orxConfig_GetString(page_key);
        button->set_handler([target_page, page]{
            page->SetNextScreen(target_page);
        });
    }
}

void StoryModePage::ExtOnCreate() {
    SimpleScreen::ExtOnCreate();
    auto numScreens = orxConfig_GetListCounter("Screens");
    for(int row=0; row<rows; ++row) {
        for(int col=0; col<cols; ++col) {
            int screenIndex = row*cols + col;
            if(screenIndex >= numScreens) break;
            auto screen = ::GetValue<const char *>(orxNULL, "Screens", screenIndex);
            auto screen_type = ::GetValue<std::string>(screen, "ScreenType") == "Level" ? Level : Cutscene;
            orxOBJECT * thumbnail = 0;
            switch(screen_type) {
            case Level:
                thumbnail = CreateLevelThumbnail(this, screen);
                break;
            case Cutscene:
                thumbnail = CreateCutsceneThumbnail(this, screen);
                break;
            }
            if(!thumbnail) continue;
            orxVECTOR pos{colXs[col], rowYs[row], 0};
            orxObject_SetPosition(thumbnail, &pos);
        }
    }
    create_change_page_button(this, "NextPage", "NextPageButton");
    create_change_page_button(this, "PreviousPage", "PreviousPageButton");
}
