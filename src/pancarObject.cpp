/*
 * pancarObject.cpp
 *
 *  Created on: Apr 13, 2013
 *      Author: eba
 */

#include <vector>
#include <random>

#include <Eigen/Dense>

#include <util/render_utils.h>

#include "pancarObject.h"
#include "constants.h"
#include <gameObjects/screens/GameLevel.h>
#include "visualElements/Triangle.hpp"

using namespace Eigen;
using namespace std;

pancarObject::pancarObject():mappedTexture() {

}

void pancarObject::ExtOnCreate ()
{

}

void pancarObject::OnDelete ()
{
    for(Node * node: mappedTexture->getNodes()) world->removeNode(node);
}

void pancarObject::Update(const orxCLOCK_INFO &_rstInfo)
{
    if(mappedTexture->empty()) SetLifeTime(0);
}

orxVECTOR pancarObject::GetFirstNodePosition() {
    auto nodes = mappedTexture->getNodes();
    if(nodes.empty()) return {0,0,0};
    auto node = nodes[0];
    return {(orxFLOAT)node->pos.x(), (orxFLOAT)node->pos.y(), GetPosition().fZ};
}

orxVECTOR pancarObject::GetCenterOfMass() {
    auto nodes = mappedTexture->getNodes();
    if(nodes.empty()) return {0,0,0};
    auto size = nodes.size();
    Vector2d result{0,0};
    for(Node * node: nodes) {
        result += node->pos/size;
    }
    return {(orxFLOAT)result.x(), (orxFLOAT)result.y(), GetPosition().fZ};
}

std::vector<Node*> pancarObject::GetNodes() {
    return mappedTexture->getNodes();
}

orxBOOL pancarObject::OnRender(orxRENDER_EVENT_PAYLOAD *_pstPayload){
	const orxBITMAP * _pstBitmap = getRenderBitmap(GetOrxObject());

	mappedTexture->updateInternalState();

	vector<orxDISPLAY_VERTEX> astVertexList;

	auto tMat = calculateMatrix(*_pstPayload->stObject.pstTransform);

	auto pushPinning = [&](const Pinning & p)
	{
		const Vector2f &xy = p.wldC.cast<float>(), &uv = p.texC.cast<float>();
		auto viewPos = transform(tMat, xy.x(), xy.y());
        orxDISPLAY_VERTEX vertex = {viewPos.fX,viewPos.fY,uv.x(),uv.y(),orx2RGBA(0xFF*p.isNode, 0x0, 0x0, 0xFF)};
		astVertexList.push_back(vertex);
	};

	mappedTexture->Triangles->applyToVertices([&](const PinningTriplet & pinningTriplet) {
		pushPinning(pinningTriplet[0]);
		for(const Pinning &p:pinningTriplet) {
			pushPinning(p);
		}
		pushPinning(pinningTriplet[2]);
	});

	if(astVertexList.empty()) null_draw(_pstBitmap);
	int displayedIndex = 0;
	int maxVertices = 2048;
	while(displayedIndex< (int)astVertexList.size()) {
		int displayStart = max(displayedIndex-2,0);
		int batchSize = min(maxVertices, (int) astVertexList.size()-displayStart);
		orxDisplay_DrawMesh(_pstBitmap, orxDISPLAY_SMOOTHING_ON, orxDISPLAY_BLEND_MODE_ALPHA, batchSize, astVertexList.data()+displayStart);
		displayedIndex=displayStart+batchSize;
	}
	return orxSTATUS_FAILURE;
}

void pancarObject::SetContext(ExtendedMonomorphic & context) {
    world = context.GetAspect<game_context>()->world.get();
    CreateMappedTexture();
}
