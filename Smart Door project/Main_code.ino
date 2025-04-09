#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <ESP32Servo.h>

// RFID module pin definitions
#define SS_PIN  5    // ESP32's SDA(SS) connected to GPIO5
#define RST_PIN 22   // ESP32's RST connected to GPIO22

// LED pin
#define LED_PIN 2    // ESP32 onboard LED typically connected to GPIO2

// Servo pin
#define SERVO_PIN 15 // Servo connected to GPIO15

// Define keypad rows and columns
const byte ROWS = 4;
const byte COLS = 4;

// Define keypad button symbols
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Keypad pin definitions
byte rowPins[ROWS] = {32, 33, 25, 26}; // Modify according to your actual connections
byte colPins[COLS] = {27, 14, 12, 13}; // Modify according to your actual connections

// Create objects
MFRC522 rfid(SS_PIN, RST_PIN);  // Create MFRC522 instance
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo doorLock;  // Create servo object

// Authorized RFID card UID (82 46 B8 02)
byte authorizedUID[4] = {0x82, 0x46, 0xB8, 0x02};

// Set password
const char* PASSWORD = "0530";
const byte PASSWORD_LENGTH = 4;

// Store user input
char enteredKeys[10]; // Can store longer input
byte currentPosition = 0;

// Door lock status
bool doorIsOpen = false;
unsigned long doorOpenTime = 0; // Record door open time
const unsigned long DOOR_OPEN_DURATION = 5000; // Time door remains open (milliseconds)

void setup() {
  Serial.begin(115200);
  SPI.begin();            // Initialize SPI bus
  rfid.PCD_Init();        // Initialize MFRC522
  doorLock.attach(SERVO_PIN); // Initialize servo
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Default LED off
  
  // Ensure door is initially closed
  closeDoor();
  
  Serial.println("Smart door lock system started");
  Serial.println("Please use authorized card or enter password:");
}

void loop() {
  // Check if door needs to be automatically closed
  if (doorIsOpen && (millis() - doorOpenTime > DOOR_OPEN_DURATION)) {
    closeDoor();
  }
  
  // Check RFID card
  checkRFID();
  
  // Check keypad input
  checkKeypad();
}

void checkRFID() {
  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // Display card UID
  Serial.print("Card UID detected:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
  
  // Check if it's an authorized card
  if (compareUID(rfid.uid.uidByte, authorizedUID, rfid.uid.size)) {
    Serial.println("Authorized card recognized!");
    openDoor();
  } else {
    Serial.println("Unauthorized card");
    errorBlink();
  }
  
  // Stop PICC
  rfid.PICC_HaltA();
  // Stop encryption
  rfid.PCD_StopCrypto1();
}

void checkKeypad() {
  char key = keypad.getKey();
  
  if (key) {
    // Key feedback - short LED flash
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    
    // Handle special keys
    if (key == '*') {
      // '*' key used to clear input
      resetInput();
      Serial.println("\nInput cleared, please enter again:");
    } 
    else if (key == '#') {
      // '#' key used to confirm input and verify password
      verifyPassword();
    }
    else {
      // Store key press and display (* for security)
      if (currentPosition < 9) { // Reserve one position for null terminator
        enteredKeys[currentPosition++] = key;
        enteredKeys[currentPosition] = '\0'; // Ensure string is properly terminated
        Serial.print("*");
      }
    }
  }
}

// Reset input
void resetInput() {
  currentPosition = 0;
  enteredKeys[0] = '\0';
}

// Verify password
void verifyPassword() {
  Serial.println("\nVerifying password...");
  
  // Compare entered password with stored password
  bool passwordCorrect = true;
  
  // Check if length is the same
  if (currentPosition != PASSWORD_LENGTH) {
    passwordCorrect = false;
  } else {
    // Compare character by character
    for (byte i = 0; i < PASSWORD_LENGTH; i++) {
      if (enteredKeys[i] != PASSWORD[i]) {
        passwordCorrect = false;
        break;
      }
    }
  }
  
  // Execute actions based on verification result
  if (passwordCorrect) {
    Serial.println("Correct password! Door opened!");
    openDoor();
  } else {
    Serial.println("Incorrect password! Please try again.");
    errorBlink();
  }
  
  // Reset input for next attempt
  resetInput();
  Serial.println("Please use authorized card or enter password:");
}

// Compare two UIDs for equality
bool compareUID(byte* uid1, byte* uid2, byte size) {
  for (byte i = 0; i < size; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

// Open door
void openDoor() {
  if (!doorIsOpen) {
    Serial.println("Opening door...");
    doorLock.write(90); // Rotate servo to 90 degrees
    successBlink();
    doorIsOpen = true;
    doorOpenTime = millis(); // Record door open time
  }
}

// Close door
void closeDoor() {
  if (doorIsOpen) {
    Serial.println("Closing door...");
    doorLock.write(0); // Rotate servo back to 0 degrees
    doorIsOpen = false;
  }
}

// LED blinking pattern for success
void successBlink() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

// LED blinking pattern for error
void errorBlink() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}
