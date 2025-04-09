#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <ESP32Servo.h>

// RFID模块引脚定义
#define SS_PIN  5    // ESP32的SDA(SS)连接到GPIO5
#define RST_PIN 22   // ESP32的RST连接到GPIO22

// LED灯引脚
#define LED_PIN 2    // ESP32主板上自带的LED通常连接到GPIO2

// 舵机引脚
#define SERVO_PIN 15 // 舵机连接到GPIO15

// 定义键盘的行数和列数
const byte ROWS = 4;
const byte COLS = 4;

// 定义键盘上的按键符号
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// 键盘引脚定义
byte rowPins[ROWS] = {32, 33, 25, 26}; // 根据您的实际连接修改
byte colPins[COLS] = {27, 14, 12, 13}; // 根据您的实际连接修改

// 创建对象
MFRC522 rfid(SS_PIN, RST_PIN);  // 创建MFRC522实例
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo doorLock;  // 创建舵机对象

// 授权的RFID卡UID (82 46 B8 02)
byte authorizedUID[4] = {0x82, 0x46, 0xB8, 0x02};

// 设置密码
const char* PASSWORD = "0530";
const byte PASSWORD_LENGTH = 4;

// 存储用户输入
char enteredKeys[10]; // 可以存储更长的输入
byte currentPosition = 0;

// 门锁状态
bool doorIsOpen = false;
unsigned long doorOpenTime = 0; // 记录开门时间
const unsigned long DOOR_OPEN_DURATION = 5000; // 门保持打开的时间(毫秒)

void setup() {
  Serial.begin(115200);
  SPI.begin();            // 初始化SPI总线
  rfid.PCD_Init();        // 初始化MFRC522
  doorLock.attach(SERVO_PIN); // 初始化舵机
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // 默认LED关闭
  
  // 确保门初始状态为关闭
  closeDoor();
  
  Serial.println("智能门锁系统已启动");
  Serial.println("请使用授权卡或输入密码：");
}

void loop() {
  // 检查门是否需要自动关闭
  if (doorIsOpen && (millis() - doorOpenTime > DOOR_OPEN_DURATION)) {
    closeDoor();
  }
  
  // 检查RFID卡
  checkRFID();
  
  // 检查键盘输入
  checkKeypad();
}

void checkRFID() {
  // 寻找新卡
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // 显示卡片的UID
  Serial.print("检测到卡片UID:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
  
  // 检查是否是授权卡片
  if (compareUID(rfid.uid.uidByte, authorizedUID, rfid.uid.size)) {
    Serial.println("授权卡片已识别!");
    openDoor();
  } else {
    Serial.println("未授权的卡片");
    errorBlink();
  }
  
  // 停止PICC
  rfid.PICC_HaltA();
  // 停止加密
  rfid.PCD_StopCrypto1();
}

void checkKeypad() {
  char key = keypad.getKey();
  
  if (key) {
    // 按键反馈 - 短闪一下LED
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    
    // 处理特殊键
    if (key == '*') {
      // '*'键用于清除输入
      resetInput();
      Serial.println("\n输入已清除，请重新输入:");
    } 
    else if (key == '#') {
      // '#'键用于确认输入并验证密码
      verifyPassword();
    }
    else {
      // 存储按键并显示(用*号代替实际数字增加安全性)
      if (currentPosition < 9) { // 预留一位给结束符'\0'
        enteredKeys[currentPosition++] = key;
        enteredKeys[currentPosition] = '\0'; // 确保字符串正确终止
        Serial.print("*");
      }
    }
  }
}

// 重置输入
void resetInput() {
  currentPosition = 0;
  enteredKeys[0] = '\0';
}

// 验证密码
void verifyPassword() {
  Serial.println("\n正在验证密码...");
  
  // 比较输入的密码和存储的密码
  bool passwordCorrect = true;
  
  // 检查长度是否相同
  if (currentPosition != PASSWORD_LENGTH) {
    passwordCorrect = false;
  } else {
    // 逐字符比较
    for (byte i = 0; i < PASSWORD_LENGTH; i++) {
      if (enteredKeys[i] != PASSWORD[i]) {
        passwordCorrect = false;
        break;
      }
    }
  }
  
  // 根据验证结果执行操作
  if (passwordCorrect) {
    Serial.println("密码正确! 门已开启!");
    openDoor();
  } else {
    Serial.println("密码错误! 请重试.");
    errorBlink();
  }
  
  // 重置输入以准备下一次尝试
  resetInput();
  Serial.println("请使用授权卡或输入密码：");
}

// 比较两个UID是否相同
bool compareUID(byte* uid1, byte* uid2, byte size) {
  for (byte i = 0; i < size; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

// 打开门
void openDoor() {
  if (!doorIsOpen) {
    Serial.println("打开门...");
    doorLock.write(90); // 舵机转到90度位置
    successBlink();
    doorIsOpen = true;
    doorOpenTime = millis(); // 记录开门时间
  }
}

// 关闭门
void closeDoor() {
  if (doorIsOpen) {
    Serial.println("关闭门...");
    doorLock.write(0); // 舵机转回0度位置
    doorIsOpen = false;
  }
}

// 成功时的LED闪烁模式
void successBlink() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

// 错误时的LED闪烁模式
void errorBlink() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}