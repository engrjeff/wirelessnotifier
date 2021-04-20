/*
   Project : Wireless Personnel Notifier
   Description : Provides wireless notification about
                  the presence of a room personnel (i.e Professor)
                  whom is identified using keypad keys
   Author : Jeffrey E. Segovia, ECE, ECT
   Date : October 2019
   Code : This code is for the Indoor Unit

*/


//Required Libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>


//Jeff's Button Class
class Button {
    //Private Members
  private:
    byte pin;
    byte state;
    byte lastReading;
    unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 50;
    byte savedState = HIGH;

    //Public Members
  public:
    //Constructor
    Button(byte pin) {
      this->pin = pin;
      lastReading = HIGH; //For Input_Pullup
      init();
    }

    void init() {
      pinMode(pin, INPUT_PULLUP);
    }

    void setDefault() {
      state = HIGH;
      lastReading = HIGH;
    }

    //Debouncer
    void update() {
      byte newReading = digitalRead(pin);
      if (newReading != lastReading) {
        lastDebounceTime = millis();
      }
      if ((millis() - lastDebounceTime) > debounceDelay) {
        if (newReading != state) {
          state = newReading;

          if (state == LOW) {
            savedState = LOW;
          }
        }
      }
      lastReading = newReading;
    }

    byte getState() {
      update();
      return state;
    }
};

//Comms Pipes Addresses
const byte pipes[][6] = {"00001", "00002"};

//For LCD Scrolling
const int scrollDelay = 200;

//Flags
bool isWaiting = true;

//Push buttons pin definitions
byte HERE_BTN_PIN = A2;
byte WAIT_BTN_PIN = A1;
byte NOT_HERE_BTN_PIN = A0;
//LED indicators pin definitions
byte HERE_LED = 2; //Green
byte WAIT_LED = 3; //Yellow
byte NOT_HERE_LED = 4; //Red

//LED Indicators' States
bool hereLedState = LOW;
bool waitLedState = LOW;
bool notHereLedState = LOW;

//On-receieve indicator LED
const byte onReceiveLed = 6; //Pin6
//Timer
unsigned long elapsedTime = 0;
//unsigned long interval = 20000; //20 sec (for testing)
unsigned long interval = 290000; //4 minutes 50 seconds

//IR Remote
const byte REMOTE_PIN = 5;

//Remote Codes
enum IRCode {
  HERE = 16738455,
  NOT_HERE = 16750695,
  WAIT = 16756815
};

enum IRCode codes;

//Struct
struct Personnel {
  int id;
  char name[29];
};

struct Status {
  int key;
  char status[29];
};

Personnel prof;
Status status;

//Instances
RF24 radio(9, 8); // CE, CSN
LiquidCrystal_I2C lcd(0x27, 16, 2);
IRrecv ir(REMOTE_PIN);
decode_results results;

//Buttons
Button hereBtn(HERE_BTN_PIN);
Button notHereBtn(NOT_HERE_BTN_PIN);
Button waitBtn(WAIT_BTN_PIN);

void setup() {
  initialize();
}

void loop() {
  runSystem();
}


void blinkOnReceive() {
  for (int i = 0; i < 20; i++) {
    digitalWrite(onReceiveLed, HIGH);
    delay(100);
    digitalWrite(onReceiveLed, LOW);
    delay(100);
  }
}

//This function encapsulates the main system
void runSystem() {
  if (isWaiting) {
    radio.startListening();
    Serial.println(F("Receive Mode"));
    elapsedTime = millis();
    receiveData();
  }
  else {
    radio.stopListening();
    Serial.println(F("Transmit Mode"));
    runTimer();
    //transmitData();
    //transmit data
    if (!transmitData()) return;
  }

}

void runTimer() {
  unsigned long currentMillis = millis();
  if ((currentMillis - elapsedTime) > interval) {
    Serial.print(F("Elapsed time: "));
    Serial.println((millis() - elapsedTime));
    isWaiting = true;
    elapsedTime = 0;
    allOff();
  }
}

bool trackButtons() {

  bool s1 = hereBtn.getState();
  bool s2 = notHereBtn.getState();
  bool s3 = waitBtn.getState();

  String state;

  if (!s1) {
    hereLedState = HIGH;
    waitLedState = LOW;
    notHereLedState = LOW;
    updateLedState();
    state = "Here";
  }
  if (!s2) {
    hereLedState = LOW;
    waitLedState = LOW;
    notHereLedState = HIGH;
    updateLedState();
    state = "Not Here";
  }
  if (!s3) {
    hereLedState = LOW;
    waitLedState = HIGH;
    notHereLedState = LOW;
    updateLedState();
    state = "Wait";
  }

  if (s1 && s2 && s3) return;

  //Prepare status to be sent
  status.key = 0;
  state.toCharArray(status.status, state.length() + 1);

  //Transmit status
  if (!radio.write(&status, sizeof(status))) {
    Serial.println(F("Transmission failed!"));
    allOff();
    showError("Comms Error");
    //delay(100);
    //showReceivedData();
    return false;
  }
  Serial.print(F("Sent Status: "));
  Serial.print(F("Key: "));
  Serial.println(status.key);
  Serial.print(F("Status: "));
  Serial.println(status.status);
  Serial.println(F("Success!"));
  isWaiting = true;
  hereBtn.setDefault();
  notHereBtn.setDefault();
  waitBtn.setDefault();
  return true;
}

void updateLedState() {
  digitalWrite(HERE_LED, hereLedState);
  digitalWrite(WAIT_LED, waitLedState);
  digitalWrite(NOT_HERE_LED, notHereLedState);
}

bool receiveData() {
  if (radio.available()) {
    radio.read(&prof, sizeof(prof));

    if (prof.id == 0) {
      showDefault();
      isWaiting = true;
      Serial.println(F("Cancelled!"));
      allOff();
    }
    else {
      blinkOnReceive();
      showReceivedData();
      isWaiting = false;
      Serial.println(F("Received!"));
      Serial.println(prof.id);
      Serial.println(prof.name);
      
      allOff();

    }
  }
}

void allOff() {
  hereLedState = LOW;
  waitLedState = LOW;
  notHereLedState = LOW;
  updateLedState();
}

bool transmitData() {

  //IR Remote
  if (ir.decode(&results)) {
    String state;
    long int IR_code = results.value;
    Serial.print(F("Received Code: "));
    Serial.println(IR_code);

    switch (IR_code) {
      case HERE:
        Serial.println(IR_code);
        hereLedState = HIGH;
        waitLedState = LOW;
        notHereLedState = LOW;
        updateLedState();
        state = "Here";
        break;
      case NOT_HERE:
        Serial.println(IR_code);
        hereLedState = LOW;
        waitLedState = LOW;
        notHereLedState = HIGH;
        updateLedState();
        state = "Not here";
        break;
      case WAIT:
        Serial.println(IR_code);
        hereLedState = LOW;
        waitLedState = HIGH;
        notHereLedState = LOW;
        updateLedState();
        state = "Wait";
        break;
      default:
        state = "INVALID"; break;
    }
    ir.resume();
    if (state == "INVALID") return;

    status.key = 0;
    state.toCharArray(status.status, state.length() + 1);
    if (!radio.write(&status, sizeof(status))) {
      Serial.println(F("Transmission failed!"));
      allOff();
      showError("Comms Error");
      if (!jeffDelay(1000)) return;
      return false;
    }
    Serial.print(F("Sent Status: "));
    Serial.print(F("Key: "));
    Serial.println(status.key);
    Serial.print(F("Status: "));
    Serial.println(status.status);
    Serial.println(F("Success!"));
    isWaiting = true;
    return true;
  }

  else {
    if (!trackButtons()) return false;
    return true;
  }
}
void showReceivedData() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Notif for: ");
  lcd.setCursor(0, 1);
  lcd.print(prof.name);
}

void showDefault() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("No notification..");
}

void showError(String error) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(error);
}
void initialize() {
  //Serial
  Serial.begin(9600);
  //IO's
  pinMode(HERE_LED, OUTPUT);
  pinMode(WAIT_LED, OUTPUT);
  pinMode(NOT_HERE_LED, OUTPUT);

  digitalWrite(HERE_LED, LOW);
  digitalWrite(WAIT_LED, LOW);
  digitalWrite(NOT_HERE_LED, LOW);

  //onReceiveLed
  pinMode(onReceiveLed, OUTPUT);
  digitalWrite(onReceiveLed, LOW);
  //LCD
  lcd.init();
  lcd.backlight();
  //nRF24L01
  radio.begin();
  radio.openWritingPipe(pipes[1]); //Writes via 00002
  radio.openReadingPipe(1, pipes[0]); //reads via 00001
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);

  //IR Remote
  ir.enableIRIn();

  showWelcomeMsg();
}
bool jeffDelay(unsigned long interval) {
  unsigned long now = millis();
  while (!((millis() - now) > interval));
  return true;
}
void showWelcomeMsg() {
  lcd.clear();
  String str = "Wireless Personnel Notifier";
  lcd.setCursor(0, 0);
  Serial.println(str);
  lcd.print(str);
  //Scroll
  delay(scrollDelay);
  int strLen = str.length();
  for (byte i = 0; i < strLen; i++) {
    lcd.scrollDisplayLeft();
    delay(scrollDelay);
  }
  str = "Welcome!";
  lcd.clear();
  lcd.setCursor((16 - str.length()) / 2, 0);
  lcd.print(str);
  str = "Initializing...";
  lcd.clear();
  lcd.setCursor((16 - str.length()) / 2, 1);
  lcd.print(str);
  Serial.println(str);
  delay(1000);
  str = "No notification..";
  lcd.clear();
  lcd.setCursor((16 - str.length()) / 2, 0);
  lcd.print(str);
  Serial.println(str);
}
