#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {A0, A1, A2, 7};
byte colPins[COLS] = {6, 5, 4};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
LiquidCrystal_I2C lcd(0x27, 16, 2);
RF24 radio(9, 8); // CE, CSN

const byte pipes[][6] = {"00001", "00002"};
char targetKey;
const int scrollDelay = 200;
bool hasKey = false;
String personnel = "INVALID"; //default
bool isWaitingForReply = false;

void initialize() {
  //Serial
  Serial.begin(9600);
  //LCD
  lcd.init();
  lcd.backlight();
  //nRF24L01
  radio.begin();
  radio.openWritingPipe(pipes[0]); //Writes via 00001
  radio.openReadingPipe(1, pipes[1]); //Read via 00002
  radio.setPALevel(RF24_PA_MIN); //changeable : min = 250kbps
  radio.stopListening(); //since transmitting only for now

  showWelcomeMsg();
}

String getTargetByKey(char key) {
  targetKey = key == '*' ? 'x' : key;
  String targetName = "INVALID";
  if (targetKey && targetKey != 'x') {
    switch (targetKey) {
      case '1' :
        targetName = "Engr. Obien"; break;
      case '2' :
        targetName = "Engr. de Leon"; break;
      case '3' :
        targetName = "Engr. Mendoza"; break;
      case '4' :
        targetName = "Engr. Landerito"; break;
      case '5' :
        targetName = "Engr. Pedrocillo"; break;
      case '6' :
        targetName = "Engr. Marcos"; break;
      case '7' :
        targetName = "Engr. Beringuel"; break;
      case '8' :
        targetName = "Engr. Barreto"; break;
      case '9' :
        targetName = "Engr. Fernando"; break;
      default: targetName = "INVALID"; break;
    }
  }
  return targetName;
}

void evaluateKey() {
  //Get keypad input
  char key = keypad.getKey();
  if (!key) {
    return;
  }
  showKeyDetails(key);
}

void showKeyDetails(char key) {
  if (!isWaitingForReply) {
    if (key != '*' && key != '#' && !hasKey) {
      personnel = getTargetByKey(key);
      displayPersonnel(key, personnel);
      if (!personnel.equals("INVALID"))
      {
        hasKey = true;
      }
    }
    else if (key == '*' && hasKey) {
      Serial.println("OK is pressed");
      hasKey = false;
      transmitName(personnel);
      showWait("Notif sent!..");
      receiveReply();
    }
    else if (key == '#' && hasKey) {
      Serial.println("Clear is pressed");
      hasKey = false;
      resetDisplay("Cancelled..");
    }
    else if (key == '#' && !hasKey) {
      hasKey = false;
      transmitName(personnel);
      resetDisplay("Cancelled..");
    }

    Serial.println(personnel);
  }

  //  else if (isWaitingForReply) {
  //    receiveReply();
  //  }
}

void displayPersonnel(char key, String name) {
  byte len = name.length();
  lcd.clear();
  String keyTxt = "KEY: " + (String)key;
  lcd.setCursor(0, 0);
  lcd.print(keyTxt);
  lcd.setCursor(7, 0);
  lcd.print("Confirm?");
  lcd.setCursor((16 - len) / 2, 1);
  lcd.print(name);
}

void receiveReply() {

  if (radio.available()) {
    char reply[32] = "";
    radio.read(&reply, sizeof(reply));
    Serial.println(reply);
    isWaitingForReply = false;
    radio.stopListening();
    hasKey = false;
  }
}
void transmitName(String personnel) {
  //  if (personnel.equals("INVALID")) {
  //    return;
  //  }
  int len  = personnel.length() + 1;
  char name[len];
  personnel.toCharArray(name, len);
  if (!radio.write(&name, sizeof(name))) {
    Serial.println(F("Not Sent"));
  }
  else {
    radio.startListening();
  }
}

void showWelcomeMsg() {
  lcd.clear();
  String str = "Wireless Personnel Notifier";
  lcd.setCursor(0, 0);
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
  delay(1000);
  str = "Waiting for KEY";
  lcd.clear();
  lcd.setCursor((16 - str.length()) / 2, 0);
  lcd.print(str);
}

void showWait(String txt) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(txt);
  delay(scrollDelay * 2);
  String str = "Waiting for ";
  lcd.clear();
  lcd.setCursor((16 - str.length()) / 2, 0);
  lcd.print(str);
  String str2 = "Reply ";
  lcd.setCursor((16 - str2.length()) / 2, 1);
  lcd.print(str2);

  personnel = "INVALID";
}
void resetDisplay(String txt) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(txt);
  delay(scrollDelay * 2);
  String str = "Waiting for KEY";
  lcd.clear();
  lcd.setCursor((16 - str.length()) / 2, 0);
  lcd.print(str);

  personnel = "INVALID";
  isWaitingForReply = false;
}

void setup() {
  initialize();
}

void loop() {
  evaluateKey();
}
