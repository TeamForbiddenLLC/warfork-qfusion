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
#include "steamshim_types.h"
#include <cstdint>

extern PipeType GPipeRead;
extern PipeType GPipeWrite;
extern bool debug;

class PipeBuffer
{
  public:
  PipeBuffer();

  bool hasmsg = false;

  void WriteData(const void* val, size_t vallen);
  void WriteByte(char val);
  void WriteInt(int val);
  void WriteFloat(float val);
  void WriteLong(uint64_t val);
  void WriteString(const char *val);

  void *ReadData(size_t vallen);
  char *ReadString();
  char ReadByte();
  int ReadInt();
  float ReadFloat();
  uint64_t ReadLong();

  int Transmit();
  int Recieve();

  private:
  char buffer[PIPEMESSAGE_MAX];
  unsigned int cursor = 0;
  int bytesRead = 0;
  int lastmsglen = 0;

};

int ReadMessage(char* buf);
int Write1ByteMessage(const uint8_t message);
