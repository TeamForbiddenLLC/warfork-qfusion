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

#include <cstdint>
#include <stddef.h>
#include <cassert>
#include <cstring>
#include "os.h"
#include "steamshim_types.h"
#include "steamshim_private.h"
#include <mutex>

PipeType GPipeRead = NULLPIPE;
PipeType GPipeWrite = NULLPIPE;
std::mutex writeGuard;

void PipeBuffer::WriteData(const void* val, size_t vallen)
{
  assert(cursor + vallen < PIPEMESSAGE_MAX);
  memcpy(buffer + cursor, val, vallen);
  cursor += vallen;
}

void PipeBuffer::WriteByte(char val)
{
  WriteData(&val, sizeof val);
}

void PipeBuffer::WriteInt(int val)
{
  WriteData(&val, sizeof val);
}

void PipeBuffer::WriteFloat(float val)
{
  WriteData(&val, sizeof val);
}

void PipeBuffer::WriteLong(uint64_t val)
{
  WriteData(&val, sizeof val);
}

void PipeBuffer::WriteString(const char *val)
{
  WriteData(val, strlen(val)+1);
}

void *PipeBuffer::ReadData(size_t vallen)
{
  assert(cursor + vallen < PIPEMESSAGE_MAX);
  void* val = cursor + buffer;
  cursor += vallen;
  return val;
}

char *PipeBuffer::ReadString()
{
  char *str = cursor + buffer;
  unsigned int len = strlen(str)+1;
  cursor +=len;
  return str;
}

char PipeBuffer::ReadByte(){
  return *(char*)ReadData(sizeof(char));
}

int PipeBuffer::ReadInt(){
  return *(int*)ReadData(sizeof(int));
}

float PipeBuffer::ReadFloat(){
  return *(float*)ReadData(sizeof(float));
}

uint64_t PipeBuffer::ReadLong(){
  return *(uint64_t*)ReadData(sizeof(long long));
}



extern int GArgc;
extern char **GArgv;


PipeBuffer::PipeBuffer(){
  memset(buffer,0,sizeof buffer);
}

int PipeBuffer::Transmit()
{
  writeGuard.lock();
  writePipe( GPipeWrite, &cursor, sizeof cursor );
  const int result = writePipe( GPipeWrite, buffer, cursor );
  writeGuard.unlock();
  return result;
}

int PipeBuffer::Recieve()
{
  // reset cursor after a succesful message read
  if (hasmsg)
  {
    cursor = 0;
    hasmsg = false;
  }

  if (lastmsglen > 0) {
    // we have extra data left over, shift it to the left
    memmove(buffer, buffer+lastmsglen+sizeof(uint32_t), bytesRead);
    lastmsglen = 0;
  }


  // read message length header
  int msglen = (bytesRead > 0) ? (*(uint32_t*) &buffer[0]) : 0;

  if (bytesRead < (msglen + sizeof(uint32_t)))  /* we have an incomplete commmand. Try to read more. */
  {
      if (pipeReady(GPipeRead))
      {

          assert(bytesRead < sizeof(buffer));
          const int morebr = readPipe(GPipeRead, buffer + bytesRead, sizeof (buffer) - bytesRead);
          if (morebr > 0)
              bytesRead += morebr;
          else  /* uh oh */
          {
            return 0;
          }
      }
  }


  // we have a full command
  if (msglen && (bytesRead >= msglen + sizeof(uint32_t)))
  {
    hasmsg = true;

    bytesRead -= msglen + sizeof(uint32_t);
    if (bytesRead > 0)
      lastmsglen = msglen;
  }

  return 1;
}


int Write1ByteMessage(const uint8_t message){
  PipeBuffer buf;
  buf.WriteByte(message);
  return buf.Transmit();
}
