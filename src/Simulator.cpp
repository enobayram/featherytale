/*
 * Simulator.cpp
 *
 *  Created on: Nov 2, 2011
 *      Author: eba
 */

#include <cmath>
#include <iostream>
#include <tuple>

#include <boost/bind.hpp>

#include <Eigen/Dense>

#include <orx.h>

#include "Simulator.h"
#include "constants.h"
#include "util/geometry.hpp"
#include "auxiliary/tuple_operators.h"
#include "physicalElements/VicinityIterator.h"

const double TIMESTEP = 0.001;
const double ATOMICMASS = 1;
const double ATOMICMOMENTOFINERTIA = 0.5*ATOMICMASS*(BONDLENGTH/2)*(BONDLENGTH/2); //The MOI of a thin disc
const double ENDREPULSIONDISTANCE = 1;

double USRFORCECONST;
double USRFORCEDAMPING;
double MAXUSERFORCE;
double FORCECONST;
double ROTCONST;
double LINEARDAMPING;
double ROTATIONALDAMPING;
double MAXBONDSTRETCH;
double MAXBONDTWIST; //Radian
double ENDREPULSIONFORCECONST;
double GRAVITYMULTIPLIER;
Vector2d gravity;
double ROTATIONALFRICTIONCONST;
double ROTATIONALFRICTIONSPEEDCAP;
double ENERGYEVENTTHRESHOLD;

void updateParameters() {
	orxConfig_PushSection("Simulation"); {
		USRFORCECONST = orxConfig_GetFloat("USRFORCECONST");
		USRFORCEDAMPING = orxConfig_GetFloat("USRFORCEDAMPING");
		MAXUSERFORCE = orxConfig_GetFloat("MAXUSERFORCE");
		FORCECONST = orxConfig_GetFloat("FORCECONST");
		ROTCONST = orxConfig_GetFloat("ROTCONST");
		LINEARDAMPING = orxConfig_GetFloat("LINEARDAMPING");
		ROTATIONALDAMPING = orxConfig_GetFloat("ROTATIONALDAMPING");
		MAXBONDSTRETCH = BONDLENGTH * orxConfig_GetFloat("MAXBONDSTRETCH");
		MAXBONDTWIST = orxConfig_GetFloat("MAXBONDTWIST"); //Radian
		ENDREPULSIONFORCECONST = orxConfig_GetFloat("ENDREPULSIONFORCECONST");
		GRAVITYMULTIPLIER = orxConfig_GetFloat("GRAVITYMULTIPLIER");
		gravity = Vector2d(0,GRAVITYMULTIPLIER);
		ROTATIONALFRICTIONCONST = orxConfig_GetFloat("ROTATIONALFRICTIONCONST");
		ROTATIONALFRICTIONSPEEDCAP = orxConfig_GetFloat("ROTATIONALFRICTIONSPEEDCAP");
		ENERGYEVENTTHRESHOLD = orxConfig_GetFloat("ENERGYEVENTTHRESHOLD");
	} orxConfig_PopSection();
}

using std::tuple;
using std::make_tuple;
using std::tie;

template <class T>
T clamp(T value, T clampval) {
	return value < -clampval ? -clampval : (value > clampval ? clampval : value);
}

Simulator::Simulator(World & world): running(false), world(world), simulationTarget(0), stopped(false), shouldPause(false){
}
void Simulator::init() {
    orxConfig_PushSection("World");
    orxFLOAT worldWidth = orxConfig_GetFloat("Width");
    orxFLOAT worldHeight = orxConfig_GetFloat("Height");

    worldTop = -worldHeight/2;
    worldBottom = worldHeight/2;
    worldRight = worldWidth/2;
    worldLeft = -worldWidth/2;

    assert(worldTop>MINY);
    assert(worldBottom<MAXY);
    assert(worldLeft>MINX);
    assert(worldRight<MAXX);

    orxConfig_PopSection();
    updateParameters();
};

Simulator::~Simulator() {
}

double endRepulsion(double dist) {
	return ENDREPULSIONFORCECONST*(1/(ENDREPULSIONDISTANCE*ENDREPULSIONDISTANCE)-1/(dist*dist));
}

double tangentialSpeedDifference(const Vector2d & lin_vel, double ang_vel, double dist_to_tangent, const Vector2d & tangent) {
	double tangential_speed = lin_vel.transpose() * tangent;
	return tangential_speed + dist_to_tangent*ang_vel;
}

inline void energyChange(Simulator & s, const Vector2d & pos, double amount, bool dissipated, bool radial) {
    double abs_amount = fabs(amount);
    if(abs_amount>ENERGYEVENTTHRESHOLD) s.fire_event<energy_change_event>(pos, abs_amount, dissipated, radial);
}

inline tuple<Vector2d,double> wallMechanics(Simulator & s, const Vector2d & pos, const Vector2d & vel, double spin, double dist_to_wall, const Vector2d & wallNormal) {
    if(dist_to_wall > ENDREPULSIONDISTANCE) return tuple<Vector2d,double>(Vector2d::Zero(), 0);

	Vector2d wallTangent = Vector2d(wallNormal.y(),-wallNormal.x());
	Vector2d perpVel = wallNormal*(vel.transpose()*wallNormal);
	Vector2d tangentialVel = wallTangent*(vel.transpose()*wallTangent);

	Vector2d radialSpringForce = -endRepulsion(dist_to_wall)*wallNormal;
	Vector2d radialDampingForce = -LINEARDAMPING*perpVel;
	Vector2d tangentialDampingForce = -LINEARDAMPING*tangentialVel/10;
	Vector2d force = radialSpringForce+radialDampingForce+tangentialDampingForce;
	double rotary_friction_speed = tangentialSpeedDifference(vel, spin,BONDLENGTH/2,wallTangent);
	double rotary_friction_force = force.norm()*clamp(rotary_friction_speed,ROTATIONALFRICTIONSPEEDCAP)*ROTATIONALFRICTIONCONST;

	// Energy events
	energyChange(s, pos, radialSpringForce.dot(vel),false, true);
	energyChange(s, pos, radialDampingForce.dot(vel), true, true);
	energyChange(s, pos, tangentialDampingForce.dot(vel), true, false);

	return tuple<Vector2d,double>(force-rotary_friction_force*wallTangent,-rotary_friction_force*BONDLENGTH/2);
}

//TODO add adaptive time steps for strong bonds
//TODO add adaptive time steps for fast collisions (maybe combine these two using force curvature)
/*TODO parallelize simulation. The collision detection can easily be parallelized as of now, since the
 * vicinity iterator iterates in a single direction to avoid deadlocks. Simply lock the two cells you're
 * working on and do the force calculations. For this you need to iterate over non-empty cells instead
 * of bonds. So, maintain a list of non-empty cells. You can do the same for bonds if you process them in the same
 * way. But beware, there's no guarantee that a bonded atom will be within vicinity, since bonds can stretch.
 * One solution can be to process those bonds you encounter during cell iteration as usual. Then process the remaining
 * bonds whose other node is residing in one of the "safe to lock cells" (as in the cell iterator)
 */

void Simulator::iterate() {
	for(Node & node: world.getNodes()) {
		node.force = Vector2d::Zero(); node.torque = 0;
	}

	for(const ExternalForce & force: world.getForces()) {
		for(Node * forceNode: force.getNodes()) {
			double forceModifier = sqrt(1.0/force.getNodes().size());
			Vector2d forceVec = (force.getForce()-forceNode->pos)*USRFORCECONST*forceModifier;
			double forceMagnitude = forceVec.norm();
			forceVec *= min(MAXUSERFORCE*forceModifier/forceMagnitude,1.0);
			forceNode->force += (forceVec-forceNode->vel*USRFORCEDAMPING); // The velocity term is for friction
		}
	}

	for(Bond & bond: world.getBonds()) {
		Node & node1 = bond.node1;
		Node & node2 = bond.node2;

		Vector2d delta = node2.pos-node1.pos;
		Vector2d delta_dt = node2.vel-node1.vel;

		double distSq = delta.squaredNorm();
		double dist = sqrt(distSq);
		Vector2d unitVec = delta/dist;
		Vector2d perpVec(-unitVec.y(), unitVec.x());

		double thetaBond = atan2(delta.y(),delta.x());
		double thetaBond_dt = atan2_dt(delta.y(), delta_dt.y(), delta.x(), delta_dt.x());

		Vector2d radialSpringForce = FORCECONST*(1/BONDLENGTH*BONDLENGTH-1/distSq) * unitVec;
		Vector2d radialDampingForce = LINEARDAMPING * (double) (unitVec.transpose()*delta_dt) * unitVec;

		double combinedBondAngle1 = bond.node1Angle + node1.angle;
		double combinedBondAngle2 = bond.node2Angle + node2.angle;

		double angularShift1 = normalizeAngle(thetaBond-combinedBondAngle1);
		double angularShift2 = normalizeAngle(thetaBond-combinedBondAngle2);
		double angularShift = angularShift1 + angularShift2;

		double rotSpringForce1 = ROTCONST*angularShift1; // rotational spring force on node 1
		double rotSpringForce2 = ROTCONST*angularShift2;
        double rotSpringForce = rotSpringForce1 + rotSpringForce2;

        double rotSpringTorque1 = rotSpringForce1*dist;
        double rotSpringTorque2 = rotSpringForce2*dist;

		double rotDamp1 = -ROTATIONALDAMPING*(node1.spin-thetaBond_dt);//Rotational damping on node 1
		double rotDamp2 = -ROTATIONALDAMPING*(node2.spin-thetaBond_dt);
		double rotDampTorque = rotDamp1 + rotDamp2;
		double rotDampForce = rotDampTorque/dist;

		Vector2d force = radialSpringForce + radialDampingForce + (rotSpringForce+rotDampForce)*perpVec;

		node1.force += force;
		node2.force -= force;
		node1.torque += rotSpringTorque1+rotDamp1;
		node2.torque += rotSpringTorque2+rotDamp2;

		if( dist>MAXBONDSTRETCH
		 || angularShift1*angularShift1>MAXBONDTWIST*MAXBONDTWIST
		 || angularShift2*angularShift2>MAXBONDTWIST*MAXBONDTWIST) {
		    fire_event<bond_tear_event>(&bond);
            bond.deleteLater();
		}

		// Calculate energies for the energy events
		Vector2d bondCenter = (node1.pos + node2.pos) / 2;
		energyChange(*this, bondCenter, radialSpringForce.dot(delta_dt),false, true);
        energyChange(*this, bondCenter, radialDampingForce.dot(delta_dt),true, true);

        energyChange(*this, bondCenter, rotSpringTorque1*node1.spin + rotSpringTorque2*node2.spin, false, false);
        energyChange(*this, bondCenter, rotDamp1*node1.spin + rotDamp2*node2.spin, true, false);

	}

	// TODO use the bonded nodes hash set in nodes to prune force calculations
	for(Node & node: world.getNodes()) {
		VicinityIterator vit = world.grid.getVicinityIterator(node,BONDLENGTH);
		for(Node * otherNode_; vit(otherNode_);) {
			Node & otherNode = *otherNode_;
//				if(node.isBondedTo(otherNode)) continue;

			Vector2d delta = otherNode.pos-node.pos;
			double distSq = delta(0)*delta(0)+delta(1)*delta(1), dist = sqrt(distSq);
			Vector2d unitVec = delta/dist;
			Vector2d delta_dt = otherNode.vel-node.vel;
			Vector2d perpVec(-unitVec.y(), unitVec.x());

			double rotary_friction_speed = tangentialSpeedDifference(node.vel-otherNode.vel, node.spin+otherNode.spin,dist/2,perpVec);

			double radialSpringMag = FORCECONST*(1/BONDLENGTH*BONDLENGTH-1/distSq); // radial spring force magnitude
			Vector2d radialSpringForce =  radialSpringMag * unitVec;
			Vector2d radialDampingForce = LINEARDAMPING * (double) (unitVec.transpose()*delta_dt) * unitVec;

			double rotary_friction_force = fabs(radialSpringMag)*clamp(rotary_friction_speed,ROTATIONALFRICTIONSPEEDCAP)*ROTATIONALFRICTIONCONST;
			double rotary_friction_torque = -rotary_friction_force*dist/2;

			node.force += radialSpringForce + radialDampingForce -rotary_friction_force*perpVec;// + r*perpVec;
			node.torque += rotary_friction_torque;
			otherNode.force+= -(radialSpringForce+radialDampingForce)+rotary_friction_force*perpVec;
			otherNode.torque += rotary_friction_torque;

	        // Calculate energies for the energy events
			Vector2d actionPos = (node.pos + otherNode.pos)/2;
	        energyChange(*this, actionPos, radialSpringForce.dot(delta_dt),false, true);
	        energyChange(*this, actionPos, radialDampingForce.dot(delta_dt),true, true);

	        energyChange(*this, actionPos, rotary_friction_torque * (node.spin + otherNode.spin), true, false);
		}
	}

	for(Node & node: world.getNodes()) {
	    tie(node.force,node.torque) += wallMechanics(*this, node.pos, node.vel, node.spin, node.pos.x()-worldLeft  , Vector2d(1,0));
		tie(node.force,node.torque) += wallMechanics(*this, node.pos, node.vel, node.spin, worldRight-node.pos.x() , Vector2d(-1,0));
		tie(node.force,node.torque) += wallMechanics(*this, node.pos, node.vel, node.spin, node.pos.y()-worldTop   , Vector2d(0,1));
		tie(node.force,node.torque) += wallMechanics(*this, node.pos, node.vel, node.spin, worldBottom-node.pos.y(), Vector2d(0,-1));
	}

	for(Node & node: world.getNodes()) {
		node.force += gravity*ATOMICMASS;
	}

	for(Node & node: world.getNodes()) {
		node.vel+=node.force/ATOMICMASS*TIMESTEP;
		node.spin += node.torque/ATOMICMOMENTOFINERTIA*TIMESTEP;
		//node.pos += node.vel*TIMESTEP;
		world.displace(node,node.vel*TIMESTEP);
		node.angle = normalizeAngle(node.angle+node.spin*TIMESTEP);
	}

	world.performDeletions();
	world.incrementTime(World::seconds(TIMESTEP));

}

double get_gravity() {
    return GRAVITYMULTIPLIER;
}
