/*
 * weak_ptr.hpp
 *
 *  Created on: May 23, 2015
 *      Author: eba
 */

#ifndef UTIL_MEMORY_HPP_
#define UTIL_MEMORY_HPP_

#include <boost/intrusive/list.hpp>
#include <memory>

template<class T> class weak_pointable;

template <class T>
struct hooks {
    typedef boost::intrusive::list_base_hook<
        boost::intrusive::link_mode<boost::intrusive::auto_unlink>, boost::intrusive::tag<T> > auto_unlink_hook;
};

template <class T>
struct member_hooks {
    typedef boost::intrusive::list_member_hook<
        boost::intrusive::link_mode<boost::intrusive::auto_unlink>, boost::intrusive::tag<T> > auto_unlink_hook;
    typedef boost::intrusive::list_member_hook<
        boost::intrusive::link_mode<boost::intrusive::normal_link>, boost::intrusive::tag<T> > normal_link_hook;
};

struct weak_pointer_tag;

template <class T>
class weak_pointer: public hooks<weak_pointer_tag>::auto_unlink_hook {
    T* pointee;
public:
    T& operator*() {
        return *pointee;
    }
    T* operator->() {
        return pointee;
    }
    bool isValid() {
        return hooks<weak_pointer_tag>::auto_unlink_hook::is_linked();
    }
    weak_pointer(weak_pointable<T> * pointee);
};

template<class T>
class weak_pointable {
    friend class weak_pointer<T>;
    boost::intrusive::list<
        weak_pointer<T>,
        boost::intrusive::constant_time_size<false>,
        boost::intrusive::base_hook<hooks<weak_pointer_tag>::auto_unlink_hook> > weakPointers;
//  ~weak_pointable() {
//      weakPointers.clear();
//  }
};

template<class T>
struct NullDeleter {
    void operator() (T*& ptr){}
};

template<class T>
bool shouldDelete(const T & obj) {return obj.upForDeletion;}

template<class T>
weak_pointer<T>::weak_pointer(weak_pointable<T> * pointee):pointee((T*) pointee) {
    pointee->weakPointers.push_back(*this);
}

class SharedCarrier {
    const std::shared_ptr<SharedCarrier> shared;
public:
    template <typename T = SharedCarrier> using weak_ptr = std::weak_ptr<T>;
    weak_ptr<> getWeakPtr() {
        return shared;
    }
    template <class T>
    weak_ptr<T> getWeakPtr() {
        return std::static_pointer_cast<T>(shared);
    }
    SharedCarrier():shared(this,NullDeleter<SharedCarrier>()) {}
    ~SharedCarrier(){};
};

#endif /* UTIL_MEMORY_HPP_ */
