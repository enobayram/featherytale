/*
 * TrailEffect.cpp
 *
 *  Created on: Aug 7, 2015
 *      Author: enis
 */

#include <util/render_utils.h>
#include <gestures.h>

#include <gameObjects/TrailEffect.h>
#include <cmath>

using namespace std;

TrailEffect::TrailEffect() {
    // TODO Auto-generated constructor stub

}

TrailEffect::~TrailEffect() {
    // TODO Auto-generated destructor stub
}

Vector2d vec(const trace & trc) {
    return {trc.x,trc.y};
}

orxBOOL TrailEffect::OnRender(orxRENDER_EVENT_PAYLOAD *_pstPayload){
    const orxBITMAP * _pstBitmap = getRenderBitmap(GetOrxObject());

    auto tMat = calculateMatrix(*_pstPayload->stObject.pstTransform);

    vector<orxDISPLAY_VERTEX> astVertexList;
    orxDOUBLE now = orxSystem_GetTime();
    orxCOLOR objColor;
    GetColor(objColor);

    auto effectLifetime = GetValue<orxFLOAT>("EffectLifetime");
    auto effectWidth = GetValue<orxFLOAT>("EffectWidth");

    for(auto it = t->traces.begin(); it != t->traces.end(); ++it) {
        const trace & trc = *it;
        if(trc.t<now-effectLifetime-0.3) continue;
        orxFLOAT segmentAge = orxFLOAT(now-trc.t)/effectLifetime;
        Vector2d delta{0,0};
        int weight=0;
        if(it-1 >= t->traces.begin()) {
            const trace & ptrc = *(it-1);
            delta += (vec(trc) - vec(ptrc)).normalized();
            weight++;
        }
        if(it+1 < t->traces.end()) {
            const trace & ntrc = *(it+1);
            delta += (vec(ntrc) - vec(trc)).normalized();
            weight++;
        }
        if(weight) delta /= weight;
        delta *= effectWidth/2;
        auto viewPos1 = transform(tMat, trc.x-delta.y(), trc.y+delta.x());
        orxDISPLAY_VERTEX vertex1 = {viewPos1.fX,viewPos1.fY,segmentAge,0,orxColor_ToRGBA(&objColor)};

        auto viewPos2 = transform(tMat, trc.x+delta.y(), trc.y-delta.x());
        orxDISPLAY_VERTEX vertex2 = {viewPos2.fX,viewPos2.fY,segmentAge,1,orxColor_ToRGBA(&objColor)};
        astVertexList.push_back(vertex1);
        astVertexList.push_back(vertex2);
    }

    if(astVertexList.size()<=2) null_draw(_pstBitmap);
    else {
        size_t displayedIndex = 0;
        size_t maxVertices = 2048;
        while(displayedIndex<astVertexList.size()) {
            size_t displayStart = max(int(displayedIndex)-2,0);
            size_t batchSize = min(maxVertices, astVertexList.size()-displayStart);
            orxDisplay_DrawMesh(_pstBitmap, orxDISPLAY_SMOOTHING_ON, orxDISPLAY_BLEND_MODE_ADD, batchSize, astVertexList.data()+displayStart);
            displayedIndex=displayStart+batchSize;
        }
    }
    return orxSTATUS_FAILURE;
}
