/*
Copyright (C) 2023 coolelectronics

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "os.h"
#include <cstdarg>
#include <stdarg.h>
#include <stdio.h>
#include <cstring>

bool debug = false;
void dbgprintf(const char *fmt, ...){
    if (!debug) return;
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
}

#ifdef _WIN32
void fail(const char *err)
{
    MessageBoxA(NULL, err, "ERROR", MB_ICONERROR | MB_OK);
    ExitProcess(1);
} // fail

bool createPipes(PipeType *pPipeParentRead, PipeType *pPipeParentWrite,
                        PipeType *pPipeChildRead, PipeType *pPipeChildWrite)
{
    SECURITY_ATTRIBUTES pipeAttr;

    pipeAttr.nLength = sizeof (pipeAttr);
    pipeAttr.lpSecurityDescriptor = NULL;
    pipeAttr.bInheritHandle = TRUE;
    if (!CreatePipe(pPipeParentRead, pPipeChildWrite, &pipeAttr, 0))
        return 0;

    pipeAttr.nLength = sizeof (pipeAttr);
    pipeAttr.lpSecurityDescriptor = NULL;
    pipeAttr.bInheritHandle = TRUE;
    if (!CreatePipe(pPipeChildRead, pPipeParentWrite, &pipeAttr, 0))
    {
        CloseHandle(*pPipeParentRead);
        CloseHandle(*pPipeChildWrite);
        return 0;
    } // if

    return 1;
} // createPipes

bool setEnvVar(const char *key, const char *val)
{
    return (SetEnvironmentVariableA(key, val) != 0);
} // setEnvVar

bool launchChild(ProcessType *pid)
{
    STARTUPINFOA si = { sizeof( si ) };
    memset( pid, 0, sizeof( *pid ) );

    LPSTR args;
    if (debug)
        args = STEAM_BLOB_LAUNCH_NAME " steamdebug";
    else
        args = STEAM_BLOB_LAUNCH_NAME;
    
    DWORD dwAttrib = GetFileAttributesA(STEAM_BLOB_LAUNCH_NAME);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES ||
         (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) return false;

    bool bResult = ( CreateProcessA(NULL, args, NULL, NULL, TRUE, 0, NULL,
                              NULL, &si, pid) != 0);
    return bResult;
} // launchChild

int closeProcess(ProcessType *pid)
{
    CloseHandle(pid->hProcess);
    CloseHandle(pid->hThread);
    return 0;
} // closeProcess

#if 0
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
    mainline();
    ExitProcess(0);
    return 0;  // just in case.
} // WinMain
#endif

#else  // everyone else that isn't Windows.
#include <unistd.h>

bool launchChild(ProcessType *pid)
{
    const char *exename = "./" STEAM_BLOB_LAUNCH_NAME;


    if (access(exename, F_OK) < 0) return false;

    *pid = fork();
    if (*pid == -1)   // failed
        return false;
    else if (*pid != 0)  // we're the parent
        return true;  // we'll let the pipe fail if this didn't work.

    char* GArgv[3] = {strdup(exename), NULL, NULL};
    if (debug)
        GArgv[1] = strdup("steamdebug");

    execvp(GArgv[0], GArgv);

    // still here? It failed! Terminate, closing child's ends of the pipes.
    _exit(1);
} // launchChild

int closeProcess(ProcessType *pid)
{
    int rc = 0;
    while ((waitpid(*pid, &rc, 0) == -1) && (errno == EINTR)) { /*spin*/ }
    if (!WIFEXITED(rc))
        return 1;  // oh well.
    return WEXITSTATUS(rc);
} // closeProcess



void fail(const char *err)
{
    // !!! FIXME: zenity or something.
    fprintf(stderr, "%s\n", err);
    _exit(1);
} // fail

bool createPipes(PipeType *pPipeParentRead, PipeType *pPipeParentWrite,
                        PipeType *pPipeChildRead, PipeType *pPipeChildWrite)
{
    int fds[2];
    if (pipe(fds) == -1)
        return 0;
    *pPipeParentRead = fds[0];
    *pPipeChildWrite = fds[1];

    if (pipe(fds) == -1)
    {
        close(*pPipeParentRead);
        close(*pPipeChildWrite);
        return 0;
    } // if

    *pPipeChildRead = fds[0];
    *pPipeParentWrite = fds[1];

    return 1;
} // createPipes

bool setEnvVar(const char *key, const char *val)
{
    return (setenv(key, val, 1) != -1);
} // setEnvVar

#endif

#ifdef _WIN32

int pipeReady(PipeType fd)
{
    DWORD avail = 0;
    return (PeekNamedPipe(fd, NULL, 0, NULL, &avail, NULL) && (avail > 0));
} /* pipeReady */

int writePipe(PipeType fd, const void *buf, const unsigned int _len)
{
    const DWORD len = (DWORD) _len;
    DWORD bw = 0;
    return ((WriteFile(fd, buf, len, &bw, NULL) != 0) && (bw == len));
} /* writePipe */

int readPipe(PipeType fd, void *buf, const unsigned int _len)
{
    const DWORD len = (DWORD) _len;
    DWORD br = 0;
    return ReadFile(fd, buf, len, &br, NULL) ? (int) br : -1;
} /* readPipe */

void closePipe(PipeType fd)
{
    CloseHandle(fd);
} /* closePipe */

char *getEnvVar(const char *key, char *buf, const size_t _buflen)
{
    const DWORD buflen = (DWORD) _buflen;
    const DWORD rc = GetEnvironmentVariableA(key, buf, buflen);
    /* rc doesn't count null char, hence "<". */
    return ((rc > 0) && (rc < buflen)) ? buf : NULL;
} /* getEnvVar */

#else

int pipeReady(PipeType fd)
{
    int rc;
    struct pollfd pfd = { fd, POLLIN | POLLERR | POLLHUP, 0 };
    while (((rc = poll(&pfd, 1, 0)) == -1) && (errno == EINTR)) { /*spin*/ }
    return (rc == 1);
} /* pipeReady */

int writePipe(PipeType fd, const void *buf, const unsigned int _len)
{
    const ssize_t len = (ssize_t) _len;
    ssize_t bw;
    while (((bw = write(fd, buf, len)) == -1) && (errno == EINTR)) { /*spin*/ }
    return (bw == len);
} /* writePipe */

int readPipe(PipeType fd, void *buf, const unsigned int _len)
{
    const ssize_t len = (ssize_t) _len;
    ssize_t br;
    while (((br = read(fd, buf, len)) == -1) && (errno == EINTR)) { /*spin*/ }
    return (int) br;
} /* readPipe */

void closePipe(PipeType fd)
{
    close(fd);
} /* closePipe */

char *getEnvVar(const char *key, char *buf, const size_t buflen)
{
    const char *envr = getenv(key);
    if (!envr || (strlen(envr) >= buflen))
        return NULL;
    strcpy(buf, envr);
    return buf;
} /* getEnvVar */

#endif


