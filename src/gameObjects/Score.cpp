/*
 * Score.cpp
 *
 *  Created on: May 21, 2015
 *      Author: enis
 */

#include <gameObjects/Score.h>
#include <scroll_ext/config_utils.hpp>
#include <orx_utilities.h>
#include <util/orx_eigen.h>

using namespace std;

ScoreDisplay::ScoreDisplay() {
	// TODO Auto-generated constructor stub
}

ScoreDisplay::~ScoreDisplay() {
	// TODO Auto-generated destructor stub
}

void ScoreDisplay::display(int score) {
    orxObject_SetTextString(text, to_string(score).c_str());
}

void ScoreDisplay::ExtOnCreate() {
	text = orxObject_GetOwnedChild(GetOrxObject());
	display(displayedScore);
	glow = orxObject_GetOwnedSibling(text);
}

void create_score_increment_text(orxOBJECT * owner, int delta, const orxVECTOR & pos) {
    auto obj = CreateChild(owner, "ScoreIncrementText", false, false);
    orxObject_SetPosition(obj, &pos);
    std::stringstream scoretext;
    scoretext << "+" << delta;
    orxObject_SetTextString(obj, scoretext.str().c_str());
}

void ScoreDisplay::addScore(int stars, const orxVECTOR & pos, bool show_text, int score_per_star) {
    assert(stars>0);
    assert(score_per_star>0);
    for(int i=0; i<stars; ++i) {
        auto piece = ScorePiece::Create(pos,this);
        piece->value = score_per_star;
    }
    if(show_text) create_score_increment_text(GetOrxObject(), stars, pos);
}

void ScoreDisplay::setScore(int score) {
    displayedScore = score;
    display(displayedScore);
}

void ScoreDisplay::pieceArrived(int increment) {
    displayedScore += increment;
    display(displayedScore);
    orxObject_AddUniqueFX(GetOrxObject(),"ScoreLogoFX");
    orxObject_AddUniqueFX(glow, "ScoreGlowFX");
    AddSound("PointStarArrivedSound");
    fire_event<score_arrived>(increment);
}


void ScorePiece::Update(const orxCLOCK_INFO &_rstInfo) {
    auto curpos = toEigen2f(GetPosition(true));
    auto tarpos = toEigen2f(target->GetPosition(true));
    if((tarpos-curpos).norm() < arrive_radius * target->GetScale(true).fX) {
        target->pieceArrived(value);
        SetLifeTime(0);
    } else {
        Eigen::Vector2f deltaV = (tarpos-curpos).normalized()*acceleration*_rstInfo.fDT;
        orxVECTOR curSpeed_;
        GetSpeed(curSpeed_);
        Eigen::Vector2f curSpeed = toEigen2f(curSpeed_);
        Eigen::Vector2f newSpeed = curSpeed*powf(damping, _rstInfo.fDT) + deltaV;
        SetSpeed({newSpeed.x(), newSpeed.y(), 0});
    }
}

ScorePiece * ScorePiece::Create(const orxVECTOR & pos, ScoreDisplay * target) {
    auto posWithZ = pos;
    posWithZ.fZ = target->GetPosition().fZ;
    auto result = target->CreateChild<ScorePiece>(target->GetValue<const char *>("Piece"), true, false);
    result->target = target;
    orxObject_SetWorldPosition(result->GetOrxObject(), &posWithZ);
    result->SetScale(piecewiseMult(target->GetScale(true), result->GetScale()));
    orxVECTOR speed;
    result->SetSpeed(result->GetValue<orxVECTOR>("Speed"), true);
    result->acceleration *= target->GetScale(true).fX;
    return result;
}

void ScorePiece::ExtOnCreate() {
    acceleration = orxConfig_GetFloat("HomingAcceleration");
    damping = orxConfig_GetFloat("HomingDamping");
    arrive_radius = orxConfig_GetFloat("ArriveRadius");
}
