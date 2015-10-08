#ifndef LOCKFREEQUEUE_H
#define LOCKFREEQUEUE_H

#include <pthread.h>
#include "shared_mem_ptr.h"

template <typename T>
class LockFreeQueue 
{
protected:
//    class item_ptr;
    
    template <typename ValT>
    class Node_
    {
        typedef ShMemPool::MBlock<Node_<T> >* NodePtr;
    public:
        ValT value;
        NodePtr next;
        Node_(): next(NULL) {};
        Node_(const ValT &val): next(NULL) {value = val;};
        Node_(const Node_& n): next(n.next), value(n.value) {};
        Node_(const NodePtr& n): next((*n)->next), value((*n)->value) {};
    };
    
    typedef Node_<T> Node;
    typedef ShMemPool::MBlock<Node >* NodePtr;
    typedef ShMemPool::MBlock<NodePtr > NodeSMPtr;
 
 /*
    template <typename ValT>
    class item_ptr
    {
        ShMemPool::MBlock<Node<ValT> > *ptr; 
    public:
        
        item_ptr(): ptr(NULL)
        {
        }
        void set(NodePtr nPtr)
        {
			__sync_lock_test_and_set(&ptr, nPtr);			
        }
        void set(const item_ptr & p)
		{
			if(NULL == &p)
                set (NULL);
            else
                set( p.ptr);
		}
        inline bool operator == (const item_ptr & p)
        {
            return ptr == p.ptr;
        }
        ValT* operator->() const
        {
            if (NULL == ptr)
                return NULL;
            return &(*ptr);
        }
    };
 */   
protected:
    NodeSMPtr headPtr, tailPtr;
    
    bool compare_and_swap(NodePtr &comp_val, NodePtr &dest, const NodePtr newval)
    {
        if ( __sync_bool_compare_and_swap(&dest, comp_val, newval) )
        {
            return true;
        } else 
        {
            __sync_lock_test_and_set(&comp_val, dest);
            return false;
        }
    }

protected:
    ShMemPool &shPool;


public:
	LockFreeQueue(ShMemPool &shPool_):  shPool(shPool_)
	{
	}
	~LockFreeQueue() 
    {
    }
    
    void push(T &val)
    {
        NodePtr newIt  = shPool.getNewMem<Node>();
        (*newIt)->value = val;
        NodePtr tail;

        bool isNULL;
        while (true)
        {
            tail = tailPtr.data;
            if ( NULL == tail || NULL == (*tail)->next)
            {
                
                if (compare_and_swap(tail, tailPtr.data, newIt))
                {
                    __sync_bool_compare_and_swap(&headPtr.data, NULL, newIt);
                    return;
                }
            } 
        }
    }
    
    bool pop (T &val)
    {
       NodePtr next;
       NodePtr head(headPtr);

        
        while (true)
        {
            if ( NULL == head.ptr )
                return false;
                
            next.set( head.ptr->next);
            if (compare_and_swap(head.ptr, headPtr.ptr, next.ptr))
            {
                __sync_bool_compare_and_swap(&tailPtr.ptr, head.ptr, NULL);
                return;
            }
        }
    }

};

#endif // LOCKFREEQUEUE_H
