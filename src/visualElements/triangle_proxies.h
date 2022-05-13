/*
 * triangle_proxies.h
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef TRIANGLE_PROXIES_H_
#define TRIANGLE_PROXIES_H_

#include "visualElements/pinTypes.h"

class TriangleProxy { // This class is used to perform special deletion actions for triangles
	template<typename T>
	void callError() { std::cerr<<"ERROR, TRIANGLEPROXY RECEIVED AN INFORMATION CALL WITH A "<<typeid(T).name()<<" TYPE";}
	friend class TriangleBase;
protected:
	TriangleBase * this_triangle;
public:
	virtual ~TriangleProxy(){}

	virtual void informationAvailable(BondPin *){
		callError<BondPin *>();
	}
	virtual void invalidating()=0;
};

class NullProxy: public TriangleProxy {
protected:
	virtual void invalidating() {}
};

class BGTriangleProxy: public TriangleProxy {
	virtual void informationAvailable(BondPin * remove) {}//this_triangle->invalidate();}
	void invalidating() {}
};

class N3TriangleProxy: public TriangleProxy { // This informs the bonds, while the triangle is being destroyed
	BondPin * bonds[3];
public:
	N3TriangleProxy(BondPin * bond1,BondPin * bond2,BondPin * bond3): bonds{bond1,bond2,bond3} {}
protected:
	virtual void invalidating() {
		for(BondPin * bond: bonds) {
			if(bond) bond->removeTriangle(this_triangle);
		}
	}
};


#endif /* TRIANGLE_PROXIES_H_ */
