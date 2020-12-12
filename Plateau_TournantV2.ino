#include <Stepper.h>

#define DAQUIN 1
#define RAMEAU 2
#define BIBER 3
#define COUPERIN 4

const int chanPin[7] = {0,1,2,3,4,5,6};
const int lightPin = 9;
const int redLedPin = 8;
const int greenLedPin = 7;

const int stepsRound = 2038; // pretended 2048...but 2038 is more realistic
const long stepsReglage = 4;
const long steps_6_Round = stepsRound/6; // /!\ be carefull, due to integer conversion, steps 6 times the motor from this value result in an offset of 4 steps wrt a complete round!
const long slowMotorSpeed = 50; // step / s
const long baseMotorSpeed = 100; // step / s
//const long baseMotorSpeed = 150; // step / s

const int LOW_ENLIGHTMENT = 100;
const int MAX_ENLIGHTMENT = 255;

const int SHORT_CLIC = 10;
const int LONG_CLIC = 2000;

//const int DELAY_BEFORE_BEGIN = 5; // *1.5 secondes
long DELAI_BIBER[8] = {128, 162, 64, 51, 37, 44, 96, 153};
//long DELAI_BIBER[8] = {5, 5, 5, 5, 5, 5, 5, 15};
long DELAI_COUPERIN[6] = {0,15,15,15,15,120};
//long DELAI_COUPERIN[6] = {0,5,5,5,5,15};

bool chan[7] = {false,false,false,false,false,false,false};
bool old_chan[7] = {false,false,false,false,false,false,false};
bool chanUP[7] = {false,false,false,false,false,false,false};
bool chanDOWN[7] = {false,false,false,false,false,false,false};
unsigned long chanT0[7] = {0,0,0,0,0,0,0};
unsigned long chanTime[7] = {0,0,0,0,0,0,0};

int i;
int counter = 0;
bool flag = false;
bool light = false;
unsigned long currentTime = 0;
unsigned long previousTime = 0;

int state = 0;
unsigned int enlightment = 0;
int stepsToMove = 0;

Stepper motor(100, 10, 12, 11, 13);

bool _blink(unsigned int curTime, int curState) {
  while (curState>0) {
    unsigned int curStep = (--curState)*250;
    if (curTime >= curStep and curTime < curStep+50) {
      return HIGH; 
    }
  }
  return LOW;
}

void fadeIN(unsigned int goal) {
  // gently fade IN the light (for 0 to MAX_ENLIGHTMENT, the fade IN will last 5.1 seconds)
  while (enlightment < goal) {
    analogWrite(lightPin,++enlightment);
    delay(20);
  }
}
void fadeOUT() {
  // gently fade OUT the light
  while (enlightment > 0) {
    analogWrite(lightPin,--enlightment);
    delay(20);
  }
}

void setup() {    
  motor.setSpeed(baseMotorSpeed);

  pinMode(chanPin[5],INPUT_PULLUP);
  for (i=1;i<5;i++) {
    pinMode(chanPin[i],INPUT);
  }
  pinMode(chanPin[5],INPUT_PULLUP);
  pinMode(chanPin[6],INPUT_PULLUP);
/*  for (i=0;i<7;i++) {
    pinMode(chanPin[i],INPUT_PULLUP);
  }*/
  pinMode(lightPin,OUTPUT);
  pinMode(redLedPin,OUTPUT);
  pinMode(greenLedPin,OUTPUT);

  light = false;
  analogWrite(lightPin, light*LOW_ENLIGHTMENT);

  state = 0; // initial state
}

void loop() {
  
  currentTime = millis();

  // chanels reading
  for (i=0;i<7;i++) {
    if (i>0 and i<5) {
      chan[i] = digitalRead(chanPin[i]);
    }
    else {
      chan[i] = not digitalRead(chanPin[i]);
    }
    chanUP[i] = not old_chan[i] and chan[i];
    chanDOWN[i] = old_chan[i] and not chan[i];
    old_chan[i] = chan[i];
    chanTime[i] = 0;
    if (chanUP[i]) {
      chanT0[i] = currentTime;
    }
    if (chanDOWN[i]) {
      chanTime[i] = currentTime - chanT0[i];
    }
  }
  
  // red led display the current state with a 3 seconds loop blinking : k blinks = state k (the led remains low in state 0)
  digitalWrite(redLedPin,not state or _blink(currentTime%3000,state));
  // green led is turned on when a program is running.
  digitalWrite(greenLedPin,state);

  // Manual position of the stepper 
  if (chan[5]) {
    // position stepper
    motor.step(stepsReglage);
    delay(20);
  }
  if (chan[6]) {
    // position stepper (other side)
    motor.step(-stepsReglage);
    delay(20);
  }
  
  // State 0 (initial) :
  // switch the light with a low enlightment
  if (not state) {
    if (chanTime[0] > SHORT_CLIC) {
      light = not light;
      analogWrite(lightPin, light*LOW_ENLIGHTMENT);
    }
    // selection the program (from 1 to 4)
    for (i=1; i<5; i++) {
      if (chanTime[i] > SHORT_CLIC and chanTime[i] <= LONG_CLIC) {
        // if light is not swith off, then switch off it!
        light = false;
        analogWrite(lightPin, light*LOW_ENLIGHTMENT);
        state = i;
        // let the time to reach clavicord and begin the peace...
/*        for (i=0; i<DELAY_BEFORE_BEGIN; i++) {
          digitalWrite(greenLedPin,HIGH);
          delay(1000);
          digitalWrite(greenLedPin,LOW);
          delay(500);
        }*/
        // initialize all used variables
        previousTime = currentTime;
        counter = 0;
        flag = false;
      }
    }
  }

  // other states (1,2,3,4) :
  else {
    // press the corresponding button at least 2 seconds to return in init state.
    if (chanTime[state] > LONG_CLIC) {
      motor.setSpeed(baseMotorSpeed);
      state = 0;
      light = false;
      fadeOUT();
      }

    // Behavior of programs
    switch (state) {

      case DAQUIN: {
        // Program DAQUIN
        if (chanTime[DAQUIN] > SHORT_CLIC and chanTime[DAQUIN] <= LONG_CLIC) {
          if (not flag) {
            // begin by fade in the light...
            fadeIN(MAX_ENLIGHTMENT);
            flag = true;
          }
          else { // flag
            // end by fade out the light and turn the motor
            fadeOUT();
            stepsToMove = steps_6_Round;
            if (counter==0) {
              stepsToMove += 80;
            }
            if (counter==4) {
              stepsToMove -= 20;
            }
            motor.step(stepsToMove);
            flag = false;
            counter++;
          }
        }
      }
      break;

      case RAMEAU: {
        // Program RAMEAU:
        delay(1000);
        digitalWrite(lightPin,HIGH);
        delay(2000);
        digitalWrite(lightPin,LOW);

        stepsToMove = steps_6_Round;
        if (counter==4) {
          // move a little bit more
          stepsToMove += 44;
        }
        if (counter == 5) {
          // move a little bit less
          stepsToMove -= 40;
        }
        motor.step(stepsToMove);

        counter++;
        if (counter >= 6) {
          // once more !
          counter = 0;
        }
      }
      break;
      
      case BIBER: {
        // Program Biber:
        if (currentTime - previousTime >= DELAI_BIBER[counter]*1000) {
          if (counter == 0) {
            fadeIN(MAX_ENLIGHTMENT);
          }
          else if (counter > 0 and counter < 6) {
            motor.step(steps_6_Round);
          }
          else if (counter == 6) {
            flag = true;
            motor.setSpeed(slowMotorSpeed);
          }
          else { // counter == 7
            // finish the program (return to init state)
            state = 0;
            light = false;
            fadeOUT();
          }
          counter++;
          previousTime = currentTime;
        }
        if (flag) {
          motor.step(steps_6_Round);
        }
      }
      break;

      case COUPERIN: {
        // Program Couperin:
        if (flag) {
          motor.step(steps_6_Round);
        }

        if (currentTime - previousTime >= DELAI_COUPERIN[counter]*1000) {
          if (counter == 0) {
            fadeIN(MAX_ENLIGHTMENT);
          }
          else if (counter > 0 and counter < 4) {
            motor.step(steps_6_Round);
          }
          else if (counter == 4) {
            flag = true;
            //motor.setSpeed(fastMotorSpeed);
          }
          else { // counter == 5
            // finish the program (return to init state)
            state = 0;
            light = false;
            fadeOUT();
          }
          counter++;
          previousTime = currentTime;
        }
      }
      break;

      default: {
        // if the demanded state does not exists, it is reset to 0.
        state = 0;
      }
      break;
    }
  }
}
