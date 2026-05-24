#include <ESP32Servo.h>

// Pin definitions
#define DESK_SW_PIN 25
#define TRIG_PIN 26
#define ECHO_PIN 27
#define SERVO_PIN 18
#define LED_R 13
#define LED_G 12
#define LED_B 14
#define BUZZER_PIN 33
#define FAILSAFE_SW_PIN 4 // manual FAILSAFE trigger

// Timers (demo)
const unsigned long BASE_FOCUS_DURATION = 2000UL;
const unsigned long FOCUS_ADJUST = 2000UL;
const unsigned long MIN_FOCUS_DURATION = 1000UL;
const unsigned long MAX_FOCUS_DURATION = 4000UL;
const unsigned long BREAK_DURATION = 2000UL;
const unsigned long DISTRACT_TIMEOUT = 2000UL;
const float PHONE_THRESHOLD = 10.0;

// FSM states
enum State { IDLE, FOCUSING, BREAK, DISTRACTED, FAILSAFE, UNLOCKED };
State currentState = IDLE;

// Globals
Servo blocker;
unsigned long stateStartTime = 0;
int cycleCount = 0;
int sessionDistracts = 0;
unsigned long currentFocusDur = BASE_FOCUS_DURATION;

// Rainbow celebration
bool doingRainbow = false;
unsigned long rainbowStartTime = 0;
const unsigned long RAINBOW_DURATION = 3000UL;

// Function declarations
void changeState(State newState);
float getDistance();
void ledColour(int r, int g, int b);
void beep(int times);
bool atDesk();
void rainbowTick();
void applyAdaptation();
void logAdaptation(unsigned long prevDur);

void setup() {
    Serial.begin(115200);

    pinMode(DESK_SW_PIN, INPUT_PULLUP);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(FAILSAFE_SW_PIN, INPUT_PULLUP);

    blocker.attach(SERVO_PIN);
    blocker.write(90);

    stateStartTime = millis();
    Serial.println("booted - IDLE");
}

void loop() {
    bool desk = atDesk();
    float distance = getDistance();
    bool phoneReach = (distance < PHONE_THRESHOLD);
    unsigned long elapsed = millis() - stateStartTime;

    // manual FAILSAFE trigger
    if (digitalRead(FAILSAFE_SW_PIN) == LOW && currentState != FAILSAFE) {
        changeState(FAILSAFE);
    }

    // rainbow celebration tick
    if (doingRainbow) {
        rainbowTick();
        if (millis() - rainbowStartTime >= RAINBOW_DURATION) {
            doingRainbow = false;
            if (cycleCount >= 4) {
                changeState(UNLOCKED);
            } else {
                changeState(BREAK);
            }
        }
        return;
    }

    switch (currentState) {

        case IDLE:
            ledColour(0, 0, 1);
            blocker.write(90);

            if (desk) {
                sessionDistracts = 0;
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
                changeState(DISTRACTED);
            } else if (elapsed >= currentFocusDur) {
                applyAdaptation();
                cycleCount++;

                Serial.print("Cycle complete: ");
                Serial.println(cycleCount);

                beep(2);

                if (sessionDistracts == 0) {
                    Serial.println("Perfect session! Rainbow celebration!");
                    doingRainbow = true;
                    rainbowStartTime = millis();
                } else {
                    if (cycleCount >= 4) {
                        changeState(UNLOCKED);
                    } else {
                        changeState(BREAK);
                    }
                }
            }
            break;

        case BREAK:
            ledColour(1, 1, 0);
            blocker.write(180);

            if (!desk) {
                changeState(IDLE);
            } else if (elapsed >= BREAK_DURATION) {
                sessionDistracts = 0;
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
                sessionDistracts++;

                Serial.print("Distraction count this session: ");
                Serial.println(sessionDistracts);

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
                currentFocusDur = BASE_FOCUS_DURATION;
                sessionDistracts = 0;
                beep(3);
                changeState(IDLE);
            }
            break;
    }
}

void applyAdaptation() {
    unsigned long prevDur = currentFocusDur;

    if (sessionDistracts == 0) {
        currentFocusDur = min(currentFocusDur + FOCUS_ADJUST, MAX_FOCUS_DURATION);
    } else if (sessionDistracts >= 3) {
        currentFocusDur = max(currentFocusDur - FOCUS_ADJUST, MIN_FOCUS_DURATION);
    }

    logAdaptation(prevDur);
}

void logAdaptation(unsigned long prevDur) {
    Serial.print("Session distractions: ");
    Serial.println(sessionDistracts);

    if (sessionDistracts == 0) {
        Serial.println("Adaptation: +5 min (perfect focus)");
    } else if (sessionDistracts >= 3) {
        Serial.println("Adaptation: -5 min (too many distractions)");
    } else {
        Serial.println("Adaptation: no change (1-2 distractions)");
    }
}

void rainbowTick() {
    unsigned long t = (millis() - rainbowStartTime) / 200;
    int phase = t % 7;

    switch (phase) {
        case 0: ledColour(1, 0, 0); break;
        case 1: ledColour(0, 1, 0); break;
        case 2: ledColour(0, 0, 1); break;
        case 3: ledColour(1, 1, 0); break;
        case 4: ledColour(0, 1, 1); break;
        case 5: ledColour(1, 0, 1); break;
        case 6: ledColour(1, 1, 1); break;
    }
}

void changeState(State newState) {
    currentState = newState;
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
    return digitalRead(DESK_SW_PIN) == LOW;
}
