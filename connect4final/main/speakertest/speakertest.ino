#include "pitches.h"
#define speaker A0
//#define button 3
//int buttonstate = 0;

void setup() {
  //pinMode(button, INPUT);
}

void loop() {
  tone(speaker,NOTE_A4,300);
  //buttonstate = digitalRead(button);
  //if (buttonstate == HIGH){
  //tone(speaker,NOTE_A4,300);
  //delay(200);
  //}
  delay(350);
}
