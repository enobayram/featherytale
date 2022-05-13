/*
 * HollowFrame.cpp
 *
 *  Created on: Dec 7, 2015
 *      Author: eba
 */

#include <vector>

#include <orx_utilities.h>
#include <util/render_utils.h>
#include <gameObjects/HollowFrame.h>

void HollowFrame::ExtOnCreate() {
    if(orxConfig_HasValue("Thickness")) {
        hThickness = vThickness = orxConfig_GetFloat("Thickness");
    } else {
        hThickness = orxConfig_GetFloat("HThickness");
        vThickness = orxConfig_GetFloat("VThickness");
    }
}

orxBOOL HollowFrame::OnRender(orxRENDER_EVENT_PAYLOAD* _pstPayload) {
    const orxBITMAP * _pstBitmap = getRenderBitmap(GetOrxObject());
    auto graphic = orxObject_GetWorkingGraphic(GetOrxObject());
    auto texture = orxObject_GetWorkingTexture(GetOrxObject());

    orxVECTOR origin, size, pivot;
    orxGraphic_GetOrigin(graphic, &origin);
    orxGraphic_GetSize(graphic, &size);
    orxGraphic_GetPivot(graphic, &pivot);

    orxFLOAT l = 0, r=size.fX, t = 0, b = size.fY;

    std::vector<orxDISPLAY_VERTEX> astVertexList;

    auto tMat = calculateMatrix(*_pstPayload->stObject.pstTransform);

    orxFLOAT tw, th;
    orxTexture_GetSize(texture, &tw, &th);
    auto t2uv = [&](orxVECTOR v) {
        return orxVECTOR{v.fX/size.fX, v.fY/size.fY};
    };

    auto t2w = [&](orxVECTOR v) {
        return v-pivot;
    };

    orxRGBA color;
    if(orxGraphic_HasColor(graphic)) {
        orxCOLOR color_;
        orxGraphic_GetColor(graphic, &color_);
        color = orxColor_ToRGBA(&color_);
    } else color = orxRGBA_Set(255,255,255,255);

    auto pushVertex = [&](orxVECTOR v) {
        auto w = t2w(v);
        auto uv = t2uv(v);
        auto viewPos = transform(tMat, w.fX, w.fY);
        orxDISPLAY_VERTEX vertex = {viewPos.fX,viewPos.fY,uv.fX,uv.fY,color};
        astVertexList.push_back(vertex);
    };

    pushVertex({l          ,b});
    pushVertex({l+hThickness,b-vThickness});
    pushVertex({l          ,t});
    pushVertex({l+hThickness,t+vThickness});
    pushVertex({r          ,t});
    pushVertex({r-hThickness,t+vThickness});
    pushVertex({r          ,b});
    pushVertex({r-hThickness,b-vThickness});
    pushVertex({l          ,b});
    pushVertex({l+hThickness,b-vThickness});

    auto smoothing = orxObject_GetSmoothing(GetOrxObject());
    auto blend_mode = orxObject_GetBlendMode(GetOrxObject());

    orxDisplay_DrawMesh(_pstBitmap, smoothing, blend_mode, astVertexList.size(), astVertexList.data());

    return orxSTATUS_FAILURE;
}
