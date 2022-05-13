/*
 * LevelEndDialog.h
 *
 *  Created on: Aug 11, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_LEVELENDDIALOG_H_
#define GAMEOBJECTS_LEVELENDDIALOG_H_

#include <scroll_ext/ExtendedObject.h>
#include <orxFSM/behavior_header.hpp>

#include <gameObjects/aspects.h>

class ScoreDisplay;
class GameLevel;

class LevelEndDialog: public button_container, public behavior_context_mixin<LevelEndDialog> {
public:
    enum TrophyType {EMPTY, OLD, NEW};
private:
    orxOBJECT * trophies[3];
    orxOBJECT * challenge_trophies[2] = {nullptr};
    orxOBJECT * curFiller;
    orxVECTOR frameSize, firstFramePos, levelScoreDispPos;
    orxFLOAT displayed_score = 0;
    orxFLOAT score_target = 0;
    bool successLayout = false;
    int thresholds[3];
    orxOBJECT * createFiller(int index);
    void createTrophy(TrophyType type, int index);
    void newTrophy(orxOBJECT *& storage, TrophyType type, orxVECTOR pos);
public:
    ScoreDisplay * scoreDisplay;
    SCROLL_BIND_CLASS("LevelEndDialog")
    void SetContext(ExtendedMonomorphic & context);
    void AddButton(orxOBJECT *, orxFLOAT xPos);
    void RemoveButton(orxOBJECT *);
    void UpdateFillers(orxFLOAT new_score);
    void createChallengeTrophy(TrophyType type, int index);
    void createSuccessLayout(GameLevel * level);
    void createFailureLayout(GameLevel * level, std::string failure_reason);
    void Update(const orxCLOCK_INFO & clock);
};

#endif /* GAMEOBJECTS_LEVELENDDIALOG_H_ */
