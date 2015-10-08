#ifndef SHAREDMEMPTR_H
#define SHAREDMEMPTR_H

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h> //shm_open
#include <stdio.h>  //printf
#include <stdlib.h> //exit
#include <unistd.h> //close
#include <string.h> //strerror

#include <sys/types.h>
   #include <sys/ipc.h>
   #include <sys/shm.h>
   #include <errno.h>
   #include <stdio.h>

#define DEF_SIZE 1024


class ShMemPool
{
protected:
    int shmFD;
    
    struct info
    {
        size_t clientsCount;
        size_t usedSize;
        size_t size;
    } *pollInf;
    char *begMBloks;
    
    void * MapData(const int &shm_fd, const size_t &size, const size_t &offset)
    {
        
    }
    
    bool AttachTo(const int &shm_fd)
    {
        pollInf = (info *) shmat(shm_fd, NULL, SHM_RND);
        if (NULL == pollInf)
        {
            begMBloks = NULL;
            printf("Failed to attach shared memory pool \n");
            return false;
        }
        begMBloks = ((char *) pollInf) + sizeof (info);
        shmFD = shm_fd;
        return true;
    }
public:
    ShMemPool(key_t &key): shmFD(-1), pollInf(NULL)
    {
        // http://www.ibm.com/developerworks/linux/library/l-shared.html
        int shm_fd = -1;
        bool first = false;
        if((shm_fd = shmget(key, DEF_SIZE, 0666)) >= 0 ) 
        {
            first = true; /* We are the first instance */
            /* Set the size of the SHM to be the size of the struct. */
            ftruncate(shm_fd, sizeof(info) + DEF_SIZE);
        }
        else
        {
            if (ENOENT != errno) 
            {
                printf("Could not create shm object. %s\n", strerror(errno));
                return;
            }
            if((shm_fd = shmget(key, DEF_SIZE, IPC_CREAT | 0666)) < 0)
            {
                /* Try to open the shm instance normally and share it with
                * existing clients
                */
                printf("Could not create shm object. %s\n", strerror(errno));
                return;
            }
            
        }

        if ( AttachTo(shm_fd) )
        {
            printf("Failed to create shared pool \n");
            return;
        }
        if ( first )
        {
            pollInf->clientsCount = 1;
            pollInf->usedSize = 0;
            pollInf->size = DEF_SIZE;
        }
    }

    template <class Type>
    class MBlock
    {
    public:
        int instCount;
        size_t offset;
        Type data;
        Type* operator->()
        {
            return &(data);
        }
        
        Type& operator*()
        {
          return data;
        }
    };
    
    template <class Type>
    MBlock<Type> * getNewMem()
    {
        if ( NULL == pollInf || pollInf->usedSize + sizeof(Type) > pollInf->size)
            return NULL;
        MBlock<Type> *p  =  (MBlock<Type> *)(begMBloks + pollInf->usedSize);
        p->instCount = 1;
        p->offset = pollInf->usedSize;
        pollInf->usedSize += sizeof(MBlock<Type>);
        ++ (pollInf->clientsCount);
        return p;
    }
    
    template <class Type>
    MBlock<Type> * getMem(const size_t offset)
    {
        if ( NULL == pollInf || offset + sizeof(Type) > pollInf->size)
            return NULL;
        MBlock<Type> *p  =  (MBlock<Type> *)(begMBloks + pollInf->usedSize);
        ++ p->instCount;
        ++ (pollInf->clientsCount);
        return p;
    }
    
    template <class Type>
    void freeMem(const size_t &offset)
    {
        if ( NULL == pollInf || offset + sizeof(Type) > pollInf->size)
            return;
        MBlock<Type> *p  =  (MBlock<Type> *)(begMBloks + pollInf->usedSize);
        -- p->instCount;
        if (0 >= p->instCount)
        {
            -- (pollInf->clientsCount);
        }
        if (0 >= pollInf->clientsCount)
        {
            shmdt(pollInf);
            pollInf = NULL;
            begMBloks = NULL;
        }
    }
};



template <class Type>
class SharedMemPtr
{
protected:
    ShMemPool &shPool;
    ShMemPool::MBlock<Type> *memPtr;
    
public:
    SharedMemPtr(ShMemPool &shPool_): shPool(shPool_)
    {
        memPtr = shPool.getNewMem<Type>();
    }

    SharedMemPtr(const size_t &offset, ShMemPool &shPool_): shPool(shPool_)
    {
        memPtr = shPool.getMem<Type>(offset);
    }

    ~SharedMemPtr()
    {
        shPool.freeMem<Type>(memPtr->offset);
    }
    
    SharedMemPtr & operator = (SharedMemPtr &ptr)
    {
        if (this == &ptr)
            return *this;
        if (&shPool != &ptr.shPool)
        {
            printf("Faled to copy. Shared mem pools a different\n");
            return *this;
        }
        memPtr = ptr.memPtr;
        ++ memPtr->instCount;
        return *this;
    }
    
    Type* operator->() const
    {
        if (NULL == memPtr)
            return NULL;
        return &(memPtr->data);
    }
    
    Type& operator*() const
    {
      return memPtr->data;
    }
    
    bool notNULL()
    {
        return NULL != memPtr;
    }

};

#endif // SHAREDMEMPTR_H
