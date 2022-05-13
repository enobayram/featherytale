/*
 * shared_object_ptr.hpp
 *
 *  Created on: Jun 13, 2015
 *      Author: eba
 */

#ifndef SHARED_OBJECT_PTR_HPP_
#define SHARED_OBJECT_PTR_HPP_

#include <orx.h>

class shared_object_ptr {
    orxOBJECT * ptr = nullptr;
public:
    shared_object_ptr() {}
    shared_object_ptr(orxOBJECT * object) {
        orxStructure_IncreaseCounter(object);
        ptr = object;
    }
    ~shared_object_ptr() {
        if(ptr) orxStructure_DecreaseCounter(ptr);
    }
    operator orxOBJECT *() {
        return ptr;
    }
};

class weak_object_ptr {
    orxU64 id = 0;
public:
    weak_object_ptr(){}
    weak_object_ptr(orxOBJECT * object): id(object ? orxStructure_GetGUID(object) : 0){}
    operator orxOBJECT *() const {
        auto res = orxStructure_Get(id);
        return res ? orxOBJECT(res):nullptr;
    }
    bool compare_with_id(orxU64 other) {return id==other;}
    explicit operator bool() const {return orxStructure_Get(id);}
};

inline orxSOUND * getSound(weak_object_ptr ptr) {
    orxOBJECT * obj = ptr;
    return obj ? orxObject_GetLastAddedSound(obj) : nullptr;
}


class viewport_guard {
    orxVIEWPORT * ptr;
public:
    viewport_guard(orxVIEWPORT * ptr): ptr(ptr) {}
    ~viewport_guard() {
        orxViewport_Delete(ptr);
    }
    operator orxVIEWPORT * () {return ptr;}
};

#endif /* SHARED_OBJECT_PTR_HPP_ */
