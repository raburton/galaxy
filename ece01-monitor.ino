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


#define RS485SEND PA1
#define READBUFFERLEN 50

#include <Ethernet_STM.h>
#include <SPI.h>

// for official core
//HardwareSerial Serial2(PA3,PA2);

bool reply = false;

// CSUM - placeholder for easier reading
#define CSUM 0xff

// device id to monitor
#define MONITORID 0x25

byte CheckCheckSum(byte *data, byte len) {
  uint sum = 0xAA;
  for (int i = 0; i < (len - 1); i++) {
    sum += data[i];
  }
  sum = ((sum & 0xff000000) >> 24) + ((sum & 0xff0000) >> 16) + ((sum & 0xff00) >> 8) + (sum & 0xff);
  return (data[len - 1] == (byte)(sum & 0xff));
}

void DumpHex(byte *data, byte len) {
  for (int i = 0; i < len; i++) {
    Serial1.print(data[i], HEX);
    Serial1.write(" ");
  }
  Serial1.write("\n");
}

void ServiceRS485() {
  size_t len = 0;
  byte data[READBUFFERLEN];

  if (Serial2.available() > 0) {
    len = Serial2.readBytes(data, READBUFFERLEN);
  }

  if (len < 1) return;
  
  if (reply) {
    DumpHex(data, len);
    reply = false;
  //} else if ((len > 2) && (data[0] == MONITORID) && (data[1] != 0x1a)) {
  } else if ((len > 2) && (data[0] == MONITORID)) {
    DumpHex(data, len);
    reply = true;
  } else {
    reply = false;
  }
  
}

void setup() {

  pinMode(RS485SEND, OUTPUT);
  digitalWrite(RS485SEND, 0);
  
  //pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, 0);
  
  Serial1.begin(115200);
  Serial2.begin(9600);
  Serial2.setTimeout(5);
}

void loop() {
  ServiceRS485();
}
