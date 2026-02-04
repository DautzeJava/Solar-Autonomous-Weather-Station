/*
 * Project: SAWS - V35c (CLEAN EDITION)
 * -------------------------------------
 * Author : Felix P - DautzeJAVA
 * -------------------------------------
 * List of components :
  -Arduino Pro Mini 5V
  -Breadboard
  -BME 680 (Weather sensor)
  -GUVA S12SD (UV sensor)
  -B10K
  -TP4056
  -MT3608
  -Li-ion battery from JBL WAVE BEAM case
  -LCD screen : 1602A
  -JOYSTICK
  -Resistors 
  -Solar panel (3W 6V)
  -A plastic case
 */

#include <LiquidCrystal.h>
#include <LowPower.h>
#include <Wire.h>               
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// ==========================================
// 1. PIN CONFIGURATION
// ==========================================

// --- POWER MANAGEMENT ---
const int PIN_PWR_SENSORS = 4;
const int PIN_PWR_LIGHT   = 10; 

// --- LCD PINS ---
const int PIN_RS = 5; 
const int PIN_E  = 6;
const int PIN_D4 = 7; 
const int PIN_D5 = 9;
const int PIN_D6 = 11; 
const int PIN_D7 = 12;

// --- SENSORS & INPUTS ---
const int PIN_JOY_X    = A0; // Menu Navigation
const int PIN_UV       = A1; // UV Sensor
const int PIN_BAT      = A2; // Battery Measure
const int PIN_JOY_Y    = A3; // Language Elevator

const int PIN_WAKE_BTN = 2;  // Wake Up Interrupt
const int PIN_UNIT_BTN = 8;  // Unit Toggle

// ==========================================
// 2. OBJECTS & VARIABLES
// ==========================================

// Initialize Objects
LiquidCrystal lcd(PIN_RS, PIN_E, PIN_D4, PIN_D5, PIN_D6, PIN_D7);
Adafruit_BME680 bme; 

// --- TIMERS ---
const long UI_TIMEOUT = 8000; 
unsigned long lastActivity = 0; 
unsigned long lastSensorTime = 0;
unsigned long lastLcdTime = 0;
bool isAwake = true; 

// --- SENSOR DATA ---
float g_temp = 0.0; 
float g_press = 0.0; 
int g_hum = 0; 
float g_gas = 0.0;
int g_uv = 0; 
int g_batt = 0; 
float g_volt = 0.0;
int g_iaq = 0;

// --- MENU STATE ---
int currentMenu = 0; 
int lastMenu = -1;

// --- LANGUAGES (ELEVATOR) ---
// 1 = Danish (UP)
// 0 = English (CENTER/DEFAULT)
// -1 = French (DOWN)
int currentLangLevel = 0; 
int lastLangLevel = -2; 

// --- SETTINGS ---
int unitTemp = 0; 
int unitPress = 0; 
bool joyMoved = false; 
int lastBtnState = HIGH;

// --- CUSTOM ICONS ---
byte degree[8] = {
  B01100, B10010, B01100, B00000, B00000, B00000, B00000, B00000
};
byte pBar[8] = {
  B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111
};

// ==========================================
// 3. POWER FUNCTIONS
// ==========================================

void powerON() {
  // 1. Turn on electricity
  digitalWrite(PIN_PWR_SENSORS, HIGH);
  digitalWrite(PIN_PWR_LIGHT, HIGH);
  
  // 2. Wait for hardware to stabilize
  delay(150); 
  
  // 3. Restart Screen & Sensors
  lcd.begin(16, 2);
  lcd.createChar(0, pBar); 
  lcd.createChar(1, degree); 
  
  if (!bme.begin(0x77)) {
    bme.begin(0x76);
  }
  
  // 4. Configure BME680
  bme.setTemperatureOversampling(BME680_OS_2X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_2X);
  bme.setGasHeater(320, 150);
}

void powerOFF() {
  // Turn everything off cleanly
  digitalWrite(PIN_PWR_LIGHT, LOW);   
  digitalWrite(PIN_PWR_SENSORS, LOW); 
  
  // Set data pins to LOW to avoid leaks
  digitalWrite(PIN_RS, LOW); 
  digitalWrite(PIN_E, LOW);
  digitalWrite(PIN_D4, LOW); 
  digitalWrite(PIN_D5, LOW);
  digitalWrite(PIN_D6, LOW); 
  digitalWrite(PIN_D7, LOW);
}

void wakeUpRoutine() {
  // Empty interrupt handler
}

// ==========================================
// 4. HELPER FUNCTIONS
// ==========================================

int calculateIAQ(float r_kohm) {
  float r_clean = 50.0; 
  float r_bad = 5.0;    
  float r_c = constrain(r_kohm, r_bad, r_clean);
  return map((long)(r_c * 100), (long)(r_bad * 100), (long)(r_clean * 100), 500, 25);
}

String getIAQText(int iaq) {
  // DANISH
  if (currentLangLevel == 1) { 
    if (iaq <= 50) 
      return "Fremragende";
    if (iaq <= 100) 
      return "Godt";
    if (iaq <= 150) 
      return "Forurenet";
    if (iaq <= 200) 
      return "Darligt";
    return "Giftigt!";
  }
  // FRENCH
  else if (currentLangLevel == -1) { 
    if (iaq <= 50) 
      return "Excellent";
    if (iaq <= 100) 
      return "Bon";
    if (iaq <= 150)  
      return "Pollue";
    if (iaq <= 200) 
      return "Mauvais";
    return "Toxique!";
  }
  // ENGLISH
  else { 
    if (iaq <= 50) 
      return "Excellent";
    if (iaq <= 100) 
      return "Good";
    if (iaq <= 150) 
      return "Polluted";
    if (iaq <= 200) 
      return "Bad";
    return "Toxic!";
  }
}

// ==========================================
// 5. SETUP
// ==========================================

void setup() {
  // Configure Output Pins
  pinMode(PIN_PWR_SENSORS, OUTPUT); 
  pinMode(PIN_PWR_LIGHT, OUTPUT);
  pinMode(PIN_RS, OUTPUT); 
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_D4, OUTPUT); 
  pinMode(PIN_D5, OUTPUT);
  pinMode(PIN_D6, OUTPUT); 
  pinMode(PIN_D7, OUTPUT);

  // Configure Input Pins
  pinMode(PIN_UNIT_BTN, INPUT_PULLUP); 
  pinMode(PIN_WAKE_BTN, INPUT_PULLUP);

  // Start System
  powerON();
  lcd.clear(); 
  
  // --- STARTUP MESSAGE FOR DTU ---
  lcd.print("SAWS Init..."); 
  lcd.setCursor(0, 1);
  lcd.print("Hi DTU"); // Message Spécial
  
  delay(2000); // Display for 2 seconds
  
  // Reset Timer
  lastActivity = millis();
}

// ==========================================
// 6. MAIN LOOP
// ==========================================

void loop() {
  
  if (isAwake) {
    // Keep power ON
    digitalWrite(PIN_PWR_SENSORS, HIGH);
    digitalWrite(PIN_PWR_LIGHT, HIGH);
    unsigned long now = millis();

    // -------------------------
    // A. BUTTON HANDLING
    // -------------------------
    int btn = digitalRead(PIN_UNIT_BTN);
    
    if (btn == LOW && lastBtnState == HIGH) {
      if(currentMenu == 1) {
        unitTemp = (unitTemp + 1) % 3;
      }
      if(currentMenu == 3) {
        unitPress = (unitPress + 1) % 2;
      }
      lastActivity = now; 
      lcd.clear(); 
      lastMenu = -1;
    }
    lastBtnState = btn;

    // -------------------------
    // B. JOYSTICK HANDLING
    // -------------------------
    int jx = analogRead(PIN_JOY_X);
    int jy = analogRead(PIN_JOY_Y);

    if (!joyMoved) {
      // X-AXIS (Menu Navigation)
      if (jx > 800) { 
        currentMenu--; 
        if(currentMenu < 0) currentMenu = 6; 
        joyMoved = true; 
        lastActivity = now; 
        lastMenu = -1; 
        lcd.clear();
      }
      else if (jx < 200) { 
        currentMenu++; 
        if(currentMenu > 6) currentMenu = 0; 
        joyMoved = true; 
        lastActivity = now; 
        lastMenu = -1; 
        lcd.clear();
      }
      
      // Y-AXIS (Language Elevator)
      // UP -> DANISH
      else if (jy < 200) { 
        if (currentLangLevel < 1) {
           currentLangLevel++;
           joyMoved = true; 
           lastActivity = now; 
           lastMenu = -1; 
           lcd.clear();
        }
      }
      // DOWN -> FRENCH
      else if (jy > 800) { 
        if (currentLangLevel > -1) {
           currentLangLevel--;
           joyMoved = true; 
           lastActivity = now; 
           lastMenu = -1; 
           lcd.clear();
        }
      }
    }

    // Reset Deadzone
    if (jx > 300 && jx < 700 && jy > 300 && jy < 700) {
      joyMoved = false;
    }

    // -------------------------
    // C. SENSOR READING
    // -------------------------
    if (now - lastSensorTime > 3000) {
      lastSensorTime = now;
      
      // Read BME680
      bme.performReading();
      g_temp = bme.temperature; 
      g_hum = bme.humidity;
      g_press = bme.pressure / 100.0; 
      g_gas = bme.gas_resistance / 1000.0;
      g_iaq = calculateIAQ(g_gas);
      
      // Read Analog
      g_uv = (int)(analogRead(PIN_UV) * (5.0/1023.0) * 10.0);
      g_volt = analogRead(PIN_BAT) * (5.0/1023.0);
      
      // Calculate Battery %
      g_batt = map((long)(g_volt*100), 340, 420, 0, 100);
      g_batt = constrain(g_batt, 0, 100);
    }

    // -------------------------
    // D. DISPLAY LOGIC
    // -------------------------
    if (now - lastLcdTime > 200 || currentMenu != lastMenu || currentLangLevel != lastLangLevel) {
      lastLcdTime = now;
      lastLangLevel = currentLangLevel;

      // 1. MENU TITLES (LINE 1)
      if (currentMenu != lastMenu || currentLangLevel != lastLangLevel) {
        lastMenu = currentMenu; 
        lcd.setCursor(0,0);
        
        switch(currentMenu) {
          case 0: 
            lcd.print("<< Dashboard >> "); 
            break;
            
          case 1: 
            if(currentLangLevel == 1)      
              lcd.print("1-Temperatur DK "); // 16 chars EXACT
            else if(currentLangLevel == -1) 
              lcd.print("1-Temperature FR"); // 16 chars EXACT
            else                            
              lcd.print("1-Temperature* ");
            break;
            
          case 2: 
            if(currentLangLevel == 1)      
              lcd.print("2-Luftfugt (DK) ");
            else if(currentLangLevel == -1) 
              lcd.print("2-Humidite (FR) ");
            else                            
              lcd.print("2-Humidity      ");
            break;
            
          case 3: 
            if(currentLangLevel == 1)      
              lcd.print("3-Lufttryk (DK) ");
            else if(currentLangLevel == -1) 
              lcd.print("3-Pression (FR) ");
            else                            
              lcd.print("3-Pressure* ");
            break;
            
          case 4: 
            if(currentLangLevel == 1)      
              lcd.print("4-Luftkval (DK) ");
            else if(currentLangLevel == -1) 
              lcd.print("4-Qual. Air(FR) ");
            else                            
              lcd.print("4-IAQ Index     ");
            break;
            
          case 5: 
            if(currentLangLevel == 1)      
              lcd.print("5-UV-indeks (DK)");
            else if(currentLangLevel == -1) 
              lcd.print("5-Index UV (FR) ");
            else                            
              lcd.print("5-UV Index      ");
            break;
            
          case 6: 
            if(currentLangLevel == 1)      
              lcd.print("6-Batteri (DK)  ");
            else if(currentLangLevel == -1) 
              lcd.print("6-Batterie (FR) ");
            else                            
              lcd.print("6-Battery       ");
            break;
        }
      }
      
      // 2. SENSOR DATA (LINE 2)
      lcd.setCursor(0,1);
      switch(currentMenu) {
        case 0: 
          lcd.print("T:"); lcd.print((int)g_temp); 
          lcd.print(" H:"); lcd.print(g_hum); 
          lcd.print("% B:"); lcd.print(g_batt); lcd.print("%"); 
          break;
        
        case 1: // Temp
          if(unitTemp == 0) { 
            lcd.print(g_temp,1); 
            lcd.write(byte(1)); 
            lcd.print("C   "); 
          } 
          else if(unitTemp == 1) { 
            lcd.print((g_temp*1.8)+32,1); 
            lcd.write(byte(1)); 
            lcd.print("F   "); 
          } 
          else { 
            lcd.print(g_temp+273.15,1); 
            lcd.print(" K   "); 
          } 
          break;
          
        case 2: // Hum
          lcd.print(g_hum); 
          lcd.print(" % RH"); 
          break;
          
        case 3: // Press
          if(unitPress == 0){
            lcd.print((int)g_press); 
            lcd.print(" hPa");
          } 
          else {
            lcd.print(g_press/1013.25,3); 
            lcd.print(" atm");
          } 
          break;
          
        case 4: // IAQ
          lcd.print("Idx:"); lcd.print(g_iaq); 
          lcd.print(" "); 
          lcd.print(getIAQText(g_iaq)); 
          break;
          
        case 5: // UV
          lcd.print("Idx:"); 
          lcd.print(g_uv); 
          if(g_uv < 3) lcd.print(" Low"); 
          else if(g_uv < 6) lcd.print(" Mod"); 
          else lcd.print(" High"); 
          break;
          
        case 6: // Bat
          lcd.print(g_volt,2); 
          lcd.print("V ("); 
          lcd.print(g_batt); 
          lcd.print("%)"); 
          break;
      }
    }

    // -------------------------
    // E. SLEEP CHECK
    // -------------------------
    if (now - lastActivity > UI_TIMEOUT) {
      isAwake = false; 
      lcd.clear();
    }

  } else {
    // -------------------------
    // MODE: DEEP SLEEP
    // -------------------------
    powerOFF();
    
    attachInterrupt(digitalPinToInterrupt(PIN_WAKE_BTN), wakeUpRoutine, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(digitalPinToInterrupt(PIN_WAKE_BTN));
    
    // -------------------------
    // MODE: WAKE UP
    // -------------------------
    powerON();
    lcd.clear(); 
    
    // Greeting based on Language Level
    if(currentLangLevel == 1) {
      lcd.print("Godmorgen DTU"); // Danois
    }
    else if(currentLangLevel == -1) {
      lcd.print("Bonjour !");    // Français
    }
    else {
      lcd.print("Hi DTU");      // Anglais
    }
    
    delay(1000); 
    lcd.clear();
    
    // Reset but keep Language Preference
    currentMenu = 0; 
    lastMenu = -1;
    isAwake = true; 
    lastActivity = millis(); 
    lastLcdTime = 0;
  }
}