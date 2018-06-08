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

#include <utility>
#include <limits>
#include <pthread.h>
#include "Event.h"
#include "LockFreeQueue.h"

namespace bdfs
{
  template<typename EventType>
  class EventLoop
  {
  public:

    typedef bool (* EventHandler)(void * sender, EventType e);

  public:

    EventLoop(EventHandler defaultHandler)
      : defaultHandler(defaultHandler)
      , queueEvent(NULL)
      , active(false)
      , quit(false)
    {
    }


    ~EventLoop()
    {
    }


    bool Start()
    {
      this->quit = false;

      this->queueEvent = new Event();

      if (!this->queueEvent->Initialize())
      {
        return false;
      }

      if (pthread_create(& this->thread, NULL, & LoopThread, this) != 0)
      {
        return false;
      }

      this->active = true;

      return true;
    }


    void Stop()
    {
      this->quit = true;

      if (this->active)
      {
        if (this->queueEvent)
        {
          this->queueEvent->Signal();
        }

        pthread_join(this->thread, NULL);

        this->active = false;
      }

      if (this->queueEvent)
      {
        delete this->queueEvent;

        this->queueEvent = NULL;
      }
    }


    void SendEvent(void * sender, EventType e)
    {
      if (!this->active || this->quit)
      {
        return;
      }

      std::pair<void *, EventType> value(sender, e);

      if (this->eventQueue.Produce(value))
      {
        this->queueEvent->Signal();
      }
    }


  private:

    static void * LoopThread(void * args)
    {
      EventLoop<EventType> * _this = (EventLoop<EventType> *)args;

      while (!_this->quit)
      {
        std::pair<void *, EventType> event;

        while (_this->eventQueue.Consume(event) && !_this->quit)
        {
          if (_this->defaultHandler)
          {
            if (!_this->defaultHandler(event.first, event.second))
            {
              return NULL;
            }
          }
        }

        if (!_this->quit)
        {
          _this->queueEvent->Wait(std::numeric_limits<unsigned int>::max());
        }
      }

      return NULL;
    }


  private:

    LockFreeQueue<std::pair<void *, EventType> > eventQueue;

    EventHandler defaultHandler;

    pthread_t thread;

    Event * queueEvent;

    bool active;

    volatile bool quit;
  };
}
