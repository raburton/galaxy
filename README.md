# Overview
Honeywell Galaxy RS485 code for Arduino/STM32duino. Can be used to impersonate hardware devices connected to the Galaxy G2 (and possibly others), such as a keypad or network adapter. This allows you to interface your alarm with a PC or other devices.

## Details
This code is written for the stm32 "blue pill" connected to an RS485 adapter (will need to be a 3.3v model for connection to the stm32). 
You will need this Arduino core: https://github.com/stm32duino
The code is not a finished product but as I haven't had time to play with it for a little while, and several people have asked for it recently, I have decided to publish it as-is at the moment.

Libraries are included to:
 * interface with the Galaxy RS485 bus - to add your own devices (and used by the other libraries).
 * impersonate a keypad - this allows you to make a remote keypad or automate anything that you might normally be able to do with a real keypad (such as settting or unsetting the alarm).
 * impersonate a network adapter - this allows you to capture alarm reports for monitoring.

The two arduino projects included are:
 * ece01.ino - uses the libraries to create a galaxy <-> ethernet interface, so you can network enable your alarm. This isn't finished, but the basics are all there so it wouldn't take a lot to get it doing whatever you want it to. ece = even cheaper ethernet, a play on the lce-01 (low cost ethernet) module from http://www.selfmon.co.uk/apps/main/. You can make an ece for less than $10 with parts from AliExpress.
 * ece01-monitor.ino - a simple monitor application, you can use this to watch the RS485 flows to and from a specific device, spitting out just these on a second serial port. Great for examining the protocol of other devices you might want to us.
 
