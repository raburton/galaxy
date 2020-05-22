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


#include "GalaxyKeyPad.h"

#define CSUM 0xff

GalaxyKeyPadClass::GalaxyKeyPadClass() {
  for (int i = 0; i < 16; i++) Line1[i] = 0x20;
  for (int i = 0; i < 16; i++) Line2[i] = 0x20;
  Line1[16] = Line2[16] = 0;
}

bool GalaxyKeyPadClass::KeysEnqueue(byte key) {
  if (KeyCount == 10) return false;
  KeyQueue[(KeyCount+KeyPos)%10] = key;
  KeyCount++;
  return true;
}

byte GalaxyKeyPadClass::KeysPeek() {
  if (KeyCount == 0) return 0;
  return KeyQueue[KeyPos];
}

bool GalaxyKeyPadClass::KeysDequeue() {
  if (KeyCount == 0) return false;
  KeyPos++;
  KeyPos %= 10;
  KeyCount--;
  return true;
}

void GalaxyKeyPadClass::SendReplyWithKey() {
  //if ((KeyCount > 0) || Tamper) {
    //byte key = (KeyCount > 0 ? KeysPeek() : 0x7f) | (Tamper ? 0x40 : 0);
  if (KeyCount > 0) {
    byte key = KeysPeek();
    byte data[] = { 0x11, 0xF4, key, CSUM };
    GalaxyRs485.Send(data, 4, true);
  } else {
    byte data[] = { 0x11, 0xFE, 0xBA };
    GalaxyRs485.Send(data, 3, false);
  }
}

bool GalaxyKeyPadClass::MoveCursor(byte n) {
  // only pass +1 or -1
  // todo: wrapping? extended range?
  byte pos = (byte)(CursorPos + n);
  if ((CursorPos >= 0x00 && CursorPos <= 0x0f) ||
    (CursorPos >= 0x40 && CursorPos <= 0x4f)) {
    CursorPos = pos;
    return true;
  }
  return false;
}

void GalaxyKeyPadClass::PlaceCharacter(byte c) {
  // todo: extended range?
  if (CursorPos >= 0x00 && CursorPos <= 0x0f) {
    Line1[CursorPos] = c;
  } else if (CursorPos >= 0x40 && CursorPos <= 0x4f) {
    Line2[CursorPos - 0x40] = c;
  }
  MoveCursor(+1);
}

// separate function to a screen update command (due to its size)
void GalaxyKeyPadClass::UpdateScreen(byte *data, int len) {
  // todo: first byte
  for (int i = 3; i < len-1; i++) {
    switch (data[i]) {
      case 0x01:
        CursorPos = 0x00;
        break;
      case 0x02:
        CursorPos = 0x40;
        break;
      case 0x03:
        i++;
        if (i < len-1) CursorPos = data[i];
        break;
      case 0x04:
        ScrollPos--;
        break;
      case 0x05:
        ScrollPos++;
        break;
      case 0x11:
        // looks same as 0x06, drop through
      case 0x06:
        CursorType = 1;
        break;
      case 0x07:
        CursorType = 0;
        break;
      case 0x10:
        CursorType = 2;
        break;
      case 0x14:
        if (MoveCursor(-1)) PlaceCharacter(0x20);
        break;
      case 0x15:
        MoveCursor(-1);
        break;
      case 0x16:
        MoveCursor(+1);
        break;
      case 0x17:
        ScrollPos = 0;
        CursorPos = 0;
        for (int c = 0; c < 16; c++) Line1[i] = 0x20;
        for (int c = 0; c < 16; c++) Line2[i] = 0x20;
        break;
      case 0x18:
        Flash = true;
        break;
      case 0x19:
        Flash = false;
        break;
      default:
        PlaceCharacter(data[i]);
        break;
    }
  }
}

// process a command sent from the panel,
// sends replies via the global GalaxyRs485 object
// data: full rs485 message, inc. checksum
// len: length of data
void GalaxyKeyPadClass::ProcessCommand(byte *data, int len) {
  byte cmd = data[1];

  if (!GalaxyRs485.CheckSum(data, len, false)) {
    // todo: proper reply
    byte reply[] = { 0x11, 0xF2, 0xAE };
    GalaxyRs485.Send(reply, 3, false);
    return;
  }
  
  if ((cmd == 0x00) && (len == 4)) {
    // device scan
    byte reply[] = { 0x11, 0xFF, 0x08, 0x00, 0x64, 0x28 };
    GalaxyRs485.Send(reply, 6, false);
    Serial1.println("keypad device poll");
  } else if ((cmd == 0x07) && len >= 39) {
    // update screen
    if (ConfirmAlt != (data[2] & 0x02)) {
      ConfirmAlt = data[2] & 0x02;
      if (KeyCount > 0) KeysDequeue();
    }
    if (DisplayAlt != (data[2] & 0x80)) {
      DisplayAlt = (byte)(data[2] & 0x80);
      UpdateScreen(data, len);
      //Serial1.print("screen: '");
      //Serial1.print((char*)Line1);
      //Serial1.print("' / '");
      //Serial1.print((char*)Line2);
      //Serial1.println("'");
      ScreenUpdate = true;
    }
    SendReplyWithKey();
    //Serial1.write("screen\n");
  } else if ((cmd == 0x0B) && (len == 4)) {
    // acknowledge key (no screen update)
    if (ConfirmAlt != (data[2] & 0x02)) {
      ConfirmAlt = data[2] & 0x02;
      if (KeyCount > 0) KeysDequeue();
    }
    SendReplyWithKey();
    //Serial1.println("next key");
  } else if ((cmd == 0x0D) && (len == 4)) {
    // backlight
    if ((data[2] & 0x01) == 0x01) {
      if (!Backlight) {
        //Serial1.println("backlight: on");
        Backlight = true;
        ScreenUpdate = true;
      }
    } else {
      if (Backlight) {
        //Serial1.println("backlight: off");
        Backlight = false;
        ScreenUpdate = true;
      }
    }
    byte reply[] = { 0x11, 0xFE, 0xBA };
    GalaxyRs485.Send(reply, 3, false);
  } else if ((cmd == 0x0c) && len == 6) {
    byte reply[] = { 0x11, 0xFE, 0xBA };
    if ((data[2] != Beep) || (data[3] != BeepOn) || (data[4] != BeepOff)) {
      Beep = data[2];
      BeepOn = data[3];
      BeepOff = data[4];
      ScreenUpdate = true;
      //Serial1.print("beep ");
      //Serial1.print(Beep);
      //Serial1.print("/");
      //Serial1.print(BeepOn);
      //Serial1.print("/");
      //Serial1.println(BeepOff);
    }
    GalaxyRs485.Send(reply, 3, false);
  } else if ((cmd == 0x19) && (len == 4)) {
    // key poll
    SendReplyWithKey();
    Serial1.write("key poll\n");
  } else {
    byte reply[] = { 0x11, 0xFE, 0xBA };
    GalaxyRs485.Send(reply, 3, false);
    Serial1.print("unknown: ");
    GalaxyRs485.DumpHex(data, len);
  }
  
}


GalaxyKeyPadClass GalaxyKeyPad;
