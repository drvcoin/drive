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

#include <drive/common/UnixDomainSocket.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <string.h>

#if !defined(__APPLE__)
// Leading NUL creates a socket that's not backed by the file system 
#define UNIX_SOC_PREFIX "\0/blocdrive/usp/"
#else
#define UNIX_SOC_PREFIX "/var/tmp/blocdrive.socket."
#endif

namespace bdfs
{
 
  UnixDomainSocket::UnixDomainSocket() :
    socketFd(-1)
  {
  }


  UnixDomainSocket::~UnixDomainSocket()
  {
    this->Close();
  }


  bool UnixDomainSocket::SetNonBlock(int fd, bool nonBlock)
  {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
      return false;
    }

    if (nonBlock)
    {
      flags |= O_NONBLOCK;
    }
    else
    {
      flags &= ~(O_NONBLOCK);
    }

    return fcntl(fd, F_SETFL, flags) >= 0;
  }


  bool UnixDomainSocket::WaitSendReady(int fd, int timeout)
  {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval * pTime = timeout >= 0 ? &tv : NULL;

    return select(fd + 1, NULL, &fds, NULL, pTime) > 0;
  }


  bool UnixDomainSocket::WaitRecvReady(int fd, int timeout)
  {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval * pTime = timeout >= 0 ? &tv : NULL;

    return select(fd + 1, &fds, NULL, NULL, pTime) > 0;
  }


  bool UnixDomainSocket::Connect(const char *uuid, int timeout)
  {
    int connectFd = -1;
    struct sockaddr_un serverAddr;
    int err = 0;
    socklen_t len = sizeof(err);
    bool inProgress = false;

    if (NULL == uuid)
    {
      return false;
    }

#if !defined(__APPLE__)
    if ((connectFd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0)) == -1)
    {
      return false;
    }
#else
    if ((connectFd = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
      return false;
    }
    fcntl(connectFd, O_CLOEXEC);
#endif

    bool success = false;
    int retry = 0;

    size_t serverAddrLen = sizeof(serverAddrLen);

    if (!UnixDomainSocket::SetNonBlock(connectFd, true))
    {
      goto Cleanup;
    }

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sun_family = AF_LOCAL;
    memcpy(serverAddr.sun_path, UNIX_SOC_PREFIX, sizeof(UNIX_SOC_PREFIX));
    strcat(&(serverAddr.sun_path[1]), uuid);

#if defined(__APPLE__)
    serverAddr.sun_len = sizeof(serverAddr);
    serverAddrLen = SUN_LEN(&serverAddr);
#endif

    while (connect(connectFd, (struct sockaddr *)&serverAddr, serverAddrLen) == -1)
    {
      if (timeout >= 0 && errno == EAGAIN)
      {
        if (++ retry < timeout * 10)
        {
          usleep(100 * 1000);
          continue;
        }
      }

      if (errno != EINPROGRESS)
      {
        goto Cleanup;
      }
      else
      {
        inProgress = true;
        break;
      }
    }

    if (!UnixDomainSocket::WaitSendReady(connectFd, timeout))
    {
      goto Cleanup;
    }

    if (inProgress)
    {
      if (getsockopt(connectFd, SOL_SOCKET, SO_ERROR, &err, &len) != 0 || err != 0)
        goto Cleanup;
    }

    this->socketFd = connectFd;
    success = true;

  Cleanup:

    if (!success)
    {
      close(connectFd);
    }

    return success;
  }


  bool UnixDomainSocket::Listen(const char *uuid, int listenLength)
  {
    int listenFd = -1;
    struct sockaddr_un serverAddr;

    if (NULL == uuid)
    {
      return false;
    }

#if !defined(__APPLE__)
    if ((listenFd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0)) == -1)
    {
      return false;
    }
#else
    if ((listenFd = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
      return false;
    }
    fcntl(listenFd, O_CLOEXEC);
#endif

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sun_family = AF_LOCAL;
    memcpy(serverAddr.sun_path, UNIX_SOC_PREFIX, sizeof(UNIX_SOC_PREFIX));
    strcat(&(serverAddr.sun_path[1]), uuid);

    size_t serverAddrLen = sizeof(serverAddrLen);

#if defined(__APPLE__)
    serverAddr.sun_len = sizeof(serverAddr);
    serverAddrLen = SUN_LEN(&serverAddr);
#endif

    unlink(serverAddr.sun_path);

    if (bind(listenFd, (struct sockaddr *)&serverAddr, serverAddrLen) == -1)
    {
      close(listenFd);
      return false;
    }

    if (listen(listenFd, listenLength) == -1)
    {
      close(listenFd);
      return false;
    }

    this->socketFd = listenFd;
    return true;
  }


  UnixDomainSocket * UnixDomainSocket::Accept()
  {
    UnixDomainSocket * clientSocket = NULL;
    struct sockaddr_un clientAddr;
    socklen_t sockLen = sizeof(struct sockaddr);
   
#if !defined(__APPLE__)
    int clientFd = accept4(this->socketFd, (struct sockaddr *)&clientAddr, &sockLen, SOCK_CLOEXEC);
#else
    int clientFd = accept(this->socketFd, (struct sockaddr *)&clientAddr, &sockLen);
    if (clientFd != -1)
    {
      fcntl(clientFd, O_CLOEXEC);
    }
#endif

    if (clientFd != -1 &&
        UnixDomainSocket::SetNonBlock(clientFd, true) &&
        (clientSocket = new UnixDomainSocket()) != NULL)
    {
      clientSocket->socketFd = clientFd;
    }

    return clientSocket;
  }


  void UnixDomainSocket::Shutdown(int flag)
  {
    if (this->socketFd != -1)
    {
      shutdown(this->socketFd, flag);
    }
  }


  void UnixDomainSocket::Close()
  {
    if (this->socketFd != -1)
    {
      close(this->socketFd);
      this->socketFd = -1;
    }
  }


  bool UnixDomainSocket::SendFd(int sendFd, int timeout)
  {
    char buf[1];
    struct msghdr msg;
    struct iovec iov[1];

    union {
      struct cmsghdr cm;
      char control[CMSG_SPACE(sizeof(int))];
    } control_un;

    struct cmsghdr *cmPtr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmPtr = CMSG_FIRSTHDR(&msg);
    cmPtr->cmsg_len = CMSG_LEN(sizeof(int));
    cmPtr->cmsg_level = SOL_SOCKET;
    cmPtr->cmsg_type = SCM_RIGHTS;

    *((int *)CMSG_DATA(cmPtr)) = sendFd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if (!UnixDomainSocket::WaitSendReady(this->socketFd, timeout))
    {
      return false;
    }

    if (sendmsg(this->socketFd, &msg, 0) == 1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }


  bool UnixDomainSocket::RecvFd(int * recvFd, int timeout)
  {
    char buf[1];
    struct msghdr msg;
    struct iovec iov[1];
    ssize_t readLength = 0;

    if (NULL == recvFd)
    {
      return false;
    }

    union {
      struct cmsghdr cm;
      char control[CMSG_SPACE(sizeof(int))];
    } control_un;

    struct cmsghdr *cmPtr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if (!UnixDomainSocket::WaitRecvReady(this->socketFd, timeout))
    {
      return false;
    }

    if ((readLength = recvmsg(this->socketFd, &msg, 0)) <= 0)
    {
      return false;
    }

    if ((cmPtr = CMSG_FIRSTHDR(&msg)) != NULL &&
        cmPtr->cmsg_len == CMSG_LEN(sizeof(int)))
    {
      if (cmPtr->cmsg_level != SOL_SOCKET ||
          cmPtr->cmsg_type != SCM_RIGHTS)
      {
        return false;
      }

      *recvFd = *((int *)CMSG_DATA(cmPtr));
      return true;
    }
    else
    {
      *recvFd = -1;
      return false;
    }
  }

  ssize_t UnixDomainSocket::Send(const void * buf, size_t nbytes, int timeout)
  {
    if (!UnixDomainSocket::WaitSendReady(this->socketFd, timeout))
    {
      return -1;
    }

    return send(this->socketFd, buf, nbytes, 0);
  }


  ssize_t UnixDomainSocket::Recv(void * buf, size_t nbytes, int timeout)
  {
    if (!UnixDomainSocket::WaitRecvReady(this->socketFd, timeout))
    {
      return -1;
    }

    return recv(this->socketFd, buf, nbytes, 0);
  }


  ssize_t UnixDomainSocket::SendMessage(const void * message, size_t messageLength, int timeout)
  {
    size_t sent = 0;
    int n = 0;

    while (sent < messageLength)
    {
      n = this->Send((const unsigned char *)message + sent, messageLength - sent, timeout);
      
      if (n <= 0)
        break;

      sent += n;
    }

    return (n <= 0) ? n : sent;
  }

  ssize_t UnixDomainSocket::RecvMessage(void * messageBuf, size_t messageLength, int timeout)
  {
    size_t recved = 0;
    int n = 0;

    while (recved < messageLength)
    {
      n = this->Recv((unsigned char *)messageBuf + recved, messageLength - recved, timeout);

      if (n <= 0)
        break;

      recved += n;
    }

    return (n <= 0) ? n : recved;
  }
}
