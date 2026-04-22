#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <Firebase_ESP_Client.h>
#include <ESP32Servo.h>

// --- FIREBASE CONFIGURATION ---
#define FIREBASE_HOST "smartatm-wifi-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "YOUR_NEW_SECRET_HERE"  // Put your regenerated secret here

// --- WIFI CONFIGURATION ---
const char* ssid = "KUNNATH 2.4Ghz";
const char* password = "9847222218";

// --- PIN DEFINITIONS ---
const int RED_LED_PIN = 2;
const int GREEN_LED_PIN = 4;
const int SERVO_PIN = 13;

// --- OBJECTS ---
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
Servo myServo;

// --- STATE ---
bool lcdReady = false;
bool userLoggedIn = false;
String currentUser = "";
long currentBalance = 0;
String lastAction = "idle";
unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 1000;  // Check Firebase every 1 second

// ============================
//    HARDWARE FEEDBACK
// ============================

void flashSuccess() {
  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(500);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void flashError() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(RED_LED_PIN, HIGH); delay(100);
    digitalWrite(RED_LED_PIN, LOW); delay(100);
  }
}

void dispenseMoney(long amount) {
  Serial.print("Dispensing Rs.");
  Serial.println(amount);
  myServo.attach(SERVO_PIN);
  myServo.write(150);
  delay(1200);
  myServo.write(30);
  delay(500);
  myServo.detach();
}

// ============================
//    LCD FUNCTIONS
// ============================

bool initLCD() {
  Wire.begin(21, 22);
  Wire.beginTransmission(0x27);
  if (Wire.endTransmission() == 0) {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    return true;
  }
  Wire.beginTransmission(0x3F);
  if (Wire.endTransmission() == 0) {
    Serial.println("LCD found at 0x3F - update constructor!");
    return false;
  }
  Serial.println("LCD not found!");
  return false;
}

// Show two lines on LCD
void lcdShow(const char* line1, const char* line2) {
  if (!lcdReady) return;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  if (line2) {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

// Show two lines using String objects
void lcdShowStr(String line1, String line2) {
  if (!lcdReady) return;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// Show user info: "User: name" + "Bal: Rs.XXXX"
void lcdShowUser() {
  lcdShowStr("User: " + currentUser, "Bal: Rs." + String(currentBalance));
}

// Show welcome screen
void lcdShowWelcome() {
  lcdShow("Blink Bank Ready", "Waiting...");
}

// ============================
//    FIREBASE MONITORING
// ============================

void checkFirebase() {
  // Only check at intervals
  if (millis() - lastCheck < CHECK_INTERVAL) return;
  lastCheck = millis();
  if (!Firebase.ready()) return;

  // Read current action
  if (!Firebase.RTDB.getString(&fbdo, "/atm/session/action")) return;
  String action = fbdo.stringData();

  // Skip if action hasn't changed
  if (action == lastAction) return;
  lastAction = action;

  Serial.print("[Action] ");
  Serial.println(action);

  // ---- LOGIN ----
  if (action == "login") {
    // Read username
    if (Firebase.RTDB.getString(&fbdo, "/atm/session/username")) {
      currentUser = fbdo.stringData();
    }
    // Read balance
    if (Firebase.RTDB.getInt(&fbdo, "/atm/session/balance")) {
      currentBalance = fbdo.intData();
    }

    userLoggedIn = true;
    digitalWrite(GREEN_LED_PIN, HIGH);  // Green LED stays ON

    // LCD: Show user + balance
    lcdShowUser();
    flashSuccess();

    Serial.print("Logged in: ");
    Serial.print(currentUser);
    Serial.print(" | Rs.");
    Serial.println(currentBalance);

    // Acknowledge
    Firebase.RTDB.setString(&fbdo, "/atm/session/action", "idle");
    lastAction = "idle";
  }

  // ---- DEPOSIT ----
  else if (action == "deposit") {
    long amount = 0;
    if (Firebase.RTDB.getInt(&fbdo, "/atm/session/amount")) {
      amount = fbdo.intData();
    }
    if (Firebase.RTDB.getInt(&fbdo, "/atm/session/balance")) {
      currentBalance = fbdo.intData();
    }

    if (amount > 0) {
      flashSuccess();

      // LCD: "Deposited Rs.500" / "Bal: Rs.1500"
      lcdShowStr(
        "Deposited Rs." + String(amount),
        "Bal: Rs." + String(currentBalance)
      );

      Serial.print("Deposited Rs.");
      Serial.print(amount);
      Serial.print(" | New Bal: Rs.");
      Serial.println(currentBalance);

      delay(3000);       // Show deposit message for 3 seconds
      lcdShowUser();     // Back to user view
    } else {
      flashError();
    }

    Firebase.RTDB.setString(&fbdo, "/atm/session/action", "idle");
    lastAction = "idle";
  }

  // ---- WITHDRAW ----
  else if (action == "withdraw") {
    long amount = 0;
    if (Firebase.RTDB.getInt(&fbdo, "/atm/session/amount")) {
      amount = fbdo.intData();
    }
    if (Firebase.RTDB.getInt(&fbdo, "/atm/session/balance")) {
      currentBalance = fbdo.intData();
    }

    if (amount > 0) {
      // LCD: "Withdrew Rs.200" / "Dispensing..."
      lcdShowStr(
        "Withdrew Rs." + String(amount),
        "Dispensing..."
      );

      dispenseMoney(amount);  // Servo moves
      flashSuccess();

      // LCD: "Withdrew Rs.200" / "Bal: Rs.800"
      lcdShowStr(
        "Withdrew Rs." + String(amount),
        "Bal: Rs." + String(currentBalance)
      );

      Serial.print("Withdrew Rs.");
      Serial.print(amount);
      Serial.print(" | New Bal: Rs.");
      Serial.println(currentBalance);

      delay(3000);       // Show withdraw message for 3 seconds
      lcdShowUser();     // Back to user view
    } else {
      flashError();
      lcdShow("Withdraw Failed!", "Invalid amount");
      delay(2000);
      lcdShowUser();
    }

    Firebase.RTDB.setString(&fbdo, "/atm/session/action", "idle");
    lastAction = "idle";
  }

  // ---- CHECK BALANCE ----
  else if (action == "check") {
    if (Firebase.RTDB.getInt(&fbdo, "/atm/session/balance")) {
      currentBalance = fbdo.intData();
    }
    lcdShowUser();
    flashSuccess();

    Serial.print("Balance checked: Rs.");
    Serial.println(currentBalance);

    Firebase.RTDB.setString(&fbdo, "/atm/session/action", "idle");
    lastAction = "idle";
  }

  // ---- LOGOUT ----
  else if (action == "logout") {
    userLoggedIn = false;
    currentUser = "";
    currentBalance = 0;
    digitalWrite(GREEN_LED_PIN, LOW);  // Green LED OFF
    lcdShowWelcome();

    Serial.println("User logged out.");

    Firebase.RTDB.setString(&fbdo, "/atm/session/action", "idle");
    Firebase.RTDB.setBool(&fbdo, "/atm/session/active", false);
    lastAction = "idle";
  }
}

// ============================
//    SETUP
// ============================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Blink Bank ATM ===");

  // LED pins
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  // Servo timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myServo.setPeriodHertz(50);

  // LCD init (once)
  lcdReady = initLCD();
  lcdShow("Blink Bank", "Starting...");
  Serial.println(lcdReady ? "LCD: OK" : "LCD: NOT FOUND");

  // WiFi with timeout
  Serial.print("WiFi: Connecting");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    digitalWrite(RED_LED_PIN, !digitalRead(RED_LED_PIN));
    attempts++;
  }
  digitalWrite(RED_LED_PIN, LOW);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi: FAILED");
    lcdShow("WiFi Failed!", "Check settings");
    flashError();
    return;
  }

  Serial.print("\nWiFi: OK | IP: ");
  Serial.println(WiFi.localIP());

  // Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize session in Firebase
  if (Firebase.ready()) {
    Firebase.RTDB.setString(&fbdo, "/atm/session/action", "idle");
    Firebase.RTDB.setBool(&fbdo, "/atm/session/active", false);
    Serial.println("Firebase: OK");
  } else {
    Serial.println("Firebase: FAILED");
  }

  // Web server
  server.begin();
  Serial.println("Server: OK");

  // Ready
  flashSuccess();
  lcdShowWelcome();
  Serial.println("\n=== ATM Ready ===\n");
}

// ============================
//    LOOP
// ============================

void loop() {
  server.handleClient();
  checkFirebase();
}