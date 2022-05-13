/*
 * Board.cpp
 *
 *  Created on: May 29, 2015
 *      Author: enis
 */

#include "Board.h"
#include <orx_utilities.h>

struct ButtonEntry {
    orxOBJECT * button;
    orxFLOAT locator;
};

// These two are here, otherwise ButtonEntry needs to go the the header file
Board::Board() {}
Board::~Board() {}

#include <iostream>

void Board::ExtOnCreate() {
    frame = orxObject_CreateFromConfig("Empty");
    orxObject_SetParent(frame, GetOrxObject());
    orxObject_SetOwner(frame, GetOrxObject());
    const char * contentSection = orxConfig_GetString("Content");
    auto contentObj = orxObject_CreateFromConfig(contentSection);

    const char * piece = orxConfig_GetString("Piece");

    for(int i=0; i<4; i++) {
        auto child = orxObject_CreateFromConfig(piece);
        orxObject_SetOwner(child,frame);
        orxObject_SetParent(child,frame);
        pieces.push_back(child);
    }

    orxObject_GetSize(pieces[0], &textureSize);

    SetContent(contentObj);
}

void Board::SetContent(orxOBJECT* content) {
    auto oldContent = this->content;
    if(oldContent) {
        orxObject_SetLifeTime(oldContent, 0);
    }

    this->content = content;
    PushConfigSection();
    orxObject_SetOwner(content, frame);
    orxObject_SetParent(content, frame);

    auto textureScale = orxConfig_GetFloat("TextureScale");
    orxVECTOR boardSize = GetBoardSize();
    orxVECTOR pieceSize{textureScale*boardSize.fX/2, textureScale*boardSize.fY/2, 0};

    auto adjustChild = [&](orxOBJECT * child, const orxVECTOR & origin, const orxVECTOR & pivot) {
        orxObject_SetOrigin(child,&origin);
        orxObject_SetPivot(child, &pivot);
        orxObject_SetSize(child, &pieceSize);
        orxVECTOR scale {1/textureScale, 1/textureScale,0};
        orxObject_SetScale(child, &scale);
    };


    adjustChild(pieces[0], {0,0,0}, pieceSize);
    adjustChild(pieces[1],{textureSize.fX-pieceSize.fX,0,0}, {1,pieceSize.fY,0});
    adjustChild(pieces[2],{0,textureSize.fY-pieceSize.fY,0}, {pieceSize.fX,1,0});
    adjustChild(pieces[3],{textureSize.fX-pieceSize.fX,textureSize.fY-pieceSize.fY,0}, {1,1,0});
    PopConfigSection();
    orxVECTOR pivotCorrection{0,boardSize.fY/2,0};
    orxObject_SetPosition(frame, &pivotCorrection);
    arrange_buttons();
}

void Board::AddButton(orxOBJECT* button, orxFLOAT xPos) {
    orxObject_SetOwner(button, frame);
    orxObject_SetParent(button, frame);
    buttons.push_back({button, xPos});
    arrange_buttons();
}

void Board::arrange_buttons() {
    orxVECTOR boardSize = GetBoardSize();
    for(auto entry: buttons) {
        orxVECTOR pos{entry.locator, boardSize.fY/2, -0.01};
        orxObject_SetPosition(entry.button, &pos);
    }
}

void Board::RemoveButton(orxOBJECT* button) {
    for(auto it=buttons.begin(); it!=buttons.end(); it++) {
        if(it->button == button) {
            buttons.erase(it);
            orxObject_SetLifeTime(button,0);
            arrange_buttons();
            return;
        }
    }
}

orxVECTOR Board::GetBoardSize() {
    auto contentSize = ::GetSize(content);
    auto margin = GetValue<orxVECTOR>("Margin");
    return {margin.fX+contentSize.fX, margin.fY+contentSize.fY};
}

orxSTATUS set_content_action(Board * board, std::string section) {
    auto content = orxObject_CreateFromConfig(section.c_str());
    if(!content) return orxSTATUS_FAILURE;

    board->SetContent(content);
    return orxSTATUS_SUCCESS;
}
