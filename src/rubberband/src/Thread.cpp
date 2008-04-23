/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band
    An audio time-stretching and pitch-shifting library.
    Copyright 2007 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Thread.h"

#include <iostream>
#include <cstdlib>

#include <sys/time.h>
#include <time.h>

//#define DEBUG_THREAD 1
//#define DEBUG_MUTEX 1
//#define DEBUG_CONDITION 1

using std::cerr;
using std::endl;
using std::string;

namespace RubberBand
{

#ifdef _WIN32

Thread::Thread() :
    m_id(0),
    m_extant(false)
{
#ifdef DEBUG_THREAD
    cerr << "THREAD DEBUG: Created thread object " << this << endl;
#endif
}

Thread::~Thread()
{
#ifdef DEBUG_THREAD
    cerr << "THREAD DEBUG: Destroying thread object " << this << ", id " << m_id << endl;
#endif
    if (m_extant) {
        WaitForSingleObject(m_id, INFINITE);
    }
#ifdef DEBUG_THREAD
    cerr << "THREAD DEBUG: Destroyed thread object " << this << endl;
#endif
}

void
Thread::start()
{
    m_id = CreateThread(NULL, 0, staticRun, this, 0, 0);
    if (!m_id) {
        cerr << "ERROR: thread creation failed" << endl;
        exit(1);
    } else {
#ifdef DEBUG_THREAD
        cerr << "THREAD DEBUG: Created thread " << m_id << " for thread object " << this << endl;
#endif
        m_extant = true;
    }
}    

void 
Thread::wait()
{
    if (m_extant) {
#ifdef DEBUG_THREAD
        cerr << "THREAD DEBUG: Waiting on thread " << m_id << " for thread object " << this << endl;
#endif
        WaitForSingleObject(m_id, INFINITE);
#ifdef DEBUG_THREAD
        cerr << "THREAD DEBUG: Waited on thread " << m_id << " for thread object " << this << endl;
#endif
        m_extant = false;
    }
}

Thread::Id
Thread::id()
{
    return m_id;
}

bool
Thread::threadingAvailable()
{
    return true;
}

DWORD
Thread::staticRun(LPVOID arg)
{
    Thread *thread = static_cast<Thread *>(arg);
#ifdef DEBUG_THREAD
    cerr << "THREAD DEBUG: " << (void *)GetCurrentThreadId() << ": Running thread " << thread->m_id << " for thread object " << thread << endl;
#endif
    thread->run();
    return 0;
}

Mutex::Mutex() :
    m_locked(false)
{
    m_mutex = CreateMutex(NULL, FALSE, NULL);
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)GetCurrentThreadId() << ": Initialised mutex " << &m_mutex << endl;
#endif
}

Mutex::~Mutex()
{
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)GetCurrentThreadId() << ": Destroying mutex " << &m_mutex << endl;
#endif
    CloseHandle(m_mutex);
}

void
Mutex::lock()
{
    if (m_locked) {
        cerr << "ERROR: Deadlock on mutex " << &m_mutex << endl;
    }
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)GetCurrentThreadId() << ": Want to lock mutex " << &m_mutex << endl;
#endif
    WaitForSingleObject(m_mutex, INFINITE);
    m_locked = true;
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)GetCurrentThreadId() << ": Locked mutex " << &m_mutex << endl;
#endif
}

void
Mutex::unlock()
{
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)GetCurrentThreadId() << ": Unlocking mutex " << &m_mutex << endl;
#endif
    m_locked = false;
    ReleaseMutex(m_mutex);
}

bool
Mutex::trylock()
{
    DWORD result = WaitForSingleObject(m_mutex, 0);
    if (result == WAIT_TIMEOUT || result == WAIT_FAILED) {
#ifdef DEBUG_MUTEX
        cerr << "MUTEX DEBUG: " << (void *)GetCurrentThreadId() << ": Mutex " << &m_mutex << " unavailable" << endl;
#endif
        return false;
    } else {
        m_locked = true;
#ifdef DEBUG_MUTEX
        cerr << "MUTEX DEBUG: " << (void *)GetCurrentThreadId() << ": Locked mutex " << &m_mutex << " (from trylock)" << endl;
#endif
        return true;
    }
}

Condition::Condition(string name) :
    m_name(name),
    m_locked(false)
{
    m_mutex = CreateMutex(NULL, FALSE, NULL);
    m_condition = CreateEvent(NULL, FALSE, FALSE, NULL);
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Initialised condition " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
}

Condition::~Condition()
{
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Destroying condition " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    if (m_locked) ReleaseMutex(m_mutex);
    CloseHandle(m_condition);
    CloseHandle(m_mutex);
}

void
Condition::lock()
{
    if (m_locked) {
#ifdef DEBUG_CONDITION
        cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Already locked " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
        return;
    }
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Want to lock " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    WaitForSingleObject(m_mutex, INFINITE);
    m_locked = true;
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Locked " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
}

void
Condition::unlock()
{
    if (!m_locked) {
#ifdef DEBUG_CONDITION
        cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Not locked " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
        return;
    }
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Unlocking " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    m_locked = false;
    ReleaseMutex(m_mutex);
}

void 
Condition::wait(int us)
{
    if (!m_locked) lock();

    if (us == 0) {

#ifdef DEBUG_CONDITION
        cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Waiting on " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
        SignalObjectAndWait(m_mutex, m_condition, INFINITE, FALSE);
        WaitForSingleObject(m_mutex, INFINITE);

    } else {

        DWORD ms = us / 1000;
        if (us > 0 && ms == 0) ms = 1;
    
#ifdef DEBUG_CONDITION
        cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Timed waiting on " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
        SignalObjectAndWait(m_mutex, m_condition, ms, FALSE);
        WaitForSingleObject(m_mutex, INFINITE);
    }

    ReleaseMutex(m_mutex);

#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Wait done on " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    m_locked = false;
}

void
Condition::signal()
{
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)GetCurrentThreadId() << ": Signalling " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    SetEvent(m_condition);
}

#else /* !_WIN32 */


Thread::Thread() :
    m_id(0),
    m_extant(false)
{
#ifdef DEBUG_THREAD
    cerr << "THREAD DEBUG: Created thread object " << this << endl;
#endif
}

Thread::~Thread()
{
#ifdef DEBUG_THREAD
    cerr << "THREAD DEBUG: Destroying thread object " << this << ", id " << m_id << endl;
#endif
    if (m_extant) {
        pthread_join(m_id, 0);
    }
#ifdef DEBUG_THREAD
    cerr << "THREAD DEBUG: Destroyed thread object " << this << endl;
#endif
}

void
Thread::start()
{
    if (pthread_create(&m_id, 0, staticRun, this)) {
        cerr << "ERROR: thread creation failed" << endl;
        exit(1);
    } else {
#ifdef DEBUG_THREAD
        cerr << "THREAD DEBUG: Created thread " << m_id << " for thread object " << this << endl;
#endif
        m_extant = true;
    }
}    

void 
Thread::wait()
{
    if (m_extant) {
#ifdef DEBUG_THREAD
        cerr << "THREAD DEBUG: Waiting on thread " << m_id << " for thread object " << this << endl;
#endif
        pthread_join(m_id, 0);
#ifdef DEBUG_THREAD
        cerr << "THREAD DEBUG: Waited on thread " << m_id << " for thread object " << this << endl;
#endif
        m_extant = false;
    }
}

Thread::Id
Thread::id()
{
    return m_id;
}

bool
Thread::threadingAvailable()
{
    return true;
}

void *
Thread::staticRun(void *arg)
{
    Thread *thread = static_cast<Thread *>(arg);
#ifdef DEBUG_THREAD
    cerr << "THREAD DEBUG: " << (void *)pthread_self() << ": Running thread " << thread->m_id << " for thread object " << thread << endl;
#endif
    thread->run();
    return 0;
}

Mutex::Mutex() :
    m_locked(false)
{
    pthread_mutex_init(&m_mutex, 0);
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)pthread_self() << ": Initialised mutex " << &m_mutex << endl;
#endif
}

Mutex::~Mutex()
{
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)pthread_self() << ": Destroying mutex " << &m_mutex << endl;
#endif
    pthread_mutex_destroy(&m_mutex);
}

void
Mutex::lock()
{
    if (m_locked) {
        cerr << "ERROR: Deadlock on mutex " << &m_mutex << endl;
    }
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)pthread_self() << ": Want to lock mutex " << &m_mutex << endl;
#endif
    pthread_mutex_lock(&m_mutex);
    m_locked = true;
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)pthread_self() << ": Locked mutex " << &m_mutex << endl;
#endif
}

void
Mutex::unlock()
{
#ifdef DEBUG_MUTEX
    cerr << "MUTEX DEBUG: " << (void *)pthread_self() << ": Unlocking mutex " << &m_mutex << endl;
#endif
    m_locked = false;
    pthread_mutex_unlock(&m_mutex);
}

bool
Mutex::trylock()
{
    if (pthread_mutex_trylock(&m_mutex)) {
#ifdef DEBUG_MUTEX
        cerr << "MUTEX DEBUG: " << (void *)pthread_self() << ": Mutex " << &m_mutex << " unavailable" << endl;
#endif
        return false;
    } else {
        m_locked = true;
#ifdef DEBUG_MUTEX
        cerr << "MUTEX DEBUG: " << (void *)pthread_self() << ": Locked mutex " << &m_mutex << " (from trylock)" << endl;
#endif
        return true;
    }
}

Condition::Condition(string name) :
    m_locked(false),
    m_name(name)
{
    pthread_mutex_init(&m_mutex, 0);
    pthread_cond_init(&m_condition, 0);
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Initialised condition " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
}

Condition::~Condition()
{
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Destroying condition " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    if (m_locked) pthread_mutex_unlock(&m_mutex);
    pthread_cond_destroy(&m_condition);
    pthread_mutex_destroy(&m_mutex);
}

void
Condition::lock()
{
    if (m_locked) {
#ifdef DEBUG_CONDITION
        cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Already locked " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
        return;
    }
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Want to lock " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    pthread_mutex_lock(&m_mutex);
    m_locked = true;
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Locked " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
}

void
Condition::unlock()
{
    if (!m_locked) {
#ifdef DEBUG_CONDITION
        cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Not locked " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
        return;
    }
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Unlocking " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    m_locked = false;
    pthread_mutex_unlock(&m_mutex);
}

void 
Condition::wait(int us)
{
    if (!m_locked) lock();

    if (us == 0) {

#ifdef DEBUG_CONDITION
        cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Waiting on " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
        pthread_cond_wait(&m_condition, &m_mutex);

    } else {

        struct timeval now;
        gettimeofday(&now, 0);

        now.tv_usec += us;
        while (now.tv_usec > 1000000) {
            now.tv_usec -= 1000000;
            ++now.tv_sec;
        }

        struct timespec timeout;
        timeout.tv_sec = now.tv_sec;
        timeout.tv_nsec = now.tv_usec * 1000;
    
#ifdef DEBUG_CONDITION
        cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Timed waiting on " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
        pthread_cond_timedwait(&m_condition, &m_mutex, &timeout);
    }

    pthread_mutex_unlock(&m_mutex);

#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Wait done on " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    m_locked = false;
}

void
Condition::signal()
{
#ifdef DEBUG_CONDITION
    cerr << "CONDITION DEBUG: " << (void *)pthread_self() << ": Signalling " << &m_condition << " \"" << m_name << "\"" << endl;
#endif
    pthread_cond_signal(&m_condition);
}

#endif /* !_WIN32 */

MutexLocker::MutexLocker(Mutex *mutex) :
    m_mutex(mutex)
{
    if (m_mutex) {
        m_mutex->lock();
    }
}

MutexLocker::~MutexLocker()
{
    if (m_mutex) {
        m_mutex->unlock();
    }
}

}

