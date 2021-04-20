#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>

//Button Class
class Button {
  private:
    byte pin;
    byte state;
    byte lastReading;
    byte attachedLed;
    bool ledState;
    unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 50;

  public:
    //Constructor
    Button(byte pin, byte attachedLed, bool ledState) {
      this->pin = pin;
      this->attachedLed = attachedLed;
      this->ledState = ledState;
      lastReading = HIGH; //For Input_Pullup
      init();
    }

    void init() {
      pinMode(pin, INPUT_PULLUP);
      pinMode(attachedLed, OUTPUT);
      digitalWrite(attachedLed, LOW);
      //update();
    }

    void update() {
      byte newReading = digitalRead(pin);
      if (newReading != lastReading) {
        lastDebounceTime = millis();
      }
      if ((millis() - lastDebounceTime) > debounceDelay) {
        if (newReading != state) {
          state = newReading;

          if (state == LOW) {
            ledState = !ledState;
          }
        }
      }
      digitalWrite(attachedLed, ledState);
      lastReading = newReading;
    }

    byte isPressed(){
      return ledState;
    }
};

const byte pipes[][6] = {"00001", "00002"};;
const int scrollDelay = 200;

//Push buttons
byte HERE_BTN_PIN = A2;
byte WAIT_BTN_PIN = A1;
byte NOT_HERE_BTN_PIN = A0;
//LED indicators
byte HERE_PIN = 2; //Green
byte WAIT_PIN = 3; //Yellow
byte NOT_HERE_PIN = 4; //Red

//States
bool hereLedState = LOW;
bool waitLedState = LOW;
bool notHereLedState = LOW;

//Flags
bool isWaitingForReply = false;

//Instances
Button hereBtn(HERE_BTN_PIN, HERE_PIN, hereLedState);
Button waitBtn(WAIT_BTN_PIN, WAIT_PIN, waitLedState);
Button notHereBtn(NOT_HERE_BTN_PIN, NOT_HERE_PIN, notHereLedState);

RF24 radio(9, 8); // CE, CSN
LiquidCrystal_I2C lcd(0x27, 16, 2);

void initialize() {
  //Serial
  Serial.begin(9600);
  //LCD
  lcd.init();
  lcd.backlight();
  //nRF24L01
  radio.begin();
  radio.openWritingPipe(pipes[1]); //Writes via 00002
  radio.openReadingPipe(1, pipes[0]); //reads via 00001
  radio.setPALevel(RF24_PA_MIN); //changeable : min = 250kbps
  radio.startListening(); //since receiving only for now

  showWelcomeMsg();
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
  str = "No notification..";
  lcd.clear();
  lcd.setCursor((16 - str.length()) / 2, 0);
  lcd.print(str);
}

void setup() {
  initialize();
}

void loop() {
  if (!isWaitingForReply) {
    
    if (radio.available()) {
      char text[32] = "";
      radio.read(&text, sizeof(text));
      lcd.clear();
      lcd.setCursor(0, 0);
      String noNotif = "No notification.";
      String msg = (String)text == "INVALID" ? noNotif : (String)text;
      Serial.println(text);
      lcd.print(msg);
      isWaitingForReply = true;
      radio.stopListening();
    }
  }

  else if (isWaitingForReply) {
    hereBtn.update();
    waitBtn.update();
    notHereBtn.update();
    
    sendReply();
  }
}


void sendReply() {
  String replyTxt = "This is a test reply...";
  int len = replyTxt.length() + 1;
  char reply[len];
  replyTxt.toCharArray(reply, len);
  //send
  if (!radio.write(&reply, sizeof(reply))) {
    Serial.println(F("Reply wasn't sent!"));
  }
  else {
    isWaitingForReply = false;
    radio.startListening();
  }
}
