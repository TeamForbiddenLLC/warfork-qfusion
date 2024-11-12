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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
typedef PROCESS_INFORMATION ProcessType;
typedef HANDLE PipeType;
#define NULLPIPE NULL
#include <malloc.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
typedef pid_t ProcessType;
typedef int PipeType;
#define NULLPIPE -1
#endif

/* Windows and Unix implementations of this stuff below. */
void fail(const char *err);
bool createPipes(PipeType *pPipeParentRead, PipeType *pPipeParentWrite,
                        PipeType *pPipeChildRead, PipeType *pPipeChildWrite);
bool setEnvVar(const char *key, const char *val);
bool launchChild(ProcessType *pid);
int closeProcess(ProcessType *pid);

int writePipe(PipeType fd, const void *buf, const unsigned int _len);
int readPipe(PipeType fd, void *buf, const unsigned int _len);
void closePipe(PipeType fd);
char *getEnvVar(const char *key, char *buf, const size_t buflen);
int pipeReady(PipeType fd);

void dbgprintf(const char *fmt, ...);
