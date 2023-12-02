#include <cstdint>
#include <stddef.h>
#include <cassert>
#include <cstring>
#include "os.h"
#include "steamshim_types.h"
#include "steamshim_private.h"

PipeType GPipeRead = NULLPIPE;
PipeType GPipeWrite = NULLPIPE;

void pipebuff_t::WriteData(void* val, size_t vallen)
{
  assert(cursize + vallen < PIPEMESSAGE_MAX);
  memcpy(buffer + cursize, val, vallen);
  cursize += vallen;
}

void pipebuff_t::WriteByte(char val)
{
  WriteData(&val, sizeof val);
}

void pipebuff_t::WriteInt(int val)
{
  WriteData(&val, sizeof val);
}

void pipebuff_t::WriteFloat(float val)
{
  WriteData(&val, sizeof val);
}

void pipebuff_t::WriteLong(long long val)
{
  WriteData(&val, sizeof val);
}

void pipebuff_t::WriteString(char *val)
{
  WriteData(val, strlen(val));
}

void *pipebuff_t::ReadData(size_t vallen)
{
  assert(cursize + vallen < PIPEMESSAGE_MAX);
  void* val = cursize + buffer;
  cursize += vallen;
  return val;
}

char *pipebuff_t::ReadString()
{
  char *str = cursize + buffer;
  unsigned int len = strlen(str);
  cursize +=len;
  return str;
}

char pipebuff_t::ReadByte(){
  return *(char*)ReadData(sizeof(char));
}

int pipebuff_t::ReadInt(){
  return *(int*)ReadData(sizeof(int));
}

float pipebuff_t::ReadFloat(){
  return *(float*)ReadData(sizeof(float));
}

long long pipebuff_t::ReadLong(){
  return *(long long*)ReadData(sizeof(long long));
}




int pipebuff_t::Transmit()
{

  writePipe(GPipeWrite, &cursize, sizeof cursize);
  return writePipe(GPipeWrite, buffer, cursize);
}

int pipebuff_t::Recieve()
{
  // reset after a succesful message read. could be more explicit but idc
  if (hasmsg)
  {
    cursize = 0;
    hasmsg = false;
    memset(buffer,0,sizeof buffer);
  }


  // read message length header
  int msglen = (br > 0) ? (*(uint32_t*) &buffer[0]) : 0;

  if (br < (msglen + sizeof(uint32_t)))  /* we have an incomplete commmand. Try to read more. */
  {
      if (pipeReady(GPipeRead))
      {

          assert(br < sizeof(buffer));
          const int morebr = readPipe(GPipeRead, buffer + br, sizeof (buffer) - br);
          if (morebr > 0)
              br += morebr;
          else  /* uh oh */
          {
            return 0;
          }
      }
  }


  // we have a full command
  if (msglen && (br >= msglen + sizeof(uint32_t)))
  {
    hasmsg = true;

    br -= msglen + sizeof(uint32_t);
    if (br > 0){
      // we have extra data left over, shift it to the left
      memmove(buffer, buffer+msglen+sizeof(uint32_t), br);
    }

  }

  return 1;
}


int Write1ByteMessage(const uint8_t message){
  pipebuff_t buf;
  buf.WriteByte(message);
  return buf.Transmit();
}
