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


#include "GalaxyRs485.h"

// general utility function for dumping hex of byte array to serial
void GalaxyRs485Class::DumpHex(byte *data, int len) {
  for (int i = 0; i < len; i++) {
    Serial1.print(data[i], HEX);
    Serial1.print(" ");
  }
  Serial1.println();
}

// initialise the rs485 class, must be called before use
// serial: ptr to hardware serial with Rs485 adapter (e.g. Serial2)
// sendPin: pin to drive high to select send mode
void GalaxyRs485Class::Begin(HardwareSerial *serial, int sendPin) {
  SerialPort = serial;
  SendPin = sendPin;

  pinMode(sendPin, OUTPUT);
  digitalWrite(sendPin, 0);
  
  SerialPort->begin(9600);
  SerialPort->setTimeout(5);
}

// set or check checksum
// data: bytes to calculate checksum for, last byte is/will be the checksum
// len: length of data, including checksum
// set: true for setting mode, false for checking mode
// returns: in set mode always returns true
//          in check mode returns true if checksum correct, else false
bool GalaxyRs485Class::CheckSum(byte *data, int len, bool set) {
  uint32 sum = 0xAA;
  len--;
  for (int i = 0; i < len; i++) {
    sum += data[i];
  }
  //sum = ((sum & 0xff000000) >> 24) + ((sum & 0xff0000) >> 16) + ((sum & 0xff00) >> 8) + (sum & 0xff);
  sum = ((sum & 0xff00) >> 8) + (sum & 0xff); // we'll never exceed two bytes
  sum &= 0xff;
  if (set) {
    data[len] = sum;
    return true;
  } else {
    return (data[len] == sum);
  }
}

// send bytes to rs485 bus
// data: bytes to send
// len: length to send, including checksum (even if not yet calculated)
// csum: if true checksum will be calculated
void GalaxyRs485Class::Send(byte *data, size_t len, bool csum) {
  if (csum) CheckSum(data, len, true);
  digitalWrite(SendPin, 1);
  delay(1);
  SerialPort->write(data, len);
  SerialPort->flush();
  digitalWrite(SendPin, 0);
  delay(1);
}

// check if any rs485 data available to read
// returns: number of bytes available
int GalaxyRs485Class::Available(void) {
	return SerialPort->available();
}

// read bytes from rs485
// returns: number of bytes read
size_t GalaxyRs485Class::Read(byte *buffer, size_t len) {
	return SerialPort->readBytes(buffer, len);
}

GalaxyRs485Class GalaxyRs485;
