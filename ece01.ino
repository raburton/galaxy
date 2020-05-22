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


#define NETRESET PA0
#define READBUFFERLEN 100

#include <Arduino.h>
#include <Ethernet_STM.h>

#include <GalaxyRs485.h>
#include <GalaxyKeyPad.h>
#include <GalaxyEthernet.h>

bool net = false;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0D};
EthernetServer server(23);
EthernetClient client;
EthernetClient tcp;
EthernetUDP udp;

byte Buffer[READBUFFERLEN];

void SendReport() {
  if (GalaxyEthernet.Protocol == GALAXY_PROTOCOL_UDP) {
    udp.beginPacket(GalaxyEthernet.MonitorIp, GalaxyEthernet.MonitorPort);
    udp.write(GalaxyEthernet.Report.Raw, GalaxyEthernet.Report.RawLen);
    udp.endPacket();
  } else {
    if (tcp.connected()) tcp.stop();
    if (tcp.connect(GalaxyEthernet.MonitorIp, GalaxyEthernet.MonitorPort)) {
      tcp.write(GalaxyEthernet.Report.Raw, GalaxyEthernet.Report.RawLen);
    }
  }
  GalaxyEthernet.Report.LastSend = millis();
}

void ServiceRS485() {
  size_t len = 0;

  if (GalaxyRs485.Available() > 0) {
    len = GalaxyRs485.Read(Buffer, READBUFFERLEN);
  }
  if ((len > 2)) {
    if ((Buffer[0] == 0x10)) {
      GalaxyKeyPad.ProcessCommand(Buffer, len);
    } else if (Buffer[0] == 0x25) {
      GalaxyEthernet.ProcessCommand(Buffer, len);
    }
  }

}

void ConfigNet() {

  //digitalWrite(NETRESET, 1);
  //delay(100);
  if (client && client.connected()) client.stop();

  Ethernet.begin(mac, GalaxyEthernet.Ip, GalaxyEthernet.Gateway, GalaxyEthernet.Subnet);
  server.begin();
  udp.begin(10000);
  
  tcp.setTimeout(500);
  client.setTimeout(500);
  
  net = true;
}

void ServiceNet() {
  if (!client || !client.connected()) {
    client = server.available();
  }
  if (client) {
    byte data[10];
    byte i = 0;
    while (client.available() && (i < 10)) {
      data[i] = client.read();
      Serial1.print(data[i], HEX);
      i++;
    }
    if (i > 3) {
      if ((data[0] == 0x12) && (data[1] == 0x13)) {
        Serial1.println("replying");
        client.write(0x99);
      }
    } else {
      if (GalaxyKeyPad.ScreenUpdate) {
        client.print("screen: '");
        client.print((char*)GalaxyKeyPad.Line1);
        client.print("' / '");
        client.print((char*)GalaxyKeyPad.Line2);
        client.print("', bl: ");
        client.print(GalaxyKeyPad.Backlight);
        client.print(", bp: ");
        client.print(GalaxyKeyPad.Beep);
        client.print("/");
        client.print(GalaxyKeyPad.BeepOn);
        client.print("/");
        client.println(GalaxyKeyPad.BeepOff);
        GalaxyKeyPad.ScreenUpdate = false;
      }
    }
  }

  if (udp.parsePacket()) {
    IPAddress ip = udp.remoteIP();
    IPAddress ip2(GalaxyEthernet.MonitorIp);
    if (ip == ip2) {
      GalaxyEthernet.Report.Retries = 0;
      GalaxyEthernet.Report.Unconfirmed = false;
    }
    // read reply TODO: validate
    while (udp.available()) udp.read(Buffer, READBUFFERLEN);
    // allow main loop to run again before any more network processing
    return;
  }

  if (tcp.available()){
    int i;
    for (i = 0; (i < READBUFFERLEN) && tcp.available(); i++) {
      // read reply TODO: validate
      Buffer[i] = tcp.read();
    }
    // stop and dispose of any remaining data
    tcp.stop();
    while (tcp.available()) tcp.read();
    GalaxyEthernet.Report.Unconfirmed = false;
    // allow main loop to run again before any more network processing
    return;
  }

  if (GalaxyEthernet.Report.Unconfirmed && ((millis() - GalaxyEthernet.Report.LastSend) > 200)) {
    if (GalaxyEthernet.Report.Retries) {
      SendReport();
      GalaxyEthernet.Report.Retries--;
    } else {
      GalaxyEthernet.Report.Unconfirmed = false;
      GalaxyEthernet.Error = 5;
    }
  }
}

void setup() {

  pinMode(NETRESET, OUTPUT);
  digitalWrite(NETRESET, 1);

  GalaxyRs485.Begin(&Serial2, PA1);

  //pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, 0);
  
  Serial1.begin(115200);
}

void loop() {
  ServiceRS485();
  if (net) ServiceNet();
  else if (GalaxyEthernet.Ip[0] && GalaxyEthernet.Subnet[0] && GalaxyEthernet.Gateway[0]) ConfigNet();
}
