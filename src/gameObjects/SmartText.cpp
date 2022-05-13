/*
 * SmartText.cpp
 *
 *  Created on: May 29, 2015
 *      Author: enis
 */

#include <iostream>
#include <string>
#include <vector>

#include <gameObjects/SmartText.h>
#include <boost/algorithm/string.hpp>

using namespace std;

SmartText::SmartText() {
	// TODO Auto-generated constructor stub

}

orxFLOAT getCharacterHeight(orxOBJECT * obj) {
	orxGRAPHIC * graphic = orxOBJECT_GET_STRUCTURE(obj,GRAPHIC);
	orxTEXT * text = orxTEXT(orxGraphic_GetData(graphic));
	orxFONT * font = orxFONT(orxText_GetFont(text));
	return orxFont_GetCharacterHeight(font);
}

void SmartText::ExtOnCreate() {
	ProcessText();
}

SmartText::~SmartText() {
	// TODO Auto-generated destructor stub
}

void SmartText::SetText(const char* text) {
	orxObject_SetTextString(GetOrxObject(), text);
	ProcessText();
}

void SmartText::ProcessText() {
	ConfigSectionGuard guard(GetModelName());
	auto charH = getCharacterHeight(GetOrxObject());
	auto lineH = orxConfig_GetFloat("LineHeight");
	string content = orxObject_GetTextString(GetOrxObject());
	vector<string> words;
	boost::split(words, content , boost::is_any_of("\t "));

	SetScale({lineH/charH, lineH/charH,0});
	auto maxWidth = charH/lineH * orxConfig_GetFloat("TextWidth");
	orxVECTOR size;
	string curstring = words.front();
	words.erase(words.begin());
	orxObject_SetTextString(GetOrxObject(), curstring.c_str());
	for(auto word: words) {
		auto sameLine = curstring+ " " + word;
		auto newLine = curstring+ "\n" + word;
		orxObject_SetTextString(GetOrxObject(), sameLine.c_str());
		if(orxObject_GetSize(GetOrxObject(), &size)->fX<maxWidth) {
			curstring = sameLine;
		} else {
			orxObject_SetTextString(GetOrxObject(), newLine.c_str());
			curstring = newLine;
		}
	}
}
