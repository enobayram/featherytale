/*
 * InMemoryTexture.cpp
 *
 *  Created on: Oct 26, 2015
 *      Author: eba
 */

#include <map>
#include <string>
#include <iostream>

#include <scroll_ext/InMemoryTexture.h>

std::map<orxTEXTURE *, std::vector<std::function<void(InMemoryTexture)>>> pending_receivers;

void init_in_memory_texture() {
    pending_receivers.clear();
    orxEvent_AddHandler(orxEVENT_TYPE_TEXTURE, inMemoryTextureLoadHandler);
}

void push_texture(orxTEXTURE * tex, std::function<void(InMemoryTexture)> receiver) {
    auto bitmap = orxTexture_GetBitmap(tex);
    orxFLOAT w,h;
    orxDisplay_GetBitmapSize(bitmap, &w, &h);
    std::vector<orxU32> data(w*h);
    orxDisplay_GetBitmapData(bitmap, (orxU8 *) data.data(), data.size()*sizeof(orxU32));
    receiver({data,(uint) w, (uint) h});
}

void get_texture(const char * imagename, std::function<void(InMemoryTexture)> receiver) {
    auto tex = orxTexture_CreateFromFile(imagename);
    pending_receivers[tex].push_back(receiver);
}

orxSTATUS orxFASTCALL inMemoryTextureLoadHandler(const orxEVENT *_pstEvent) {
    auto payload = (orxDISPLAY_EVENT_PAYLOAD *) _pstEvent->pstPayload;
    if(_pstEvent->eID==orxTEXTURE_EVENT_LOAD) {
        auto tex = (orxTEXTURE *) _pstEvent->hSender;
        if(pending_receivers.find(tex) == pending_receivers.end()) return orxSTATUS_SUCCESS;
        for(auto & receiver: pending_receivers[tex]) {
            push_texture(tex, receiver);
        }
    }
    return orxSTATUS_SUCCESS;
}
