/*
 * render_utils.h
 *
 *  Created on: Dec 7, 2015
 *      Author: eba
 */

#ifndef UTIL_RENDER_UTILS_H_
#define UTIL_RENDER_UTILS_H_

#include <display/orxDisplay.h>
#include <object/orxObject.h>

struct TransformationMatrix {
    orxVECTOR vX;
    orxVECTOR vY;
};

static orxINLINE TransformationMatrix calculateMatrix(const orxDISPLAY_TRANSFORM & t)
{
  orxFLOAT fCos, fSin, fSCosX, fSCosY, fSSinX, fSSinY, fTX, fTY;

  /* Has rotation? */
  if(t.fRotation != orxFLOAT_0)
  {
    /* Gets its cos/sin */
    fCos = orxMath_Cos(t.fRotation);
    fSin = orxMath_Sin(t.fRotation);
  }
  else
  {
    /* Inits cos/sin */
    fCos = orxFLOAT_1;
    fSin = orxFLOAT_0;
  }

  /* Computes values */
  fSCosX = t.fScaleX * fCos;
  fSCosY = t.fScaleY * fCos;
  fSSinX = t.fScaleX * fSin;
  fSSinY = t.fScaleY * fSin;
  fTX = t.fDstX - (t.fSrcX *fSCosX) + (t.fSrcY *fSSinY);
  fTY = t.fDstY - (t.fSrcX *fSSinX) - (t.fSrcY *fSCosY);

  TransformationMatrix result;
  /* Updates matrix */
  orxVector_Set(&(result.vX), fSCosX, -fSSinY, fTX);
  orxVector_Set(&(result.vY), fSSinX, fSCosY, fTY);

  /* Done! */
  return result;
}

static orxINLINE orxVECTOR transform(const TransformationMatrix & m, orxFLOAT x, orxFLOAT y) {
    return {m.vX.fX*x+m.vX.fY*y+m.vX.fZ,m.vY.fX*x+m.vY.fY*y+m.vY.fZ,0};
}

// The following function is used to perform a non-draw to work-around the orx bug that
// if no drawing is performed during a custom render, visual glitches appear.
inline void null_draw(const orxBITMAP * _pstBitmap) {
    orxDISPLAY_VERTEX vertices[] = {
         {0,0,0,0,orx2RGBA(0xFF, 0x0, 0x0, 0xFF)}
        ,{0,0,0,0,orx2RGBA(0xFF, 0x0, 0x0, 0xFF)}
        ,{0,0,0,0,orx2RGBA(0xFF, 0x0, 0x0, 0xFF)}
    };
    orxDisplay_DrawMesh(_pstBitmap, orxDISPLAY_SMOOTHING_ON, orxDISPLAY_BLEND_MODE_ALPHA, 3, vertices);
}

inline orxBITMAP * getRenderBitmap(orxOBJECT * obj) {
    orxGRAPHIC * graphic = orxOBJECT_GET_STRUCTURE(obj, GRAPHIC);
    orxTEXTURE * texture = orxTEXTURE(orxGraphic_GetData(graphic));
    return orxTexture_GetBitmap(texture);
}

#endif /* UTIL_RENDER_UTILS_H_ */
