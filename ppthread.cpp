
#include "ppthread.h"


int pthread_create(pthread_t* thread, pthread_attr_t* attr, void* (*start_routine)(void*), void* arg)
{
    if (thread == NULL || start_routine == NULL)
        return 1;

    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
    if (*thread == NULL) return 1;
    return 0;
}

int pthread_join(pthread_t thread, void** value_ptr)
{
#pragma warning(suppress: 4312)
    if (value_ptr != nullptr) *value_ptr = (void*)WaitForSingleObject(thread, INFINITE);
    else WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}

int pthread_detach(pthread_t thread)
{
    CloseHandle(thread);
    return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* attr)
{
    (void)attr;

    if (mutex == NULL)
        return 1;

    InitializeCriticalSection(mutex);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    if (mutex == NULL)
        return 1;
    DeleteCriticalSection(mutex);
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    if (mutex == NULL)
        return 1;
    EnterCriticalSection(mutex);
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    if (mutex == NULL)
        return 1;
    LeaveCriticalSection(mutex);
    return 0;
}

int pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr)
{
    (void)attr;
    if (rwlock == NULL)
        return 1;
    InitializeSRWLock(&(rwlock->lock));
    rwlock->exclusive = false;
    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t* rwlock)
{
    (void)rwlock;
}

int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock)
{
    if (rwlock == NULL)
        return 1;
    AcquireSRWLockShared(&(rwlock->lock));
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock)
{
    if (rwlock == NULL)
        return 1;
    return !TryAcquireSRWLockShared(&(rwlock->lock));
}

int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock)
{
    if (rwlock == NULL)
        return 1;
    AcquireSRWLockExclusive(&(rwlock->lock));
    rwlock->exclusive = true;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t* rwlock)
{
    BOOLEAN ret;

    if (rwlock == NULL)
        return 1;

    ret = TryAcquireSRWLockExclusive(&(rwlock->lock));
    if (ret)
        rwlock->exclusive = true;
    return ret;
}

int pthread_rwlock_unlock(pthread_rwlock_t* rwlock)
{
    if (rwlock == NULL)
        return 1;

    if (rwlock->exclusive) {
        rwlock->exclusive = false;
#pragma warning(suppress: 26110)
        ReleaseSRWLockExclusive(&(rwlock->lock));
    }
    else {
        ReleaseSRWLockShared(&(rwlock->lock));
    }
}

