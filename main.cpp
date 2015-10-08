#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "lock_free_queue.h"

    key_t key = 1234;

void *threadFunction( void *ptr )
{
    ShMemPool shmPool(key);
    SharedMemPtr<LockFreeQueue<int> >Q(shmPool);
    const int &from = *(int *)ptr;
    for (int i=from, upto = from + 100 ; i < upto; ++i)
    {
        Q->push(i);
        sleep(1);
    }
}


int main(int argc, char **argv)
{
    ShMemPool shmPool(key);
    
    SharedMemPtr<LockFreeQueue<int> >Q(shmPool);
    pthread_t thread1, thread2;
    int arg1 = 0, arg2 = 1000;
    int  iret1, iret2;
    iret1 = pthread_create( &thread1, NULL, threadFunction, (void*) &arg1);
    iret2 = pthread_create( &thread2, NULL, threadFunction, (void*) &arg2);
    pthread_join( thread1, NULL);
    pthread_join( thread2, NULL); 

    printf("Thread 1 returns: %d\n",iret1);
    printf("Thread 2 returns: %d\n",iret2);
    return 0;
}
