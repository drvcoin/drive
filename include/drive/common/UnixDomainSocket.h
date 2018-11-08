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

#include <arpa/inet.h>
#include <unistd.h>


#define UNIX_SOC_PREFIX "\0/blocdrive/usp/"

namespace bdfs
{
  class UnixDomainSocket
  {
  private:
    int socketFd;

    static bool SetNonBlock(int fd, bool nonBlock);

    static bool WaitSendReady(int fd, int timeout);

    static bool WaitRecvReady(int fd, int timeout);

  public:
    UnixDomainSocket();
    ~UnixDomainSocket();

    bool Connect(const char *uuid, int timeout = 5);
    bool Listen(const char *uuid, int listenLength);
    UnixDomainSocket * Accept();
    void Shutdown(int flag);
    void Close();

    bool SendFd(int sendFd, int timeout = 5);
    bool RecvFd(int * recvFd, int timeout = 5);

    ssize_t Send(const void * buf, size_t nbytes, int timeout = 5);
    ssize_t Recv(void * buf, size_t nbytes, int timeout = 5);

    ssize_t SendMessage(const void * message, size_t messageLength, int timeout = 5);
    ssize_t RecvMessage(void * messageBuf, size_t messageLength, int timeout = 5);
    
    int GetSocket() { return socketFd; }
  };
}
