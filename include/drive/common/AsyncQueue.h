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

#include <drive/common/LockFreeQueue.h>

#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace bdfs
{
  template <class T>
  class AsyncQueue
  {
  public:
    typedef void (* Callback)(T & item, void * context);
    typedef void (* EmptyCallback)(void * context);

  private:
    LockFreeQueue<T> outQueue;

    Callback onDequeue;
    Callback onDelete;
    EmptyCallback onEmpty;

    volatile uint32_t m_queuedCount;

    void * context;
    bool done;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE event;
    HANDLE thread;
#else
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
#endif

  public:
    AsyncQueue(Callback onDequeue, Callback onDelete, void * context, EmptyCallback onEmpty = NULL) :
      onDequeue(onDequeue),
      onDelete(onDelete),
      onEmpty(onEmpty),
      m_queuedCount(0),
      context(context),
      done(false),
#if defined(_WIN32) || defined(_WIN64)
      event(INVALID_HANDLE_VALUE),
      thread(INVALID_HANDLE_VALUE)
#else
      thread(0)
#endif
    {
#if !(defined(_WIN32) || defined(_WIN64))
      pthread_cond_init(&cond, NULL);
      pthread_mutex_init(&mutex, NULL);
#endif
    }

    ~AsyncQueue()
    {
      if (onDelete)
      {
        T item;

        while (outQueue.Consume(item))
        {
          ATOMIC_DECS32(& m_queuedCount);
          onDelete(item, context);
        }
      }

#if defined(_WIN32) || defined(_WIN64)
      CloseHandle(event);
#else
      pthread_cond_destroy(&cond);
      pthread_mutex_destroy(&mutex);
#endif
    }

    bool Start()
    {
      done = false;
#if defined(_WIN32) || defined(_WIN64)
      event = CreateEvent(NULL, true, false, NULL);
      if (event == INVALID_HANDLE_VALUE)
      {
        return false;
      }
      thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ThreadStart, this, 0, NULL);
      if (thread == INVALID_HANDLE_VALUE)
      {
        return false;
      }
#else
      int res;

      if ((res = pthread_cond_init(&cond, NULL)) != 0)
      {
        return false;
      }

      if ((res = pthread_mutex_init(&mutex, NULL)) != 0)
      {
        return false;
      }

      if ((res = pthread_create(&thread, NULL, &ThreadStart, this)) != 0)
      {
        return false;
      }
#endif

      return true;
    }

    void SetMaxCount(int32_t value)
    {
      outQueue.SetMaxCount(value);
    }

    bool Enqueue(const T & item)
    {
      if (!done)
      {
        uint32_t count = ATOMIC_INCS32(& m_queuedCount);
        outQueue.Produce(item);

        if (count == 1)
        {
#if defined(_WIN32) || defined(_WIN64)
          SetEvent(event);
#else
          pthread_mutex_lock(&mutex);
          pthread_cond_signal(&cond);
          pthread_mutex_unlock(&mutex);
#endif
        }
      }

      return true;
    }

    bool Stop()
    {
      done = true;

#if defined(_WIN32) || defined(_WIN64)
      SetEvent(event);

      WaitForSingleObject(thread, INFINITE);
#else
      pthread_mutex_lock(&mutex);
      pthread_cond_signal(&cond);
      pthread_mutex_unlock(&mutex);

      pthread_join(thread, NULL);
#endif

      return true;
    }

    bool Reset()
    {
      if (done)
      {
        T item;

        if (onDelete)
        {
          while (outQueue.Consume(item))
          {
            ATOMIC_DECS32(& m_queuedCount);
            onDelete(item, context);
          }
        }
        else
        {
          while (outQueue.Consume(item))
          {
            ATOMIC_DECS32(& m_queuedCount);
          }
        }
        
        done = false;

        return true;
      }

      return false;
    }

  private:
    static void * ThreadStart(void * state)
    {
      AsyncQueue<T> * _this = (AsyncQueue<T> *)state;

      _this->SendLoop();

      return 0;
    }

    void SendLoop()
    {
      T item;

      while (!done)
      {
        if (outQueue.Consume(item))
        {
          ATOMIC_DECS32(& m_queuedCount);

          if (onDequeue)
          {
            onDequeue(item, context);
          }
          
          if (onDelete)
          {
            onDelete(item, context);
          }
        }
        else
        {
          if (onEmpty)
          {
            onEmpty(context);
          }

#if defined(_WIN32) || defined(_WIN64)
          WaitForSingleObject(event, INFINITE);
          ResetEvent(event);
#else
          pthread_mutex_lock(&mutex);
          while (m_queuedCount == 0 && !done)
          {
            pthread_cond_wait(&cond, &mutex);      
          }
          pthread_mutex_unlock(&mutex);
#endif
        }
      }
    }
  };
}
