/*
---------------------------------------------------------------------------------------
This file is part of the OMX repository
Devon Cowan, CSHL 2024
---------------------------------------------------------------------------------------
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3.

This program is distributed  WITHOUT ANY WARRANTY and without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
---------------------------------------------------------------------------------------
This sketch is for running a teensy(4.1)-based driver board with 176 outputs. It takes 
serial input from LabView (or wherever).
---------------------------------------------------------------------------------------
*/

#include <SPI.h>

//IC addressing
int ChipSelect[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 24, 18, 17, 16, 15, 14, 41, 23, 22, 21, 20, 19};
int NumCSPins = 22; //There are 22 Driver ICs

//For SPI later
int CSNum = 0;
int ChannelNum = 0;
int Disconnect = 0;

//Serial input
char ICChar[16];
char ChannelChar[16];
char StartMarker = '<';
char EndMarker = '>';
char ChanMarker = '^';
char NewByte;

boolean ICReceiving = false;
boolean ChanReceiving = false;
boolean NewData = false;

byte ByteCount = 0;


void setup() {

  //Assign chip select pins to output and set to HIGH (low selects IC)
  for (int i = 0; i < NumCSPins; i++){
    pinMode(ChipSelect[i], OUTPUT);
    digitalWrite(ChipSelect[i], HIGH);
  }

  //Setting disconnect pin to low turns off all drivers and clears registers, must be high for SPI communication
  pinMode(Disconnect, OUTPUT);
  digitalWrite(Disconnect, HIGH);

  Serial.begin(115200);
  SPI.begin();

}

void loop() {
  RecSerialData(); //Bring in serial data
  ControlValves(); //Turn on/off valves
}

void RecSerialData() {

  //Uses markers to determine when to start and stop pulling in serial data (also to differentiate the driver and channel numbers)
  while (Serial.available() > 0 && NewData == false) {
    
    NewByte = Serial.read();

    if (ICReceiving == true) {
      if (NewByte == ChanMarker) {
        ICChar[ByteCount] = '\0';
        ICReceiving = false;
        ChanReceiving = true;
        ByteCount = 0;
      }
      else {
        ICChar[ByteCount] = NewByte;
        ByteCount++;
      }
    }
    else if (ChanReceiving == true) {
      if (NewByte == EndMarker) {
        ChannelChar[ByteCount] = '\0';
        ChanReceiving = false;
        ByteCount = 0;
        NewData = true;
      }
      else {
        ChannelChar[ByteCount] = NewByte;
        ByteCount++;
      }
    }          
    else if (NewByte == StartMarker) {
      ICReceiving = true;
    }
  }
}

void ControlValves() {
  //Convert serial chars to numbers for setting driver states
  if (NewData == true) {

    CSNum = (atoi(ICChar)-1);
    ChannelNum = atoi(ChannelChar);

    //Sending a driver number greater than exists on the board will turn off everything
    if (CSNum > 22) {
      digitalWrite(Disconnect, LOW);
      delay(10);
      digitalWrite(Disconnect, HIGH);
    }
    //Send SPI data
    else {
      SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
      digitalWrite(ChipSelect[CSNum], LOW);
      SPI.transfer(ChannelNum);
      digitalWrite(ChipSelect[CSNum], HIGH);
      SPI.endTransaction();
    }
    NewData = false;
  }
}