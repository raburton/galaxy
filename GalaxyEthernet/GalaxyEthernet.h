/*
    Honeywell Galaxy RS485 code for Arduino/STM32duino
    Copyright (C) 2020 Richard Antony Burton (richardaburton@gmail.com)

    This software is dual licensed, GPLv3 or MIT, your choice:

    GPLv3
    -----
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>


    MIT
    ---
    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

*/


#ifndef galaxyethernet_h
#define galaxyethernet_h

#include <Arduino.h>

#include <GalaxyRs485.h>

// public defines
#define GALAXY_PROTOCOL_UDP 0
#define GALAXY_PROTOCOL_TCP 1

// defines for our own use
// max sia message data is 63, but ours includes record type byte and null terminator
#define SIA_LEN_MAX 65
// max packet is 2 full messages (63+3 each) + plus an a/c message (6+3) + header and csum (4+1)
#define REPORT_LEN_MAX ((2*66)+6+3+4+1)

class GalaxyEthernetClass {
	
private:

  void Crypt(byte *data, int len);
  bool ParseIp(byte *data, byte *out);
  void ParsePort(byte *data, int len, int *out);
  void SiaChecksum(byte *data, int len);

public:

  void CreateReport();
  void ProcessCommand(byte *data, int len);
  
  struct {
    unsigned long LastSend;
    char Account[8] = {0};
    char Header[SIA_LEN_MAX] = {0};
    char Message[SIA_LEN_MAX] = {0};
    byte Retries = 0;
    byte Raw[REPORT_LEN_MAX];
    byte RawLen;
    bool Unconfirmed = false;
  } Report;
  
  int PrimaryPort = 0;
  int SecondaryPort = 0;
  int MonitorPort = 0;
  byte PrimaryIp[4] = {0};
  byte SecondaryIp[4] = {0};
  byte MonitorIp[4] = {0};
  byte Ip[4] = {0};
  byte Subnet[4] = {0};
  byte Gateway[4] = {0};
  byte Format = 1;
  byte Retries = 1;
  byte Error = 0;
  byte Protocol = 0;

};

extern GalaxyEthernetClass GalaxyEthernet;

#endif
