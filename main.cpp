#include <ESP32Servo.h>

// Pin definitions
#define IR_PIN           25
#define TRIG_PIN         26
#define ECHO_PIN         27
#define SERVO_PIN        18
#define LED_R            13
#define LED_G            12
#define LED_B            14
#define BUZZER_PIN       33
#define FAILSAFE_SW_PIN   4  // manual FAILSAFE trigger

// Timers (demo)
const unsigned long FOCUS_DURATION_BASE = 2000;
const unsigned long BREAK_DURATION      = 2000;
const unsigned long DISTRACT_TIMEOUT    = 2000;
const float         PHONE_THRESHOLD     = 10.0;

// FSM states
enum State { IDLE, FOCUSING, BREAK, DISTRACTED, FAILSAFE, UNLOCKED };
State currentState = IDLE;

// Globals
Servo blocker;
unsigned long stateStartTime  = 0;
int cycleCount                = 0;
int distractionCount          = 0;
unsigned long focusDuration   = FOCUS_DURATION_BASE;

// Function declarations
void changeState(State newState);
float getDistance();
void ledColour(int r, int g, int b);
void beep(int times);
bool atDesk();

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN,          INPUT_PULLUP);
  pinMode(TRIG_PIN,        OUTPUT);
  pinMode(ECHO_PIN,        INPUT);
  pinMode(LED_R,           OUTPUT);
  pinMode(LED_G,           OUTPUT);
  pinMode(LED_B,           OUTPUT);
  pinMode(BUZZER_PIN,      OUTPUT);
  pinMode(FAILSAFE_SW_PIN, INPUT_PULLUP);
  blocker.attach(SERVO_PIN);
  blocker.write(90);
  stateStartTime = millis();
  Serial.println("booted - IDLE");
}

void loop() {
  bool desk       = atDesk();
  float distance  = getDistance();
  bool phoneReach = (distance < PHONE_THRESHOLD);
  unsigned long elapsed = millis() - stateStartTime;

  // manual FAILSAFE trigger
  if (digitalRead(FAILSAFE_SW_PIN) == LOW && currentState != FAILSAFE) {
    changeState(FAILSAFE);
  }

  switch (currentState) {

    case IDLE:
      ledColour(0, 0, 1);
      blocker.write(90);
      if (desk) {
        distractionCount = 0;
        focusDuration    = FOCUS_DURATION_BASE;
        beep(1);
        changeState(FOCUSING);
      }
      break;

    case FOCUSING:
      ledColour(0, 1, 0);
      blocker.write(0);
      if (!desk) {
        changeState(IDLE);
      } else if (phoneReach) {
        distractionCount++;
        Serial.print("Distraction count: ");
        Serial.println(distractionCount);
        changeState(DISTRACTED);
      } else if (elapsed >= focusDuration) {
        cycleCount++;
        Serial.print("Cycle complete: ");
        Serial.println(cycleCount);
        beep(2);

        // adaptive timing for next session
        if (distractionCount == 0) {
          focusDuration = FOCUS_DURATION_BASE + 2000; // +5 min (demo: +2s)
          Serial.println("Great focus! +5 min next session");
        } else if (distractionCount <= 2) {
          focusDuration = FOCUS_DURATION_BASE;         // unchanged
          Serial.println("Session unchanged next cycle");
        } else {
          focusDuration = FOCUS_DURATION_BASE - 1000; // -5 min (demo: -1s)
          Serial.println("Too many distractions. -5 min next session");
        }
        distractionCount = 0;

        if (cycleCount >= 4) {
          changeState(UNLOCKED);
        } else {
          changeState(BREAK);
        }
      }
      break;

    case BREAK:
      ledColour(1, 1, 0);
      blocker.write(180);
      if (!desk) {
        changeState(IDLE);
      } else if (elapsed >= BREAK_DURATION) {
        beep(1);
        changeState(FOCUSING);
      }
      break;

    case DISTRACTED:
      ledColour(1, 0, 0);
      blocker.write(0);
      if (!desk) {
        changeState(IDLE);
      } else if (!phoneReach && elapsed >= DISTRACT_TIMEOUT) {
        beep(1);
        changeState(FOCUSING);
      }
      break;

    case FAILSAFE:
      ledColour(1, 0, 1);
      blocker.write(90);
      Serial.println("FAILSAFE - check sensors");
      if (digitalRead(FAILSAFE_SW_PIN) == HIGH) {
        changeState(IDLE);
      }
      break;

    case UNLOCKED:
      ledColour(1, 1, 1);
      blocker.write(180);
      if (!desk) {
        cycleCount = 0;
        beep(3);
        changeState(IDLE);
      }
      break;
  }
}

void changeState(State newState) {
  currentState   = newState;
  stateStartTime = millis();
  const char* names[] = {
    "IDLE", "FOCUSING", "BREAK", "DISTRACTED", "FAILSAFE", "COMPLETE - PHONE UNLOCKED"
  };
  Serial.println(names[newState]);
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999.0;
  return duration * 0.034 / 2.0;
}

void ledColour(int r, int g, int b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

bool atDesk() {
  return digitalRead(IR_PIN) == LOW;
}
