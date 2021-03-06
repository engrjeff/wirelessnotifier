#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>


RF24 radio(9, 8); // CE, CSN
const byte addresses[][6] = {"00001", "00002"};
boolean buttonState = 0;
void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(addresses[1]); // 00002
  radio.openReadingPipe(1, addresses[0]); // 00001
  radio.setPALevel(RF24_PA_MIN);
}
void loop() {
  delay(5);
  radio.stopListening();
  String name = "Engr. Jeff!";
  int len = name.length() + 1;
  char test[len];
  name.toCharArray(test, len);
  radio.write(&test, sizeof(test)); //send
  delay(5);
  radio.startListening();
  while (!radio.available());
  radio.read(&buttonState, sizeof(buttonState)); //receive
  if (buttonState == HIGH) {
    Serial.println("Button State: HIGH");
  }
  else {
    Serial.println("Button State: LOW");
  }
}
