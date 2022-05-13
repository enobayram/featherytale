/*
 * SlowTimeTool.h
 *
 *  Created on: Sep 11, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_SLOWTIMETOOL_H_
#define GAMEOBJECTS_SLOWTIMETOOL_H_

#include "Tool.h"

class Button;
class game_screen;

class SlowTimeTool: public Tool {
    orxVIEWPORT * viewport;
    orxFLOAT slownessCoeff = 0;
    void setCoeff(orxFLOAT new_coeff);
public:
    SCROLL_BIND_CLASS("SlowTimeTool")
    virtual void SetContext(ExtendedMonomorphic & context);
    virtual void Update(const orxCLOCK_INFO &_rstInfo);
    virtual void OnDelete();
    virtual void OnPause(bool pause);
};

#endif /* GAMEOBJECTS_SLOWTIMETOOL_H_ */
