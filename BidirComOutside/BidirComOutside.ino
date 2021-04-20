#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define button A2

RF24 radio(9, 8); // CE, CSN
const byte addresses[][6] = {"00001", "00002"};

boolean buttonState = 0;

void setup() {
  pinMode(button, INPUT_PULLUP);
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(addresses[0]); // 00001
  radio.openReadingPipe(1, addresses[1]); // 00002
  radio.setPALevel(RF24_PA_MIN);
}
void loop() {
  delay(5);
  radio.startListening();
  if ( radio.available()) {
    while (radio.available()) { //send
      String test = "";
      radio.read(&test, sizeof(test));
      Serial.print("Received: ");
      Serial.println(test);
    }
  }
  delay(5);
  radio.stopListening();
  buttonState = digitalRead(button);
  radio.write(&buttonState, sizeof(buttonState));
}
