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


#include "GalaxyEthernet.h"

void GalaxyEthernetClass::Crypt(byte *data, int len) {
  byte key[] = { 0x86, 0x93, 0xb5, 0x37, 0x12, 0xd6, 0xe4, 0x77 };
  for (int i = 0; i < len; i++) {
    data[i] ^= key[i % 8];
  }
}

// parse ip string, updates byte array and returns true if
// it's changed from stored value, else returns false
bool GalaxyEthernetClass::ParseIp(byte *data, byte *out) {
  int i = 0;
  byte temp[4] = {0};
  char *item = strtok((char*)(data+3), ".");
  while ((item != NULL) && (i < 4)) {
    temp[i] = (byte)atol(item);
    //Serial1.println(temp[i]);
    item = strtok(NULL, ".");
    i++;
  }
  //Serial1.println(*((int32*)out));
  //Serial1.println(*((int32*)temp));
  if (*((int32*)out) != *((int32*)temp)) {
    *((int32*)out) = *((int32*)temp);
    return true;
  }
  return false;
}

void GalaxyEthernetClass::ParsePort(byte *data, int len, int *out) {
  *out = data[len-3];
  *out <<= 8;
  *out += data[len-2];
}

// set sia checksum
// data: bytes to calculate checksum for, last byte is/will be the checksum
// len: length of data, including checksum
// returns: true if checksum correct, else false
void GalaxyEthernetClass::SiaChecksum(byte *data, int len) {
  byte sum = 0xff;
  for (int i = 0; i < len-1; i++) {
    sum ^= data[i];
  }
  data[len-1] = sum;
}

void GalaxyEthernetClass::CreateReport() {
  //byte accLen = strlen(Report.Account) + 1;
  int accLen = 7;
  int hdrLen = strlen(Report.Header);
  int msgLen = strlen(Report.Message);
  // each sia block is msg len + 3 (1 byte for length, 1 for record type, 1 for sia csum)
  // but our string includes the record type already, so just add 2 to length
  // packet has 4 byte header and 1 byte checksum (rs485 type), so total = sum of lengths + (3*2) + 5
  Report.RawLen = 11 + accLen + hdrLen + msgLen;
  Serial1.print("msg len: ");
  Serial1.println(Report.RawLen);
  if (Report.RawLen > REPORT_LEN_MAX) {
    // TODO: Need to report this
  } else {
    memset(Report.Raw, 0xcc, Report.RawLen);
    byte *ptr = Report.Raw;
    *ptr++ = 0xa0;
    ptr += 3;
    *ptr = (accLen - 1);
    memcpy(ptr+1, Report.Account, accLen);
    SiaChecksum(ptr, accLen + 2);
    ptr+=(accLen + 2);
    *ptr = (hdrLen-1);
    memcpy(ptr+1, Report.Header, hdrLen);
    SiaChecksum(ptr, hdrLen + 2);
    ptr+=(hdrLen+2);
    *ptr = (msgLen-1);
    memcpy(ptr+1, Report.Message, msgLen);
    SiaChecksum(ptr, msgLen + 2);
    ptr+= (msgLen+2);

    Crypt(Report.Raw, Report.RawLen - 1);
    GalaxyRs485.CheckSum(Report.Raw, Report.RawLen, true);
    //Serial1.print("report: ");
    //DumpHex(Report.Raw, Report.RawLen);

    // mark for sending
    Report.Unconfirmed = true;
    Retries = Retries;
  }
}

// process a command sent from the panel,
// sends replies via the global GalaxyRs485 object
// data: full rs485 message, inc. checksum
// len: length of data
void GalaxyEthernetClass::ProcessCommand(byte *data, int len) {
  byte cmd = data[1];
  // basic ok reply, used for almost everything
  byte reply[] = { 0x11, 0xEC, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x6C };

  if (!GalaxyRs485.CheckSum(data, len, false)) {
    // todo: confirm
    byte reply[] = { 0x11, 0xFE, 0x00, 0xBA };
    GalaxyRs485.Send(reply, 4, false);
    return;
  }
  
  if ((cmd == 0x00) && (len == 9)) {
    // i.e. 25 00 0E 0F 00 31 35 36 89
    // device scan
    byte reply[] = { 0x11, 0xFF, 0x0A, 0x00, 0xDD, 0xA3 };
    GalaxyRs485.Send(reply, 6, false);
    Serial1.println("ethernet device poll");
    return;
  } else if ((cmd == 0x1a) && (len == 4)) {
    //Serial1.write("poll\n");
    // send error if we have one (sent to several polls in a row to ensure gets through)
    if (Error) {
      reply[3] = 0xE0; // error flag
      reply[8] = 0x8C; // update checksum
      Error--;
      //Serial1.println("ARC Comm fail");
    }
  } else if (cmd == 0x1c) {
    byte cmd2 = data[2];
    if (cmd2 == 0x10) {
      ParseIp(data, PrimaryIp);
      ParsePort(data, len, &PrimaryPort);
      //Serial1.write("PrimaryIp\n");
    } else if (cmd2 == 0x12) {
      // i.e. 25 1C 12 00 FD
      // unclear what this does, but occurs before an arc message send
      // then the following 1c commands are sia type blocks without the
      // 50/51/52 byte that is sent with monitor messages
    } else if ((cmd2 == 0x14) && (len == 6)) {
      Format = data[3];
      //Serial1.print("format: ");
      //DumpHex(data, len);
      //Serial1.write("Format\n");
    } else if ((cmd2 == 0x18) && (len == 5)) {
      //Serial1.write("Retries\n");
      Retries = data[3];
    } else if (cmd2 == 0x1b) {
      //Serial1.write("Gateway\n");
      //if (ParseIp(data, Gateway)) RestartNet();
    } else if (cmd2 == 0x1c) {
      //Serial1.write("Subnet\n");
      //if (ParseIp(data, Subnet)) RestartNet();
    } else if (cmd2 == 0x1a) {
      //Serial1.write("Ip\n");
      //if (ParseIp(data, Ip)) RestartNet();
    } else if ((cmd2 == 0x1d) && (len == 11)) {
      //Serial1.print("options: ");
      // todo: process more options
      Protocol = data[9];
    } else if (cmd2 == 0x1e) {
      //Serial1.write("SecondaryIp\n");
      ParseIp(data, SecondaryIp);
      ParsePort(data, len, &SecondaryPort);
    } else if ((cmd == 0x1c) && (cmd2 == 0x1f) && (len = 6)) {
      // todo: unknown command
    } else if (cmd2 == 0x20) {
      //Serial1.write("MonitorIp\n");
      ParseIp(data, MonitorIp);
      ParsePort(data, len, &MonitorPort);
    } else if (cmd2 == 0x50) {
      //Serial1.write("Report Account\n");
      data[len-1] = 0;
      strncpy(Report.Account, (char*)(data+3), 8);
    } else if (cmd2 == 0x51) {
      //Serial1.write("Report Message\n");
      data[len-1] = 0;
      strncpy(Report.Message, (char*)(data+3), SIA_LEN_MAX);
      // last part of the report, so prep for sending
      CreateReport();
    } else if (cmd2 == 0x52) {
      //Serial1.write("Report Header\n");
      data[len-1] = 0;
      strncpy(Report.Header, (char*)(data+3), SIA_LEN_MAX);
    } else {
      Serial1.print("unknown: ");
      GalaxyRs485.DumpHex(data, len);
    }
  } else {
    Serial1.print("unknown: ");
    GalaxyRs485.DumpHex(data, len);
  }
  
  GalaxyRs485.Send(reply, 9, false);
  
}

GalaxyEthernetClass GalaxyEthernet;
