/*
 * Dialog.h
 *
 *  Created on: May 24, 2015
 *      Author: eba
 */

#ifndef GAMEOVERDIALOG_H_
#define GAMEOVERDIALOG_H_

#include <scroll_ext/ExtendedObject.h>

#include "../aspects.h"

class Dialog: public ExtendedMonomorphic, public button_container {
public:
    SCROLL_BIND_CLASS("Dialog")
    void AddButton(orxOBJECT *, orxFLOAT xPos);
    void RemoveButton(orxOBJECT *);
};

#endif /* GAMEOVERDIALOG_H_ */
