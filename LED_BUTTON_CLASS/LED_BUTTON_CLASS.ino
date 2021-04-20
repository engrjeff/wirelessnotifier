class Indicator {
  private:
    byte pin;
    bool state;
  public:
    //Constructor
    Indicator(byte pin) {
      this->pin = pin;
      init();
    }
    //Methods
    void init() {
      pinMode(pin, OUTPUT);
      off();
    }
    void toggleState() {
      state = !state;
    }
    void updateState() {
      digitalWrite(pin, state);
    }
    void on() {
      digitalWrite(pin, HIGH);
    }
    void off() {
      digitalWrite(pin, LOW);
    }
};


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

    byte getState() {
      //update();
      return state;
    }

    bool isPressed() {
      return (getState() == LOW);
    }
};


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

//Instances
Button hereBtn(HERE_BTN_PIN, HERE_PIN, hereLedState);
Button waitBtn(WAIT_BTN_PIN, WAIT_PIN, waitLedState);
Button notHereBtn(NOT_HERE_BTN_PIN, NOT_HERE_PIN, notHereLedState);

void setup() {

}

void loop() {
  hereBtn.update();
  waitBtn.update();
  notHereBtn.update();
}
