/*
 * pancarObject.h
 *
 *  Created on: Apr 13, 2013
 *      Author: eba
 */

#ifndef PANCAROBJECT_H_
#define PANCAROBJECT_H_

#include <memory>
#include <vector>

#include <orx.h>
#include <scroll_ext/ExtendedObject.h>
#include <orxFSM/behavior_header.hpp>

#include "visualElements/mappedTexture.h"

class pancarObject: public ExtendedMonomorphic {
protected:
	std::unique_ptr<MappedTexture> mappedTexture;
	World * world = nullptr;
public:
	pancarObject();
	void SetContext(ExtendedMonomorphic &);
	orxVECTOR GetFirstNodePosition();
	orxVECTOR GetCenterOfMass();
	std::vector<Node *> GetNodes();
private:
    //! Called on object creation.
    virtual void ExtOnCreate ();
    //! Called on object deletion
    virtual void OnDelete ();
    //! Called on clock update
    virtual void Update (const orxCLOCK_INFO &_rstInfo);

    virtual orxBOOL OnRender(orxRENDER_EVENT_PAYLOAD *_pstPayload);

    virtual void CreateMappedTexture() = 0;
};

ACTION_LEAF(get_center_of_mass, &pancarObject::GetCenterOfMass)
ACTION_LEAF(get_first_node_position, &pancarObject::GetFirstNodePosition)

#endif /* PANCAROBJECT_H_ */
