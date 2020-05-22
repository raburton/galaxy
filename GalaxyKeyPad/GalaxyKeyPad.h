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


#ifndef galaxykeypad_h
#define galaxykeypad_h

#include <Arduino.h>

#include <GalaxyRs485.h>

class GalaxyKeyPadClass {

private:

  byte KeyQueue[10];
  byte KeyCount = 0;
  byte KeyPos = 0;
  byte ConfirmAlt = 0x00; // alternates between 0x00 and 0x02
  byte DisplayAlt = 0x80; // alternates between 0x80 and 0x00
  
  bool KeysEnqueue(byte k);
  byte KeysPeek(void);
  bool KeysDequeue(void);
  void UpdateScreen(byte *data, int len);
  void SendReplyWithKey(void);
  bool MoveCursor(byte n);
  void PlaceCharacter(byte c);

public:

  bool ScreenUpdate = false;

  bool Backlight = false; // incoming
  //bool Tamper = false;    // outgoing

  byte Line1[17];
  byte Line2[17];
  byte CursorPos = 0;
  byte CursorType = 0;
  byte Led = 0;
  bool Flash = false;
  byte Beep = 0;
  byte BeepOn = 0;
  byte BeepOff = 0;
  byte ScrollPos = 0;
  
  GalaxyKeyPadClass();
  void ProcessCommand(byte *data, int len);

};

extern GalaxyKeyPadClass GalaxyKeyPad;

#endif
