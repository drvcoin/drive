/*
  Copyright (c) 2018 Drive Foundation

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#pragma once

#include <pthread.h>
#include <errno.h>
#include <stdint.h>

#if defined(__DARWIN__)
#include <sys/time.h>  // gettimeofday
#endif

namespace bdfs 
{
  class AutoMutex
  {
  private:
    pthread_mutex_t * mutex;

  public:
    AutoMutex(pthread_mutex_t * mutex) : mutex(mutex)
    {
      pthread_mutex_lock(mutex);
    }

    ~AutoMutex()
    {
      if (mutex != NULL)
      {
        pthread_mutex_unlock(mutex);
      }
    }

    bool Wait(pthread_cond_t * cond)
    {
      if (mutex == NULL)
      {
        return false;
      }

      return pthread_cond_wait(cond, mutex) == 0;
    }

    bool Wait(pthread_cond_t * cond, uint64_t milliseconds)
    {
      if (mutex == NULL)
      {
        return false;
      }

      // Convert to absolute time
      struct timespec ts;
#if defined(__linux__)
      clock_gettime(CLOCK_REALTIME, &ts);
#else
      timeval tv;
      gettimeofday(&tv, NULL);
      ts.tv_nsec = tv.tv_usec * 1000;
      ts.tv_sec  = tv.tv_sec;
#endif
      uint64_t seconds = (uint64_t)(milliseconds / 1000);
      uint32_t nseconds = ((milliseconds - (seconds * 1000)) * 1000000L);
      nseconds += nseconds + ts.tv_nsec;
      if (nseconds >= 1000000000)
      {
        seconds += 1;
        nseconds -= 1000000000;
      }
      ts.tv_sec += seconds;
      ts.tv_nsec = nseconds;
      return pthread_cond_timedwait(cond, mutex, &ts) == ETIMEDOUT;
    }

    void Signal(pthread_cond_t * cond)
    {
      pthread_cond_signal(cond);
    }

    void Unlock()
    {
      pthread_mutex_unlock(mutex);

      mutex = NULL;
    }

    pthread_mutex_t * Release()
    {
      pthread_mutex_t * tmp = mutex;

      mutex = NULL;

      return tmp;
    }
  };
}
