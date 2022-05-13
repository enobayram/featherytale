/*
 * view_utilities.h
 *
 *  Created on: Dec 7, 2015
 *      Author: eba
 */

#ifndef UTIL_VIEW_UTILITIES_H_
#define UTIL_VIEW_UTILITIES_H_

#include <render/orxCamera.h>

inline void adjustFrustumForArea(orxVIEWPORT * viewport, orxFLOAT desiredW, orxFLOAT desiredH) {
    orxFLOAT viewWidth, viewHeight;
    orxViewport_GetSize(viewport,&viewWidth, &viewHeight);
    auto camera = orxViewport_GetCamera(viewport);
    orxFLOAT magnification = std::max(desiredW/viewWidth, desiredH/viewHeight);
    orxCamera_SetFrustum(camera, viewWidth*magnification, viewHeight*magnification, -1,2);
}



#endif /* UTIL_VIEW_UTILITIES_H_ */
