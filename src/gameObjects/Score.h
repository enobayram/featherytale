/*
 * Score.h
 *
 *  Created on: May 21, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_SCORE_H_
#define GAMEOBJECTS_SCORE_H_

#include <scroll_ext/ExtendedObject.h>
#include <scroll_ext/eventdispatcher.h>

struct score_arrived {
    using signature = void(int increment);
};

class ScoreDisplay: public ExtendedMonomorphic, public event_dispatcher {
	int displayedScore = 0;
	orxOBJECT * text = nullptr; //initialized in OnCreate
    orxOBJECT * glow = nullptr; //initialized in OnCreate
	void display(int score);
public:
	SCROLL_BIND_CLASS("ScoreDisplay")
	void ExtOnCreate();
	ScoreDisplay();
	virtual ~ScoreDisplay();
	void addScore(int stars, const orxVECTOR & pos, bool show_text = true, int score_per_star = 1);
	void setScore(int score);
	void pieceArrived(int increment);
};

class ScorePiece: public ExtendedMonomorphic {
    orxFLOAT acceleration, damping, arrive_radius;
public:
    int value = 1;
    SCROLL_BIND_CLASS("ScorePiece")
    ScoreDisplay * target = nullptr;
    virtual void Update(const orxCLOCK_INFO &_rstInfo);
    static ScorePiece * Create(const orxVECTOR & pos, ScoreDisplay * target);
    void ExtOnCreate();
};

#endif /* GAMEOBJECTS_SCORE_H_ */
