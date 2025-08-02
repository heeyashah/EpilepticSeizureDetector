# Epileptic Seizure Risk Warning System

A real-time, Arduino-based system designed to detect and escalate warnings for epileptic seizure risks triggered by flashing lights. This project was built as part of a Digital Systems course, emphasizing the use of timers, I2C communication, and hardware interfacing.

## Project Overview

This system simulates potentially seizure-triggering flashing light patterns using an LED. A photoresistor monitors the frequency of flashes, classifying the risk into four categories:  
- **Safe**  
- **Warning**  
- **Alarm**  
- **Emergency**

If high-risk flashing persists for more than 5 seconds—or medium-risk for 10 seconds without user acknowledgment—the system escalates the alert level.

A second Arduino, connected via **I2C**, displays real-time warnings and alerts on an **LCD screen**.

## Features

- Flash frequency classification based on photoresistor input  
- Escalation logic with time-based thresholds  
- Real-time visual warnings via LCD  
- I2C communication between two Arduino boards  
- Timer- and clock-driven LED blinking logic

## Tools & Technologies

- Arduino Uno (x2)  
- Timers & Clocks  
- Photoresistor  
- LED  
- LCD Display  
- I2C Communication  
- C/C++ (Arduino IDE)
