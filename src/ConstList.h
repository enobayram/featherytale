/*
 * ConstList.h
 *
 *  Created on: Dec 18, 2011
 *      Author: eba
 */

#ifndef CONSTLIST_H_
#define CONSTLIST_H_

#include <boost/intrusive/list.hpp>

template < typename T>
struct type_wrapper{  typedef T type;};

template<class listType, class owner, class item>
class ConstList: private listType {
	friend typename type_wrapper<owner>::type;
public:
	class iterator {
		typename listType::iterator it;
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef item value_type;
		typedef int difference_type;
		typedef item* pointer;
		typedef item& reference;
		template<class T>
		iterator(const T it): it(it){}
		bool operator==(iterator & otherIt) {return it==otherIt.it;}
		bool operator!=(iterator & otherIt) {return it!=otherIt.it;}
		iterator & operator++() {
			it++;
			return *this;
		}
		item & operator*() {
			return *it;
		}
	};
	class const_iterator {
		typename listType::iterator it;
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef const item value_type;
		typedef int difference_type;
		typedef const item* pointer;
		typedef const item& reference;
		template<class T>
		const_iterator(const T it): it(it){}
		bool operator==(iterator & otherIt) {return it==otherIt.it;}
		iterator & operator++() {
			it++;
			return *this;
		}
		const item & operator*() {
			return *it;
		}
	};

	iterator begin() {
		return iterator(listType::begin());
	}

	iterator end() {
		return iterator(listType::end());
	}

	using listType::size;
    using listType::empty;
};

template<class item, class owner, class hooktype>
class HookedItem: public item, public hooktype {
	friend typename type_wrapper<owner>::type;
public:
private:
	template<class Arg1, class Arg2>
	HookedItem(Arg1 &arg1, Arg2 &arg2): item(arg1, arg2){}
};

#endif /* CONSTLIST_H_ */
