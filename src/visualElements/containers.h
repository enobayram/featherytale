/*
 * containers.h
 *
 *  Created on: Dec 26, 2011
 *      Author: eba
 */

#ifndef CONTAINERS_H_
#define CONTAINERS_H_

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/slist.hpp>

#include "visualElements/forward_declarations.h"

namespace intrusive = boost::intrusive;
using intrusive::base_hook;
using intrusive::set_base_hook;
using intrusive::optimize_size;
using intrusive::compare;
using intrusive::constant_time_size;
using intrusive::link_mode;
using intrusive::auto_unlink;
using intrusive::slist_base_hook;
using std::greater;

typedef set_base_hook<link_mode<auto_unlink> > PinHook;

typedef intrusive::set< NodePin, constant_time_size<false>,base_hook<PinHook>, compare<greater<NodePin> > > NodePinSet;
typedef intrusive::set< BondPin, constant_time_size<false>,base_hook<PinHook>, compare<greater<BondPin> > > BondPinSet;

typedef NodePinSet::iterator NodeIt;
typedef BondPinSet::iterator BondIt;

typedef slist_base_hook<> InvalidHook;
typedef intrusive::slist<Invalidatable> InvalidList;

template<class T, typename compType>
class Comparable {
	compType value() const {return static_cast<const T*>(this)->comparisonValue();}
	friend bool operator< (const Comparable &a, const Comparable &b)
	{  return a.value() < b.value();  }
	friend bool operator> (const Comparable &a, const Comparable &b)
	{  return a.value() > b.value();  }
	friend bool operator== (const Comparable &a, const Comparable &b)
	{  return a.value() == b.value(); }
};

// These classes are needed to be able to retrieve pins using pointers.
template<class T, class keyType>
struct Comp
{
   bool operator()(keyType key, const T &c) const
   {  return key > c.comparisonValue();  }

   bool operator()(const T &c, keyType key) const
   {  return c.comparisonValue()>key;  }
};


#endif /* CONTAINERS_H_ */
