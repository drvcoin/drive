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

#include "Event.h"
#include "AutoMutex.h"

namespace bdfs
{

  Event::Event()
    : isSet(false)
  {
    memset(&this->cond, 0, sizeof(this->cond));
    memset(&this->mutex, 0, sizeof(this->mutex));
  }

  Event::~Event()
  {
    pthread_cond_destroy(&this->cond);
    pthread_mutex_destroy(&this->mutex);
  }

  bool Event::Initialize()
  {
    int res = -1;
    if ((res = pthread_cond_init(&this->cond, NULL)) != 0)
    {
      return false;
    }
    if ((res = pthread_mutex_init(&this->mutex, NULL)) != 0)
    {
      return false;
    }
    return true;
  }  

  bool Event::Wait(unsigned int milliseconds)
  {
    int res = -1;

    if ((res = gettimeofday(&this->tp, NULL)) != 0) 
    {
      return false;
    }

    this->ts.tv_sec = this->tp.tv_sec + milliseconds/1000 + ((milliseconds%1000 + (this->tp.tv_usec/1000))/1000);
    this->ts.tv_nsec = (milliseconds%1000 + (this->tp.tv_usec/1000))%1000 * 1000000;
    
    AutoMutex lock(&this->mutex);
    if (!isSet)
    {
      if ((res = pthread_cond_timedwait(&this->cond, &this->mutex, &this->ts)) != 0)
      {
        return false;
      }
    }
    else
    {
      isSet = false;
    }
    return true;
  }

  bool Event::Signal()
  {
    int res = -1;
    
    AutoMutex lock(&this->mutex);
    this->isSet = true;
    if ((res = pthread_cond_signal(&this->cond)) != 0)
    {
      return false;
    }
    return true;
  }
}
