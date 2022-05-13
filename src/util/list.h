/*
 * orxdata.h
 *
 *  Created on: Nov 20, 2015
 *      Author: enis
 */

#ifndef ORXPP_LIST_H_
#define ORXPP_LIST_H_

#include <utils/orxLinkList.h>

namespace orx {

namespace internal {

template <class T>
struct LinkListNode: public orxLINKLIST_NODE {
    T value;
    LinkListNode(T value) {
    	orxMemory_Zero(this, sizeof(LinkListNode));
    	this->value = value;
    }
};

}

template <class T>
class LinkList {
    orxLINKLIST list;
    typedef internal::LinkListNode<T> Node;
    void delete_node(orxLINKLIST_NODE * node) {
        orxLinkList_Remove(node);
        delete static_cast<Node *>(node);
    }
public:
    LinkList() {
    	orxMemory_Zero(&list, sizeof(orxLINKLIST));
    }
    T& front() {
        return static_cast<Node *>(orxLinkList_GetFirst(&list))->value;
    }
    T& back() {
        return static_cast<Node *>(orxLinkList_GetLast(&list))->value;
    }
    void push_front(const T & value) {
        orxLinkList_AddStart(&list, new Node(value));
    }
    void pop_front() {
        delete_node(orxLinkList_GetFirst(&list));
    }
    void push_back(const T & value) {
        orxLinkList_AddEnd(&list, new Node(value));
    }
    void pop_back() {
        delete_node(orxLinkList_GetLast(&list));
    }

    struct iterator {
        Node * node;
        iterator(Node * node): node(node) {}
        T& operator*() {
            return node->value;
        }
        iterator & operator++() {
            node = static_cast<Node *>(orxLinkList_GetNext(node));
            return *this;
        }
        iterator & operator--() {
            node = orxLinkList_GetPrevious(node);
            return *this;
        }
        bool operator!=(const iterator & other) {
            return node != other.node;
        }
    };

    iterator begin() {
        return iterator(static_cast<Node *>(orxLinkList_GetFirst(&list)));
    }
    iterator end() {return iterator(orxNULL); }
    iterator insert(iterator position, const T & val) {
        Node * new_node = new Node(val);
        orxLinkList_AddBefore(position.node, new_node);
        return new_node;
    }

    size_t size() {
        return orxLinkList_GetCounter(&list);
    }

    template <int BUFFER_SIZE>
    size_t copy_to_buffer(T (&buf)[BUFFER_SIZE]) {
        return copy_to_buffer(buf, BUFFER_SIZE);
    }

    size_t copy_to_buffer(T * buffer, size_t buffer_size) {
        size_t copied = 0;
        for(iterator it = begin(); it!=end() && copied<buffer_size; ++it) {
            buffer[copied] = *it;
            copied++;
        }
        return copied;
    }
    ~LinkList() {
        for(orxLINKLIST_NODE * node = orxLinkList_GetFirst(&list);
            node;
            node = orxLinkList_GetFirst(&list)) {
            delete_node(node);
        }
    }
};

}

#endif /* UTIL_LIST_H_ */
