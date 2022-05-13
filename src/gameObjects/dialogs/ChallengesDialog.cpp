/*
 * ChallengesDialog.cpp
 *
 *  Created on: Dec 8, 2015
 *      Author: enis
 */

#include <orx_utilities.h>
#include "ChallengesDialog.h"
#include <gameObjects/Button.h>
#include <gameObjects/SmartText.h>
#include <pancarDemo.h>
#include <util/config_processors.h>
#include <game_state.h>
#include <sstream>

void ChallengesDialog::ExtOnCreate() {
	auto current_level = pancarDemo::GetInstance().level->GetScreenName();
	auto history = fetchLevelData(current_level);

	for(int i=0; i<2; ++i) {
		auto tab = CreateChild<Button>(orxConfig_GetListString("Tabs", i), true);
		tab->set_handler([this,i]{
			SetTab(i);
		});
		auto tabIconType = history.challenges[i] ? "CompletedChallengeIcon" : "UncompletedChallengeIcon";
		tab->CreateChild(tabIconType, true);
		tabs[i] = tab->GetOrxObject();
	}
	SetTab(0);
}

void ChallengesDialog::SetTab(int tabIndex) {
	auto current_level = pancarDemo::GetInstance().level->GetScreenName();

	for(int i=0; i<2; ++i) {
		orxObjectX_SetCurrentAnimByTag(tabs[i], i == tabIndex ? "Selected" : "Unselected");
	}
	if(content) orxObject_SetLifeTime(content, 0);

	std::stringstream ss;
	ss << "Challenge" << (tabIndex+1);
	auto challenge_key = ss.str();
	auto challenge_explanation = Localize(::GetValue<const char * >(current_level, challenge_key.c_str()));
	auto content_ = CreateChild<SmartText>("ChallengeExplanationText",true);
	content_->SetText(challenge_explanation);
	content = content_->GetOrxObject();
}
