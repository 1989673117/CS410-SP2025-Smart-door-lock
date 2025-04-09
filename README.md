# CS410-SP2025-Smart-door-lock
An ESP32-based smart door lock system integrating RFID recognition, capacitive touch, and remote control for secure home access. Features multiple unlocking methods, real-time monitoring, and mobile app control. Developed for CS410-SP2025 using Arduino framework.
# Hardware Requirements
* ESP32 Development Board
* RC522 RFID Reader Module
* 4x4 Matrix Keypad
* SG90 Micro Servo
* Jumper wires
* Breadboard
# Wiring Connections
* Generally speaking, ESP32 should be connected to the computer directly through the USB interface, but some computers' Arduino IDE cannot find the COM port connected to ESP32. This is because the computer lacks a driver that adapts to the interface. You can use the interface driver file included in the project to fix this problem. For specific installation methods, please refer to https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers
# Dependencies
The project requires the following Arduino libraries:
* SPI.h
* MFRC522.h
* Keypad.h
* ESP32Servo.h

Please install these support libraries before uploading the program.
