# StudyBuddy
**Group:** OL27 — ESP32-based Pomodoro focus companion  
Built for 3003ICT Programming for Robotics (Trimester 1, 2026) at Griffith University.

## What it does
StudyBuddy physically blocks phone access during Pomodoro focus sessions using a servo motor. It runs a 6-state FSM on an ESP32, using the HC-SR04 ultrasonic sensor to detect phone reach attempts and a slide switch to confirm desk presence. Session length adapts based on distraction count each cycle.

## Hardware
- ESP32
- HC-SR04 ultrasonic sensor — proximity/reach detection (GPIO 26/27)
- Slide switch — desk presence (GPIO 25)
- Slide switch — manual FAILSAFE trigger (GPIO 4)
- Servo motor — physical phone blocker (GPIO 18)
- RGB LED — state indicator (GPIO 13, 12, 14)
- Active buzzer — audio feedback (GPIO 33)

## States
| State | LED | Servo | Description |
|---|---|---|---|
| IDLE | Blue | 90° neutral | Waiting for user to sit at desk |
| FOCUSING | Green | 0° blocked | Phone blocked, focus timer running |
| BREAK | Amber | 180° open | Phone accessible, break timer running |
| DISTRACTED | Red | 0° blocked | Reach attempt detected, servo stays locked |
| FAILSAFE | Purple | 90° neutral | Sensor fault, system locked until reset |
| UNLOCKED | White | 180° open | All 4 cycles complete, session done |

## FSM Transitions
- IDLE → FOCUSING: desk switch flipped on
- FOCUSING → DISTRACTED: ultrasonic detects reach below 10cm
- DISTRACTED → FOCUSING: hand retracted and 2 second timeout elapsed
- FOCUSING → BREAK: focus timer complete
- BREAK → FOCUSING: break timer complete
- BREAK → UNLOCKED: 4 cycles completed
- UNLOCKED → IDLE: user leaves desk, resets cycle count
- Any → FAILSAFE: ultrasonic returns 999.0 (invalid reading) or manual switch triggered
- FAILSAFE → IDLE: manual switch flipped off

## Adaptive Timing
At the end of each focus session, the next session length adjusts based on distraction count:
- 0 distractions → +5 min next session
- 1–2 distractions → unchanged
- 3+ distractions → −5 min next session

## Setup

### Required Library
Add a `libraries.txt` file to the project root containing:

### Timer Constants
All timers are set to 2000ms in the current build for demo purposes. For real deployment update these constants in `main.cpp`:
```cpp
const unsigned long FOCUS_DURATION_BASE = 1500000; // 25 min
const unsigned long BREAK_DURATION      = 300000;  // 5 min
```

## Simulation Notes
Built and tested in Wokwi (wokwi.com). The HC-SR04 in Wokwi has a 2cm minimum distance and cannot trigger the 999.0 FAILSAFE sentinel naturally — a second slide switch on GPIO 4 is included to manually demonstrate FAILSAFE during simulation.

## Group Members
- Jasmine Hamouda (s5224734)
- Lily Wise (s5424843)
- Amy Wise (s5425135)
