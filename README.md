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
# Multiple unlocking methods
The smart door lock in this project includes three unlocking methods: RFID card recognition, password, and mobile phone remote unlocking. Users can choose the unlocking method they want through the collaboration of different modules.
* RFID method: By setting the unlock card ID in the program, the designated card used for unlocking can be recorded. When unlocking, just put the card close to the RFID module to unlock. The system compares the scanned card's unique identifier with pre-authorized values, providing convenient contactless access.
* Keypad Method: A 4-digit PIN code (0530) is pre-configured in the system. Users can input this code using the 4×4 matrix keypad and press the '#' key to confirm. The system verifies the entered sequence against the stored PIN, granting access upon successful verification. If an error occurs during input, pressing the '*' key clears the current entry.
* Mobile Control Method: Utilizing ESP32's built-in WiFi capabilities, the door lock can be controlled remotely through any smartphone. By connecting to the door lock's WiFi network and accessing the web interface or using custom iPhone Shortcuts, users can open or close the door with a simple tap on their device screen, offering flexibility and remote access functionality.
