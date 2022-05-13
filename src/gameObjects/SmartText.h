/*
 * SmartText.h
 *
 *  Created on: May 29, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_SMARTTEXT_H_
#define GAMEOBJECTS_SMARTTEXT_H_

#include <scroll_ext/ExtendedObject.h>

class SmartText: public ExtendedMonomorphic {
public:
    SCROLL_BIND_CLASS("SmartText")
	void ExtOnCreate();
	SmartText();
	virtual ~SmartText();
	void SetText(const char * text);
	void ProcessText();
};

#endif /* GAMEOBJECTS_SMARTTEXT_H_ */
