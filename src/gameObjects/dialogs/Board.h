/*
 * Board.h
 *
 *  Created on: May 29, 2015
 *      Author: enis
 */

#ifndef GAMEOBJECTS_BOARD_H_
#define GAMEOBJECTS_BOARD_H_

#include <vector>

#include <scroll_ext/ExtendedObject.h>
#include <orxFSM/behavior_header.hpp>

#include "../aspects.h"

struct ButtonEntry;

class Board: public ExtendedMonomorphic, public button_container {
    std::vector<orxOBJECT *> pieces;
    std::vector<ButtonEntry> buttons;
    orxOBJECT * content = nullptr;
    orxOBJECT * frame = nullptr;
    orxVECTOR textureSize;
    void arrange_buttons();
public:
    SCROLL_BIND_CLASS("Board")
	Board();
	virtual ~Board();
	void ExtOnCreate();
	void SetContent(orxOBJECT *);
	void AddButton(orxOBJECT *, orxFLOAT xPos);
	void RemoveButton(orxOBJECT *);
	orxVECTOR GetBoardSize();
};

orxSTATUS set_content_action(Board * board, std::string section);
ACTION_LEAF(set_content, set_content_action)

#endif /* GAMEOBJECTS_BOARD_H_ */
