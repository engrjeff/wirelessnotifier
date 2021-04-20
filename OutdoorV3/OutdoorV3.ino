/*
 * Project : Wireless Personnel Notifier 
 * Description : Provides wireless notification about
 *                the presence of a room personnel (i.e Professor)
 *                whom is identified using keypad keys
 * Author : Jeffrey E. Segovia, ECE, ECT               
 * Date : October 2019
 * Code : This code is for the Outdoor Unit
 * 
 */


//Required Libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

//Keypad-related
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

//Class Instances
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
LiquidCrystal_I2C lcd(0x27, 16, 2);
RF24 radio(9, 8); // CE, CSN

//Comms Pipe Addresses
const byte pipes[][6] = {"00001", "00002"};

//The personnel key
char targetKey;

//For LCD Scrolling (on setup)
const int scrollDelay = 200;

//timer
unsigned long elapsedTime = 0;
//unsigned long interval = 30000; //30 sec for testing only
unsigned long interval = 300000; //5 min

//Flags
bool hasKey = false;
bool isConfirmed = false;
bool isCancelled = false;
bool isWaitingForConfirm = false;
bool isWaitingForReply = false;

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

void initialize() {
  //Serial Comms
  Serial.begin(9600);
  //LCD
  lcd.init();
  lcd.backlight();
  //nRF24L01
  radio.begin();
  radio.setRetries(5, 15);
  radio.openWritingPipe(pipes[0]); //Writes via 00001
  radio.openReadingPipe(1, pipes[1]); //Read via 00002
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);

  //Show Welcome Message
  showWelcomeMsg();
}

//This function encapsulates the main system
void runSystem() {
  if (!isWaitingForReply) {
    elapsedTime = millis();
    evaluateKey();
  }
  else {
    unsigned long currentMillis = millis();
    //Serial.println((currentMillis - elapsedTime));
    if ((currentMillis - elapsedTime) > interval) {
      Serial.print(F("Elapsed time: "));
      Serial.println((millis() - elapsedTime));
      elapsedTime = currentMillis;
      //Print to LCD
      showCancelledMsg("Time out..");
      //Switch radio mode
      prof.id = 0;
      String n = "INVALID";
      n.toCharArray(prof.name, n.length() + 1);
      radio.stopListening();
      transmitData();
      isWaitingForReply = false;
    }
    else {
      Serial.println("Receiving...");
      receiveData();
    }
  }
}


void evaluateKey() {
  char key = keypad.getKey();
  if (!key || key == '0') return;

  switch (key) {
    case '*' :
      isConfirmed = true;
      isCancelled = false;
      hasKey = false;
      break;
    case '#' :
      isConfirmed = false;
      isCancelled = true;
      hasKey = false;
      isWaitingForReply = false;
      break;
    default:
      isConfirmed = false;
      isCancelled = false;
      hasKey = true;
      isWaitingForConfirm = true;
      break;
  }
  //Show flags' status
  Serial.print("Confirm: ");
  Serial.println(isConfirmed);
  Serial.print("Cancelled: ");
  Serial.println(isCancelled);
  Serial.print("HasKey: ");
  Serial.println(hasKey);

  Serial.print("Waiting for Reply: ");
  Serial.println(isWaitingForReply);

  //Assign value to Personnel members
  if (hasKey) {
    prof.id = key - '0';
    String name = getTargetByKey(key);
    name.toCharArray(prof.name, name.length() + 1);

    Serial.println(prof.id);
    Serial.println(prof.name);

    //Print to LCD
    showProf(key, prof.name);

    //Switch radio mode
    radio.stopListening();
  }

  if (isConfirmed && isWaitingForConfirm) {
    Serial.println(F("Confirmed!"));
    Serial.println(prof.id);
    Serial.println(prof.name);
    isWaitingForConfirm = false;
    //Print to LCD
    showConfirmedMsg("Confirmed", "Sending notif...");

    //transmit data
    if (!transmitData()) return;

    //add delay
    if (!jeffDelay(1000)) return;

    //Print to LCD
    showConfirmedMsg("Notif sent!", "");

    //add delay
    if (!jeffDelay(1000)) return;
    //Print to LCD
    showConfirmedMsg("Waiting for", "reply");

    //Switch radio mode
    radio.startListening();

    isWaitingForReply = true;
  }

  if (isCancelled) {
    Serial.println(F("Cancelled!"));
    prof.id = 0;
    String n = "INVALID";
    n.toCharArray(prof.name, n.length() + 1);

    Serial.println(prof.id);
    Serial.println(prof.name);
    isWaitingForConfirm = false;
    //Print to LCD
    showCancelledMsg("Cancelled..");
    //Switch radio mode
    radio.stopListening();
    transmitData();
  }
}
bool receiveData() {
  if (radio.available()) {
    radio.read(&status, sizeof(status));
    Serial.print(F("Recieved Status: "));
    Serial.print(F("Key: "));
    Serial.println(status.key);
    Serial.print(F("Status: "));
    Serial.println(status.status);

    //Print to LCD
    showStatusReply();

    isWaitingForReply = false;
  }
}
bool transmitData() {
  if (!radio.write(&prof, sizeof(prof))) {
    Serial.println(F("Transmission failed!"));
    showCancelledMsg("Comms Error...");
    return false;
  }
  Serial.println(F("Success!"));
  return true;
}

String getTargetByKey(char key) {
  targetKey = key == '*' ? 'x' : key;
  String targetName = "INVALID";
  if (targetKey && targetKey != 'x') {
    switch (targetKey) {
      case '1' :
        targetName = "Engr. Obien"; break;
      case '2' :
        targetName = "Engr. Antipolo"; break;
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

void showStatusReply() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(prof.name);
  lcd.setCursor(0, 1);
  String str1 = "Reply: " + (String)status.status;
  lcd.print(str1);
}

void showProf(char key, String name) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(name);
  lcd.setCursor(0, 1);
  String str1 = "Key: " + (String)key + " Confirm? ";
  lcd.print(str1);
}

void showConfirmedMsg(String a, String b) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(a);
  lcd.setCursor(0, 1);
  lcd.print(b);
}

void showCancelledMsg(String a) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(a);

  if (!jeffDelay(2000)) return;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for KEY");
}

bool jeffDelay(unsigned long interval) {
  unsigned long now = millis();
  while (!((millis() - now) > interval));
  return true;
}
void setup() {
  initialize();
}

void loop() {
  runSystem();
}



void showWelcomeMsg() {
  lcd.clear();
  String str = "Wireless Personnel Notifier";
  lcd.setCursor(0, 0);
  lcd.print(str);
  Serial.println(str);
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
  str = "Waiting for KEY";
  lcd.clear();
  lcd.setCursor((16 - str.length()) / 2, 0);
  lcd.print(str);
  Serial.println(str);
}
