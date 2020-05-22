/*
    Honeywell Galaxy RS485 code for Arduino/STM32duino
    Copyright (C) 2020 Richard Antony Burton (richardaburton@gmail.com)

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
*/


#ifndef galaxyrs485_h
#define galaxyrs485_h

#include <Arduino.h>

class GalaxyRs485Class {
	
private:

  int SendPin;
  HardwareSerial *SerialPort;

public:

  void Begin(HardwareSerial *serial, int sendPin);
  void DumpHex(byte *data, int len);
  void Send(byte *data, size_t len, bool csum);
  bool CheckSum(byte *data, int len, bool set);
  int Available(void);
  size_t Read(byte *buffer, size_t len);
  
};

extern GalaxyRs485Class GalaxyRs485;

#endif
