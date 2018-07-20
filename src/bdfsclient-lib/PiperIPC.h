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
#include <stdint.h>
#include <Windows.h>
#include <string>
#include <algorithm>

#ifndef TEXT
#define TEXT(quote) L##quote 
#endif

#define PIPER_NAME "\\\\.\\pipe\\drvcoin\bdfs_client"
#define PIPER_SIZE 1024*1024 //kb buff
#define PIPER_TIMEOUT 500 
#define PIPER_SYNCOP_TIMEOUT 1000

class PiperIPC
{
public:
  PiperIPC()
    : m_fServer(false)
  {
    // Security descriptor
    m_sa.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    InitializeSecurityDescriptor(m_sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    // ACL is set as NULL in order to allow all access to the object.
    SetSecurityDescriptorDacl(m_sa.lpSecurityDescriptor, TRUE, NULL, FALSE);
    m_sa.nLength = sizeof(m_sa);
    m_sa.bInheritHandle = TRUE;

    m_hPipeInst = NULL;
    m_fPipeConnected = false;
  }

  ~PiperIPC()
  {
    m_fPipeConnected = false;

    Disconnect();

  }

  void Disconnect()
  {
    if (m_hPipeInst)
    {
      if (m_fServer)
      {
        FlushFileBuffers(m_hPipeInst);
        DisconnectNamedPipe(m_hPipeInst);
      }
      CloseHandle(m_hPipeInst);
      m_hPipeInst = NULL;
    }
  }

  bool IsConnected() { return m_fPipeConnected; }

  bool SyncRead(uint8_t *pBuf, uint32_t bufSize, DWORD &cbRead, DWORD timeout = PIPER_SYNCOP_TIMEOUT)
  {
    if (!pBuf) return false;

    memset(pBuf, 0, bufSize);
    cbRead = 0;
    bool fSuccess = ReadFile(
      m_hPipeInst,        // handle to pipe 
      pBuf,    // buffer to Read
      bufSize,          // bufsize 
      &cbRead, // number of bytes read 
      NULL);        // not overlapped I/O 

    if ((!fSuccess || cbRead == 0) && GetLastError() != ERROR_MORE_DATA) //  op in progress
    {
      //printf("SyncRead Error: %d\n", GetLastError());
    }
    return fSuccess;
  }

  bool SyncWrite(const uint8_t *pBuf, DWORD len, DWORD timeout = PIPER_SYNCOP_TIMEOUT)
  {
    if (!pBuf || len < 0) return false;

    DWORD cbWrite;
    bool fSuccess = WriteFile(
      m_hPipeInst,        // handle to pipe 
      pBuf,    // buffer to Write 
      len,          // size of buffer 
      &cbWrite, // number of bytes read 
      NULL);        // overlapped I/O 

    if (!fSuccess || cbWrite != len) //  op in progress
    {
      //printf("SyncWrite Error: %d\n", GetLastError());
    }
    return fSuccess;
  }

  ////////////////////// Client Specific Methods //////////////////////////
  bool AttachEndPoint(std::string pipeName = "")
  {
    m_fServer = false;

    if (pipeName.length() == 0) pipeName = PIPER_NAME;
    m_strPipeName = pipeName;

    LPTSTR lpszPipename = (LPTSTR)pipeName.c_str();

    bool m_fSuccess = false;
    // Try to open a named pipe; wait for it, if necessary. 
    HANDLE hPipe = NULL;
    while (1)
    {
      hPipe = CreateFile(
        lpszPipename,   // pipe name 
        GENERIC_READ | GENERIC_WRITE,         // read and write access 
        0,              // no sharing 
        NULL,           // default security attributes
        OPEN_EXISTING, // opens existing pipe 
        0,              // default attributes 
        NULL);          // no template file 

      // Break if the pipe handle is valid. 
      if (hPipe != INVALID_HANDLE_VALUE)
      {
        break;
      }
      // Exit if an error other than ERROR_PIPE_BUSY occurs. 
      if (GetLastError() != ERROR_PIPE_BUSY)
      {
        CloseHandle(hPipe);
        return false;
      }

      // All pipe instances are busy,// so wait for 5 seconds. 
      if (!WaitNamedPipe(lpszPipename, NMPWAIT_USE_DEFAULT_WAIT))
      {
        return false;
      }
    }

    m_hPipeInst = hPipe;
    m_fPipeConnected = true;

    return true;
  }

  bool CreateEndPoint(uint32_t pipeCount, std::string pipeName = "") // Server Method
  {
    // Read Pipe instances
    m_fServer = true;

    if (pipeName.length() == 0) pipeName = PIPER_NAME;
    m_strPipeName = pipeName;

    LPTSTR lpszPipename = (LPTSTR)pipeName.c_str();

    m_hPipeInst = CreateNamedPipe(
      lpszPipename,             // pipe name 
      PIPE_ACCESS_DUPLEX,      // read/write access
      PIPE_TYPE_MESSAGE |       // message type pipe 
      PIPE_READMODE_MESSAGE |   // message-read mode 
      PIPE_WAIT,                // blocking mode 
      std::min<DWORD>(pipeCount, PIPE_UNLIMITED_INSTANCES),               // max. instances
      PIPER_SIZE,                  // output buffer size 
      PIPER_SIZE,                  // input buffer size 
      PIPER_TIMEOUT,                        // client time-out 
      &m_sa);                    // default security attribute 

    if (m_hPipeInst == INVALID_HANDLE_VALUE)
    {
      return false;
    }

    m_fPipeConnected = ConnectNamedPipe(m_hPipeInst, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);

    return m_fPipeConnected;
  }



private:
  SECURITY_ATTRIBUTES m_sa;
  DWORD m_cbRead;
  DWORD m_cbWrite;

  HANDLE m_hPipeInst;
  bool m_fPipeConnected;
  bool m_fServer;

  std::string m_strPipeName;
};


