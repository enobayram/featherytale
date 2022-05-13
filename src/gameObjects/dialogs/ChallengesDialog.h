/*
 * ChallengesDialog.h
 *
 *  Created on: Dec 8, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_CHALLENGESDIALOG_H_
#define GAMEOBJECTS_CHALLENGESDIALOG_H_

#include <scroll_ext/ExtendedObject.h>

class ChallengesDialog: public ExtendedMonomorphic {
	orxOBJECT * tabs[2];
	orxOBJECT * content = nullptr;
public:
	SCROLL_BIND_CLASS("ChallengesDialog")
    void ExtOnCreate();
    void SetTab(int tabIndex);
};

#endif /* GAMEOBJECTS_CHALLENGESDIALOG_H_ */
