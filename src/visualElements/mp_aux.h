/*
 * mp_aux.h
 *
 *  Created on: Dec 22, 2011
 *      Author: eba
 */

#ifndef MP_AUX_H_
#define MP_AUX_H_

#include <boost/type_traits.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/transform.hpp>

namespace mpl = boost::mpl;

#define CREATE_PROXY(function_name_, proxy_name_) \
		template<bool T__1 = true>\
		struct proxy_name_ { \
			template<class T__2>\
			void operator()(T__2 & obj) { function_name_(obj);}\
		};\
		template<> \
		struct proxy_name_ <false> {\
			template<class T__2>\
			void operator()(T__2 & obj) {}\
		};

#define CONDITIONAL_CALLER(function_name_, proxy_name_) \
		template<bool T__1 = true>\
		struct proxy_name_ { \
			template<class T__2>\
			void call(T__2 arg1) { function_name_(arg1);}\
			template<class T__2, class T__3>\
			void call(T__2 arg1, T__3 arg2) { function_name_(arg1, arg2);}\
			template<class T__2, class T__3, class T__4>\
			void call(T__2 arg1, T__3 arg2, T__4 arg3) { function_name_(arg1, arg2, arg3);}\
		};\
		template<> \
		struct proxy_name_ <false> {\
			template<class T__2>\
			void call(T__2 arg1) {}\
			template<class T__2, class T__3>\
			void call(T__2 arg1, T__3 arg2) {}\
			template<class T__2, class T__3, class T__4>\
			void call(T__2 arg1, T__3 arg2, T__4 arg3) {}\
		};\



template<class node1type, class node2type, class node3type>
struct Triangle;

template<class possiblyPointerType>
struct ptr_to_ref {
	typedef typename boost::remove_pointer<possiblyPointerType>::type no_ptr;
	typedef typename boost::add_reference<no_ptr>::type with_ref;
	typedef typename mpl::if_<
	          boost::is_pointer<possiblyPointerType>
	        , with_ref
	        , possiblyPointerType // #1
	        >::type type;
};

template<class node1type, class node2type, class node3type>
struct ProperTriangle {
	typedef Triangle<typename ptr_to_ref<node1type>::type,
		     	 	 typename ptr_to_ref<node2type>::type,
		     	 	 typename ptr_to_ref<node3type>::type> type;
};

template <typename T>
T& deref(T& x) { return x; }

template <typename T>
T& deref(T* x) { return *x; }


template<class node1type, class node2type, class node3type>
typename ProperTriangle<node1type, node2type, node3type>::type
make_triangle(node1type node1, node2type node2, node3type node3) {
	typename ProperTriangle<node1type, node2type, node3type>::type result(deref(node1), deref(node2), deref(node3));
	return result;
}

template <class element, class hooktype>
struct to_list {
	typedef boost::intrusive::list<
			element,
			boost::intrusive::constant_time_size<false>,
			boost::intrusive::base_hook<hooktype> > type;
};

template <class element>
struct to_vector {
	typedef std::vector<element> type;
};

template<class elementVector, class hooktype>
struct to_containers {
	typedef typename mpl::transform<elementVector, to_list<mpl::_1, hooktype> >::type type;
//	typedef typename mpl::transform<elementVector, to_vector<mpl::_1>>::type type;
};

#endif /* MP_AUX_H_ */
