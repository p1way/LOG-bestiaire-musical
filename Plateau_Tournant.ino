#include <Stepper.h>

#define BIBER 3
#define RAMEAU 2
#define COUPERIN 4

const int chanPin[4] = {4,2,5,3};
const int lightPin = 9;
const int redLedPin = 8;
const int orangeLedPin = 7;
const int greenLedPin = 6;

const int SHORT_CLIC = 10;
const int LONG_CLIC = 2000;

const int stepsRound = 2038; // pretended 2048...but 2038 is more realistic
const long stepsReglage = 4;
const long steps_6_Round = stepsRound/6;
const long steps_7_Round = stepsRound/7;
const long steps_8_Round = stepsRound/8;
const long steps_12_Round = stepsRound/12;
const long slowMotorSpeed = 100; // step / s
const long baseMotorSpeed = 100; // step / s
//const long fastMotorSpeed = 300; // step / s
const long fastMotorSpeed = 100; // step / s

bool chan[4] = {false,false,false,false};
bool old_chan[4] = {false,false,false,false};
bool chanUP[4] = {false,false,false,false};
bool chanDOWN[4] = {false,false,false,false};
unsigned long chanT0[4] = {0,0,0,0};
unsigned long chanTime[4] = {0,0,0,0};

int i;
int counter = 0;
bool flag = false;

int state = 0;
bool isActive = false;

long counterStepper = 0;
unsigned int counterClick = 0;

unsigned int enlightment = 0;

unsigned long currentTime = 0;
unsigned long previousTime = 0;
// real second delay is 162, but 5 seconds is substract to take into account the delay for fade on the light.
//long delai[8] = {125, 157, 64, 51, 37, 44, 96, 153};
long delai[8] = {5, 5, 5, 5, 5, 5, 5, 15};

Stepper moteur(100, 10, 12, 11, 13);

bool _blink(unsigned int curTime, int curState) {
  while (curState>0) {
    unsigned int curStep = (--curState)*250;
    if (curTime >= curStep and curTime < curStep+50) {
      return HIGH; 
    }
  }
  return LOW;
}

void resetMotor(int counterStepper) {
  counterStepper %= stepsRound;
  if (counterStepper <= stepsRound/2) {
    moteur.step( - counterStepper);
  }
  else {
    moteur.step(stepsRound - counterStepper);
  }
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  Serial.println(steps_6_Round);
    
  moteur.setSpeed(baseMotorSpeed);

  for (i=0;i<4;i++) {
    pinMode(chanPin[i],INPUT);
  }
  pinMode(lightPin,OUTPUT);
  pinMode(redLedPin,OUTPUT);
  pinMode(orangeLedPin,OUTPUT);
  pinMode(greenLedPin,OUTPUT);

  moteur.step(1);

  state = 0; // initial state
}

void loop() {

  // input readings
  currentTime = millis();

  for (i=0;i<4;i++) {
    chan[i] = digitalRead(chanPin[i]);
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
  
  Serial.println(state);
  Serial.println("========");

  // orange led display the current state with a 3 seconds loop blinking : k blinks = state k (the led remains low in state 0)
  digitalWrite(orangeLedPin,chan[2] or _blink(currentTime%3000,state));
  // green led is turned on when the program is running.
  digitalWrite(greenLedPin,isActive);
  // red led indicates state 0;
  digitalWrite(redLedPin,not state);

  // State 0 (initial) : waiting for a program command.
  if (not state) {
    if (chan[0]) {
      // position stepper
      moteur.step(stepsReglage);
      analogWrite(lightPin,100);
    }
    else if (chan[1]) {
      // position stepper (other side)
      moteur.step(-stepsReglage);
      analogWrite(lightPin,100);
    }
    else {
      analogWrite(lightPin,0);
    }
    if (chanTime[2] > SHORT_CLIC and chanTime[2] <= LONG_CLIC) {
      // k click for demanding state number k.
      counterClick++;
    }
    if (chanTime[3] > SHORT_CLIC) {
      // validate the required state
      if (counterClick <= 5) {
        state = counterClick;
      }
      counterClick = 0;
      if (state == RAMEAU) {
        moteur.setSpeed(fastMotorSpeed);
      } else {
        moteur.setSpeed(baseMotorSpeed);
      }
    }
  }
  else {
    if (chanTime[2] > LONG_CLIC) {
      // when you press the third button at least 3 seconds, the state is reinitialized.
      isActive = false;
      state = 0;
      // reset light
      enlightment = 0;
      analogWrite(lightPin,enlightment);
      //resetMotor(counterStepper);
    }
    if (chanTime[3] > SHORT_CLIC) {
      isActive = not isActive;
      if (isActive) {
        // initialize motor step and counters and clock.
        previousTime = currentTime;
        counterStepper = 0;
        counter = 0;
        flag = false;
        // let the time to reach clavicord and begin the peace...
        for (i=0;i<5;i++) {
          digitalWrite(greenLedPin,HIGH);
          delay(1000);
          digitalWrite(greenLedPin,LOW);
          delay(500);
        }
      }
      else {
        while (enlightment > 0) {
          analogWrite(lightPin,--enlightment);
          delay(20);
        }
      }
    }
    if (isActive) {
      switch (state) {
        case BIBER: {
          // Program Biber:
          if (counter < 8) {
            if (currentTime - previousTime >= delai[counter]*1000) {
              if (counter == 0) {
                // gently fade on the light
                while (enlightment < 255) {
                  analogWrite(lightPin,++enlightment);
                  delay(20);
                }
              }
              else if (counter > 0 and counter < 6) {
                moteur.step(steps_6_Round);
                counterStepper += steps_6_Round;
              }
              else if (counter == 6) {
                flag = true;
              }
              else { // counter == 7
                // finish the program by gently fade off the light
                moteur.setSpeed(baseMotorSpeed);
                while (enlightment > 0) {
                  analogWrite(lightPin,--enlightment);
                  delay(20);
                }
                isActive = false;
                //resetMotor(counterStepper);
              }
              counter++;
              previousTime = currentTime;
            }
          }
          if (flag) {
            moteur.setSpeed(slowMotorSpeed);
            moteur.step(steps_6_Round);
            counterStepper += steps_6_Round;
          }
        }
        break;
        case RAMEAU: {
/*          // Program RAMEAU:
          if (not flag) {
            // gently fade on the light
            while (enlightment < 255) {
              analogWrite(lightPin,++enlightment);
              delay(20);
            }
            flag = true;
          }
          moteur.step(steps_6_Round);
          counterStepper += steps_6_Round;*/
          // Program RAMEAU:
          delay(1000);
          digitalWrite(lightPin,HIGH);
          delay(2000);
          digitalWrite(lightPin,LOW);
          if (counter==4) {
            moteur.step(steps_6_Round + 44);
            counterStepper += steps_6_Round+44;
          }
          else if (counter == 5) {
            moteur.step(steps_6_Round - 40);
            counterStepper += steps_6_Round-40;
          }
          else {
            moteur.step(steps_6_Round);
            counterStepper += steps_6_Round;
          }
          counter++;
          if (counter==6) {
            counter = 0;
          }
        }
        break;
        case 5 : {
          enlightment = 150;
          analogWrite(lightPin,enlightment);
          if (chan[0]) {
            // position stepper
            moteur.step(stepsReglage);
          }
          else if (chan[1]) {
            // position stepper (other side)
            moteur.step(-stepsReglage);
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
    else { // not isActive : reglage
      if (chan[0]) {
        // position stepper
        moteur.step(stepsReglage);
        analogWrite(lightPin,100);
      }
      else if (chan[1]) {
        // position stepper (other side)
        moteur.step(-stepsReglage);
        analogWrite(lightPin,100);
      }
      else {
        analogWrite(lightPin,0);
      }
    }
  }
}
