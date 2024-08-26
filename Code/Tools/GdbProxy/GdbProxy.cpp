#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>

#include <atomic>
#include <mutex>
#include <thread>

// Enable the below define for debugging
// #define OUTPUT_STREAMS_TO_FILE
#define BUFSIZE 4096

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_ERR_Rd = NULL;
HANDLE g_hChildStd_ERR_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

std::atomic<bool> g_runThreads = true;

PROCESS_INFORMATION CreateChildProcess(int iArgc, TCHAR* pArgv[]);
void ForwardThread(HANDLE hReadHandle, HANDLE hWriteHandle, const char* szLogname, const char* szLogMode);
void ForwardStdin(HANDLE hReadHandle, HANDLE hWriteHandle, HANDLE hParentStdout);
void ErrorExit(const wchar_t*);

int _tmain(int iArgc, TCHAR* pArgv[])
{
#ifdef OUTPUT_STREAMS_TO_FILE
  FILE* argumentFile = fopen("H:\\temp\\arguments.txt", "w");
  fwprintf_s(argumentFile, L"Arguments %d\n", argc);
  for (int i = 0; i < argc; i++)
  {
    fwprintf_s(argumentFile, L"%ws\n", argv[i]);
  }
  fclose(argumentFile);
#endif

  SECURITY_ATTRIBUTES saAttr = {};
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  // Create a pipe for the child process's STDOUT.
  if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
    ErrorExit(TEXT("StdoutRd CreatePipe"));

  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    ErrorExit(TEXT("Stdout SetHandleInformation"));

  // Create a pipe for the child process's STDERR.
  if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0))
    ErrorExit(TEXT("StdoutRd CreatePipe"));

  // Ensure the read handle to the pipe for STDERR is not inherited.
  if (!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0))
    ErrorExit(TEXT("Stdout SetHandleInformation"));

  // Create a pipe for the child process's STDIN.
  if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
    ErrorExit(TEXT("Stdin CreatePipe"));

  // Ensure the write handle to the pipe for STDIN is not inherited.
  if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
    ErrorExit(TEXT("Stdin SetHandleInformation"));

  // Create the child process.

  auto childProcess = CreateChildProcess(iArgc, pArgv);

  HANDLE parentStdin = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE parentStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  HANDLE parentStderr = GetStdHandle(STD_ERROR_HANDLE);

  std::thread stdinThread([&]()
    { ForwardStdin(parentStdin, g_hChildStd_IN_Wr, parentStdout); });
  std::thread stdoutThread([&]()
    { ForwardThread(g_hChildStd_OUT_Rd, parentStdout, "H:\\temp\\stdout.log", "wb"); });
  std::thread stderrThread([&]()
    { ForwardThread(g_hChildStd_ERR_Rd, parentStderr, "H:\\temp\\stderr.log", "wb"); });

  WaitForSingleObject(childProcess.hProcess, INFINITE);
  g_runThreads = false;

  DWORD exitCode = 0;
  GetExitCodeProcess(childProcess.hProcess, &exitCode);

  HANDLE stdinThreadHandle = stdinThread.native_handle();
  stdinThread.detach();
  CancelSynchronousIo(stdinThreadHandle);

  CloseHandle(childProcess.hThread);
  CloseHandle(childProcess.hProcess);

  stdoutThread.join();
  stderrThread.join();

  return exitCode;
}

BOOL FileExists(const TCHAR* pPath)
{
  DWORD dwAttrib = GetFileAttributes(pPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
PROCESS_INFORMATION CreateChildProcess(int iArgc, TCHAR* pArgv[])
{
  TCHAR appTmp[] = TEXT("%ANDROID_NDK_HOME%\\prebuilt\\windows-x86_64\\bin\\gdb.exe");
  TCHAR app[MAX_PATH] = {};

  DWORD numCharsWritten = ExpandEnvironmentStringsW(appTmp, app, ARRAYSIZE(app));
  if (numCharsWritten == 0 || numCharsWritten > ARRAYSIZE(app))
  {
    ErrorExit(TEXT("Failed to build the gdb executable path. Check your ANDROID_NDK_HOME environment variable"));
  }

  if (!FileExists(app))
  {
    TCHAR msg[2048];
    StringCchPrintf(msg, ARRAYSIZE(msg),
      TEXT("Failed to find gdb.exe in the expected location '%s'. Please check your ANDROID_NDK_HOME environment variable."), app);
    ErrorExit(msg);
  }

  TCHAR szCmdline[4096];
  wsprintf(szCmdline, L"\"%ws\"", app);
  for (int i = 1; i < iArgc; i++)
  {
    TCHAR* start = szCmdline + wcslen(szCmdline);
    wsprintf(start, L" \"%ws\"", pArgv[i]);
  }

  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  BOOL bSuccess = FALSE;

  // Set up members of the PROCESS_INFORMATION structure.

  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

  // Set up members of the STARTUPINFO structure.
  // This structure specifies the STDIN and STDOUT handles for redirection.

  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO);
  siStartInfo.hStdError = g_hChildStd_ERR_Wr;
  siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
  siStartInfo.hStdInput = g_hChildStd_IN_Rd;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  // Create the child process.

  bSuccess = CreateProcess(NULL,
    szCmdline,        // command line
    NULL,             // process security attributes
    NULL,             // primary thread security attributes
    TRUE,             // handles are inherited
    CREATE_NO_WINDOW, // creation flags
    NULL,             // use parent's environment
    NULL,             // use parent's current directory
    &siStartInfo,     // STARTUPINFO pointer
    &piProcInfo);     // receives PROCESS_INFORMATION

  // If an error occurs, exit the application.
  if (!bSuccess)
    ErrorExit(TEXT("CreateProcess"));
  else
  {
    // Close handles to the stdin and stdout pipes no longer needed by the child process.
    // If they are not explicitly closed, there is no way to recognize that the child process has ended.

    CloseHandle(g_hChildStd_IN_Rd);
    CloseHandle(g_hChildStd_OUT_Wr);
    CloseHandle(g_hChildStd_ERR_Wr);
  }

  return piProcInfo;
}

void ErrorExit(const wchar_t* pLpszFunction)

// Format a readable error message, display a message box,
// and exit from the application.
{
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

  lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)pLpszFunction) + 40) * sizeof(TCHAR));
  StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"), pLpszFunction, dw, lpMsgBuf);
  MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
  ExitProcess(1);
}

void ForwardThread(HANDLE hReadHandle, HANDLE hWriteHandle, const char* szLogname, const char* szLogmode)
{
#ifdef OUTPUT_STREAMS_TO_FILE
  FILE* log = fopen(logname, logmode);
#endif

  char buffer[4096];

  while (g_runThreads)
  {
    DWORD numberOfBytesRead = 0;
    if (ReadFile(hReadHandle, buffer, ARRAYSIZE(buffer), &numberOfBytesRead, NULL))
    {
      const char* writeBuf = buffer;
      while (numberOfBytesRead > 0)
      {
#ifdef OUTPUT_STREAMS_TO_FILE
        if (log)
        {
          fwrite(buffer, 1, numberOfBytesRead, log);
          fflush(log);
        }
#endif
        DWORD bytesWritten = 0;
        if (WriteFile(hWriteHandle, writeBuf, numberOfBytesRead, &bytesWritten, NULL))
        {
          numberOfBytesRead -= bytesWritten;
          writeBuf += bytesWritten;
        }
        else
        {
          goto error;
        }
      }
    }
    else
    {
      break;
    }
  }

error:

#ifdef OUTPUT_STREAMS_TO_FILE
  if (log)
  {
    fflush(log);
    fclose(log);
  }
#endif
  return;
}

void ForwardStdin(HANDLE hReadHandle, HANDLE hWriteHandle, HANDLE hParentStdout)
{
#ifdef OUTPUT_STREAMS_TO_FILE
  FILE* log = fopen("H:\\temp\\stdin.log", "wb");
#endif

  const char* solibSearchPathCmd = "-gdb-set solib-search-path";
  DWORD solibSearchPathCmdLength = (DWORD)strlen(solibSearchPathCmd);
  const char* fileExecAndSymbolsCmd = "-file-exec-and-symbols";
  DWORD fileExecAndSymbolsCmdLength = (DWORD)strlen("-file-exec-and-symbols");

  bool solibSearchPathSkipped = false;
  bool fileExecAndSymbolsSkipped = false;

  char buffer[4096];

  while (g_runThreads && (!solibSearchPathSkipped || !fileExecAndSymbolsSkipped))
  {
    DWORD numberOfBytesRead = 0;
    if (ReadFile(hReadHandle, buffer, ARRAYSIZE(buffer), &numberOfBytesRead, NULL))
    {
      const char* writeBuf = buffer;
      while (numberOfBytesRead > 0)
      {
#ifdef OUTPUT_STREAMS_TO_FILE
        if (log)
        {
          fwrite(buffer, 1, numberOfBytesRead, log);
        }
#endif

        if (!solibSearchPathSkipped && numberOfBytesRead > solibSearchPathCmdLength + 4 &&
            strncmp(buffer + 4, solibSearchPathCmd, solibSearchPathCmdLength) == 0)
        {
          solibSearchPathSkipped = true;
          numberOfBytesRead = 0;
          char answer[256];
          sprintf_s(answer, "%.*s^done\r\n(gdb) \r\n", 4, buffer);
          DWORD bytesWritten = 0;
          if (!WriteFile(hParentStdout, answer, (DWORD)strlen(answer), &bytesWritten, NULL))
          {
            goto error;
          }
        }
        else if (!fileExecAndSymbolsSkipped && numberOfBytesRead > fileExecAndSymbolsCmdLength + 4 &&
                 strncmp(buffer + 4, fileExecAndSymbolsCmd, fileExecAndSymbolsCmdLength) == 0)
        {
          fileExecAndSymbolsSkipped = true;
          numberOfBytesRead = 0;
          char answer[256];
          sprintf_s(answer, "%.*s^done\r\n(gdb) \r\n", 4, buffer);
          DWORD bytesWritten = 0;
          if (!WriteFile(hParentStdout, answer, (DWORD)strlen(answer), &bytesWritten, NULL))
          {
            goto error;
          }
        }
        else
        {
          DWORD bytesWritten = 0;
          if (WriteFile(hWriteHandle, writeBuf, numberOfBytesRead, &bytesWritten, NULL))
          {
            numberOfBytesRead -= bytesWritten;
            writeBuf += bytesWritten;
          }
          else
          {
            goto error;
          }
        }
      }
    }
    else
    {
      break;
    }
  }

  while (g_runThreads)
  {
    DWORD numberOfBytesRead = 0;
    if (ReadFile(hReadHandle, buffer, ARRAYSIZE(buffer), &numberOfBytesRead, NULL))
    {
      const char* writeBuf = buffer;
      while (numberOfBytesRead > 0)
      {
#ifdef OUTPUT_STREAMS_TO_FILE
        if (log)
        {
          fwrite(buffer, 1, numberOfBytesRead, log);
        }
#endif
        if (numberOfBytesRead >= 6 && numberOfBytesRead <= 10 && strncmp(buffer, "logout", 6) == 0)
        {
          DWORD bytesWritten = 0;
          numberOfBytesRead = 0;
          if (!WriteFile(hWriteHandle, "quit\r\n", 6, &bytesWritten, NULL))
          {
            goto error;
          }
        }
        else
        {
          DWORD bytesWritten = 0;
          if (WriteFile(hWriteHandle, writeBuf, numberOfBytesRead, &bytesWritten, NULL))
          {
            numberOfBytesRead -= bytesWritten;
            writeBuf += bytesWritten;
          }
          else
          {
            goto error;
          }
        }
      }
    }
    else
    {
      break;
    }
  }

error:

#ifdef OUTPUT_STREAMS_TO_FILE
  if (log)
  {
    fclose(log);
  }
#endif
  return;
}
