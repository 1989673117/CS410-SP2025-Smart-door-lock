#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

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

// WiFi credentials
const char* ssid = "SmartDoorLock";      // Your WiFi AP name
const char* password = "doorlock123";    // WiFi password

// Create web server
WebServer server(80);

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

// Access log
#define MAX_LOG_ENTRIES 10
String accessLog[MAX_LOG_ENTRIES];
int logIndex = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();            // Initialize SPI bus
  rfid.PCD_Init();        // Initialize MFRC522
  doorLock.attach(SERVO_PIN); // Initialize servo
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Default LED off
  
  // Ensure door is initially closed
  closeDoor();
  
  // Start WiFi Access Point
  WiFi.softAP(ssid, password);
  
  Serial.println("Smart door lock system started");
  Serial.println("WiFi Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // Define web server endpoints
  server.on("/", handleRoot);
  server.on("/open", HTTP_GET, handleOpen);
  server.on("/close", HTTP_GET, handleClose);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/log", HTTP_GET, handleLog);
  
  // Special endpoints optimized for Shortcuts
  server.on("/api/open", HTTP_GET, handleApiOpen);
  server.on("/api/close", HTTP_GET, handleApiClose);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  
  // Start web server
  server.begin();
  Serial.println("HTTP server started");
  
  Serial.println("Please use authorized card or enter password:");
  
  // Add initial log entry
  addLogEntry("System initialized");
}

void loop() {
  // Handle web server client requests
  server.handleClient();
  
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
  String cardUID = "";
  Serial.print("Card UID detected:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
    
    // Build UID string for log
    cardUID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    cardUID += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) cardUID += " ";
  }
  Serial.println();
  
  // Check if it's an authorized card
  if (compareUID(rfid.uid.uidByte, authorizedUID, rfid.uid.size)) {
    Serial.println("Authorized card recognized!");
    addLogEntry("Access granted: Card " + cardUID);
    openDoor();
  } else {
    Serial.println("Unauthorized card");
    addLogEntry("Access denied: Card " + cardUID);
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
    addLogEntry("Access granted: Keypad PIN");
    openDoor();
  } else {
    Serial.println("Incorrect password! Please try again.");
    addLogEntry("Access denied: Wrong PIN");
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
    addLogEntry("Door closed");
  }
}

// Add entry to access log
void addLogEntry(String entry) {
  // Add timestamp to entry
  unsigned long runTime = millis() / 1000; // Get runtime in seconds
  String timestamp = String(runTime / 3600) + "h:" + 
                     String((runTime % 3600) / 60) + "m:" + 
                     String(runTime % 60) + "s";
  
  accessLog[logIndex] = timestamp + " - " + entry;
  logIndex = (logIndex + 1) % MAX_LOG_ENTRIES; // Circular buffer
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

// Web server handlers
void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; background-color: #f5f5f5; }";
  html += "h1 { color: #333; }";
  html += ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "button { background-color: #4CAF50; border: none; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 10px 5px; cursor: pointer; border-radius: 5px; }";
  html += "button.close { background-color: #f44336; }";
  html += "button.status { background-color: #2196F3; }";
  html += "button.log { background-color: #607D8B; }";
  html += ".status-container { margin-top: 20px; padding: 10px; background-color: #e8f5e9; border-radius: 5px; }";
  html += ".shortcuts-info { margin-top: 30px; padding: 15px; background-color: #e3f2fd; border-radius: 5px; text-align: left; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>Smart Door Lock Control</h1>";
  html += "<div>";
  html += "<button onclick='location.href=\"/open\"'>Open Door</button>";
  html += "<button class='close' onclick='location.href=\"/close\"'>Close Door</button>";
  html += "</div>";
  html += "<div>";
  html += "<button class='status' onclick='location.href=\"/status\"'>Check Status</button>";
  html += "<button class='log' onclick='location.href=\"/log\"'>View Log</button>";
  html += "</div>";
  html += "<div class='status-container'>";
  html += "<p>Door status: <strong>" + String(doorIsOpen ? "OPEN" : "CLOSED") + "</strong></p>";
  html += "</div>";
  html += "<div class='shortcuts-info'>";
  html += "<h3>iPhone Shortcuts Integration:</h3>";
  html += "<p>Use these URLs in your iPhone Shortcuts app:</p>";
  html += "<ul>";
  html += "<li>Open Door: <code>http://" + WiFi.softAPIP().toString() + "/api/open</code></li>";
  html += "<li>Close Door: <code>http://" + WiFi.softAPIP().toString() + "/api/close</code></li>";
  html += "<li>Check Status: <code>http://" + WiFi.softAPIP().toString() + "/api/status</code></li>";
  html += "</ul>";
  html += "</div>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleOpen() {
  openDoor();
  addLogEntry("Door opened via Web UI");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleClose() {
  closeDoor();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStatus() {
  String status = doorIsOpen ? "OPEN" : "CLOSED";
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; background-color: #f5f5f5; }";
  html += "h1 { color: #333; }";
  html += ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "button { background-color: #4CAF50; border: none; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 10px 5px; cursor: pointer; border-radius: 5px; }";
  html += ".status { font-size: 24px; font-weight: bold; margin: 20px; padding: 20px; background-color: " + String(doorIsOpen ? "#ffcccc" : "#ccffcc") + "; border-radius: 5px; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>Door Status</h1>";
  html += "<div class='status'>Door is currently: " + status + "</div>";
  html += "<button onclick='location.href=\"/\"'>Back to Main</button>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleLog() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; background-color: #f5f5f5; }";
  html += "h1 { color: #333; }";
  html += ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "button { background-color: #4CAF50; border: none; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 10px 5px; cursor: pointer; border-radius: 5px; }";
  html += ".log-entry { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }";
  html += ".log-container { max-height: 300px; overflow-y: auto; margin: 20px 0; border: 1px solid #ddd; border-radius: 5px; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>Access Log</h1>";
  html += "<div class='log-container'>";
  
  // Display log entries in reverse chronological order
  int count = 0;
  for (int i = (logIndex == 0 ? MAX_LOG_ENTRIES - 1 : logIndex - 1); count < MAX_LOG_ENTRIES; i = (i == 0 ? MAX_LOG_ENTRIES - 1 : i - 1)) {
    if (accessLog[i] != "") {
      html += "<div class='log-entry'>" + accessLog[i] + "</div>";
    }
    count++;
  }
  
  html += "</div>";
  html += "<button onclick='location.href=\"/\"'>Back to Main</button>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// API handlers for iPhone Shortcuts
void handleApiOpen() {
  openDoor();
  addLogEntry("Door opened via iPhone Shortcut");
  String jsonResponse = "{\"status\":\"success\",\"message\":\"Door opened\",\"door\":\"open\"}";
  server.send(200, "application/json", jsonResponse);
}

void handleApiClose() {
  closeDoor();
  addLogEntry("Door closed via iPhone Shortcut");
  String jsonResponse = "{\"status\":\"success\",\"message\":\"Door closed\",\"door\":\"closed\"}";
  server.send(200, "application/json", jsonResponse);
}

void handleApiStatus() {
  String doorState = doorIsOpen ? "open" : "closed";
  String jsonResponse = "{\"status\":\"success\",\"door\":\"" + doorState + "\"}";
  server.send(200, "application/json", jsonResponse);
}