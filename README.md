# StudyBuddy
**Group:** OL27 — ESP32-based study focus device

Built for 3003ICT Programming for Robotics (Trimester 1, 2026) at Griffith University.

## What it does
StudyBuddy physically blocks phone access during 25-minute Pomodoro focus sessions using a servo motor. It uses sensor fusion between an IR sensor and HC-SR04 ultrasonic sensor to detect genuine phone reach attempts, and runs a 6-state FSM with adaptive session timing based on distraction count.

## Hardware
- ESP32
- HC-SR04 ultrasonic sensor
- IR sensor
- Slide switch
- Servo motor
- RGB LED
- Active buzzer

## States
| State | Description |
|---|---|
| IDLE | Awaiting desk presence |
| FOCUSING | Phone blocked, timer running |
| BREAK | Phone accessible, break timer running |
| DISTRACTED | Reach attempt detected during focus |
| FAILSAFE | Invalid sensor reading detected |
| UNLOCKED | 4 cycles completed, session done |

## Simulation
Built and tested using the Wokwi online simulator (wokwi.com).

## Group Members
- Jasmine Hamouda (s5224734)
- Lily Wise (s5424843)
- Amy Wise (s5425135)
