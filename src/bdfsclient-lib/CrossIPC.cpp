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

#include <drive/client/CrossIPC.h>

CrossIPC::CrossIPC()
{
#if !defined(_WIN32)
  unixSocket.Listen("bdfsclient", 2);
#endif
}


CrossIPC::~CrossIPC()
{
}


bool CrossIPC::Listen()
{
#if defined(_WIN32)
  return ipc.CreateEndPoint(1);
#else
  UnixDomainSocket * socket = this->unixSocket.Accept();
  return (socket == NULL);
#endif

}

bool CrossIPC::Attach()
{
#if defined(_WIN32)
  return ipc.AttachEndPoint();
#endif
}

void CrossIPC::Disconnect()
{
#if defined(_WIN32)
  ipc.Disconnect();
#endif
}

bool CrossIPC::Read(uint8_t *pBuf, uint32_t bufSize, uint32_t &cbRead)
{
  bool ret = false;
#if defined(_WIN32)
  DWORD dwRead;
  ret = ipc.SyncRead(pBuf, bufSize, dwRead);
  cbRead = dwRead;
#endif
  return ret;
}

bool CrossIPC::Write(const uint8_t *pBuf, uint32_t bufSize)
{
  bool ret = false;
#if defined(_WIN32)
  ret = ipc.SyncWrite(pBuf, bufSize);
#endif
  return ret;
}
