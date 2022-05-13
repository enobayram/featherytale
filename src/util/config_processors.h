/*
 * config_processors.h
 *
 *  Created on: Sep 24, 2015
 *      Author: eba
 */

#ifndef UTIL_CONFIG_PROCESSORS_H_
#define UTIL_CONFIG_PROCESSORS_H_

#include <object/orxObject.h>

#ifdef __cplusplus
extern "C" {
#endif

void ProcessAnimGraph(const char * section);

void RegisterProcessAnimGraphCommand();

const orxSTRING orxObjectX_GetAnimForTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag);

orxSTATUS orxObjectX_SetCurrentAnimByTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag);

orxSTATUS orxObjectX_SetTargetAnimByTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag);

orxBOOL orxObjectX_CurrentAnimHasTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag);

orxBOOL orxObjectX_TargetAnimHasTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag);

#ifdef __cplusplus
}
#endif

#endif /* UTIL_CONFIG_PROCESSORS_H_ */
