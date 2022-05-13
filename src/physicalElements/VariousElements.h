/*
 * VariousElements.h
 *
 *  Created on: Dec 18, 2011
 *      Author: eba
 */

#ifndef VARIOUSELEMENTS_H_
#define VARIOUSELEMENTS_H_

#include <vector>
#include <algorithm>

#include <Eigen/Core>

#include "Node.h"

using namespace Eigen;


class ExternalForce : public SharedCarrier, public boost::intrusive::list_base_hook<>{
	Matrix<double,2,1,DontAlign> force;
	std::vector<Node *> forceNodes;
	friend class World;
	World & world;
public:
	ExternalForce(const std::vector<Node *> & forceNds, const Vector2d &f, World & world): world(world)
	{
		forceNodes=forceNds;
		force = f;
	}
public:
	void setLocation(Vector2d loc);
	const std::vector<Node *> & getNodes() const {
		return forceNodes;
	}
	Vector2d getForce() const {return force;}
	void removeNode(Node * node) {
	    auto it = std::find(forceNodes.begin(),forceNodes.end(), node);
	    if(it != forceNodes.end()) forceNodes.erase(it);
	}
	void remove();
//	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

typedef boost::intrusive::list<ExternalForce> ForceList;



#endif /* VARIOUSELEMENTS_H_ */
