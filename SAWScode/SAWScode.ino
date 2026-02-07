/*
 * Project: SAWS - V37 (FINAL CLEAN EDITION)
 * -------------------------------------
 * Author : Felix P - DautzeJAVA
 * -------------------------------------
 * List of components :
   - Arduino Pro Mini 5V
   - Breadboard
   - BME 680 (Weather sensor) -> Powered via PIN 3 (ALWAYS ON when Awake)
   - GUVA S12SD (UV sensor)
   - B10K Potentiometer
   - TP4056 (Charging)
   - MT3608 (Boost Converter)
   - Li-ion battery (Salvaged from JBL Case)
   - LCD screen : 1602A
   - JOYSTICK (Analog)
   - Resistors 
   - Solar panel (3W 6V)
   - Plastic case
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

// Pin 4 powers: LCD Logic, UV Sensor, Joystick
const int PIN_PWR_SENSORS = 4;

// Pin 3 powers: BME680 ONLY (Continuous Power when Awake)
const int PIN_PWR_BME     = 3;

// Pin 10 powers: LCD Backlight (High current)
const int PIN_PWR_LIGHT   = 10; 


// --- LCD PINS ---
const int PIN_RS = 5; 
const int PIN_E  = 6;
const int PIN_D4 = 7; 
const int PIN_D5 = 9;
const int PIN_D6 = 11; 
const int PIN_D7 = 12;


// --- SENSORS & INPUTS ---
const int PIN_JOY_X    = A0; // Menu Navigation (Left/Right)
const int PIN_UV       = A1; // UV Sensor Output
const int PIN_BAT      = A2; // Battery Voltage Divider
const int PIN_JOY_Y    = A3; // Language Elevator (Up/Down)

const int PIN_WAKE_BTN = 2;  // Wake Up Interrupt Button
const int PIN_UNIT_BTN = 8;  // Unit Toggle Button

// ==========================================
// 2. OBJECTS & VARIABLES
// ==========================================

// Initialize Objects
LiquidCrystal lcd(PIN_RS, PIN_E, PIN_D4, PIN_D5, PIN_D6, PIN_D7);
Adafruit_BME680 bme; 

// --- TIMERS ---
const long UI_TIMEOUT = 10000; // 10 seconds before sleep

unsigned long lastActivity = 0; 
unsigned long lastSensorTime = 0;
unsigned long lastLcdTime = 0;
bool isAwake = true; 

// --- SENSOR DATA VARIABLES ---
float g_temp  = 0.0; 
float g_press = 0.0; 
int   g_hum   = 0; 
float g_gas   = 0.0;
int   g_uv    = 0; 
int   g_batt  = 0; 
float g_volt  = 0.0;
int   g_iaq   = 0;

// --- MENU STATE ---
int currentMenu = 0; 
int lastMenu = -1;

// --- LANGUAGES (ELEVATOR LOGIC) ---
//  1 = Danish (UP)
//  0 = English (CENTER/DEFAULT)
// -1 = French (DOWN)
int currentLangLevel = 0; 
int lastLangLevel = -2; 

// --- SETTINGS ---
int unitTemp = 0;   // 0=C, 1=F, 2=K
int unitPress = 0;  // 0=hPa, 1=atm
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
  // 1. Turn on General Power Rails
  digitalWrite(PIN_PWR_SENSORS, HIGH);
  digitalWrite(PIN_PWR_LIGHT, HIGH);
  
  // 2. Turn on BME680 (Exclusive Rail)
  digitalWrite(PIN_PWR_BME, HIGH);
  
  // 3. Wait for hardware stability
  delay(150); 
  
  // 4. Initialize Screen
  lcd.begin(16, 2);
  lcd.createChar(0, pBar); 
  lcd.createChar(1, degree); 
  
  // 5. Initialize BME Sensor
  if (!bme.begin(0x77)) {
    bme.begin(0x76);
  }
  
  // 6. Configure BME680 settings
  bme.setTemperatureOversampling(BME680_OS_2X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_2X);
  bme.setGasHeater(320, 150);
}
void powerOFF() {
  
  // 1. Cut Power to High-Drain Peripherals (Backlight & Logic)
  digitalWrite(PIN_PWR_LIGHT, LOW);   
  digitalWrite(PIN_PWR_SENSORS, LOW); 
  
  // 2. IMPORTANT: KEEP BME680 POWERED!
  // We keep Pin 3 HIGH to maintain thermal stability and calibration data.
  // The sensor automatically enters Deep Sleep (0.15uA) when not reading.
  digitalWrite(PIN_PWR_BME, HIGH); 
  
  // 3. Set LCD data pins to LOW to prevent current leaks
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
  
  // --- DANISH (Level 1) ---
  if (currentLangLevel == 1) { 
    if (iaq <= 50)  return "Fremragende";
    if (iaq <= 100) return "Godt";
    if (iaq <= 150) return "Forurenet";
    if (iaq <= 200) return "Darligt";
    return "Giftigt!";
  }
  
  // --- FRENCH (Level -1) ---
  else if (currentLangLevel == -1) { 
    if (iaq <= 50)  return "Excellent";
    if (iaq <= 100) return "Bon";
    if (iaq <= 150) return "Pollue";
    if (iaq <= 200) return "Mauvais";
    return "Toxique!";
  }
  
  // --- ENGLISH (Level 0) ---
  else { 
    if (iaq <= 50)  return "Excellent";
    if (iaq <= 100) return "Good";
    if (iaq <= 150) return "Polluted";
    if (iaq <= 200) return "Bad";
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
  pinMode(PIN_PWR_BME, OUTPUT); // Pin 3
  
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
  
  // Line 1: English
  lcd.print("Hi DTU :)"); 
  
  // Line 2: Danish
  lcd.setCursor(0, 1);
  lcd.print("Hej DTU :)"); 
  
  delay(2000); // Display for 2 seconds
  
  // Reset Timer
  lastActivity = millis();
}

// ==========================================
// 6. MAIN LOOP
// ==========================================

void loop() {
  
  if (isAwake) {
    // -------------------------
    // MODE: ACTIVE
    // -------------------------
    
    // SAFETY CHECK: Force Power ON for BME
    // This ensures Pin 3 NEVER drops while awake
    digitalWrite(PIN_PWR_BME, HIGH); 
    digitalWrite(PIN_PWR_SENSORS, HIGH);
    digitalWrite(PIN_PWR_LIGHT, HIGH);

    unsigned long now = millis();

    // -------------------------
    // A. BUTTON HANDLING (UNITS)
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
      
      // --- X-AXIS (MENU NAVIGATION) ---
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
      
      // --- Y-AXIS (LANGUAGE ELEVATOR) ---
      
      // UP MOVEMENT (Going UP)
      else if (jy < 200) { 
        if (currentLangLevel < 1) {
           
           // Determine Transition Text
           lcd.clear();
           
           if(currentLangLevel == -1) {
              // French -> English
              lcd.print("Back to English");
              lcd.setCursor(0,1);
              lcd.print("Mode: English");
           } else {
              // English -> Danish
              lcd.print("Initializing DK");
              lcd.setCursor(0,1);
              lcd.print("Dansk Init...");
           }
           
           delay(1500); // Visual effect
           
           currentLangLevel++; // GO UP
           joyMoved = true; 
           lastActivity = now; 
           lastMenu = -1; 
           lcd.clear();
        }
      }
      
      // DOWN MOVEMENT (Going DOWN)
      else if (jy > 800) { 
        if (currentLangLevel > -1) {
           
           // Determine Transition Text
           lcd.clear();
           
           if(currentLangLevel == 1) {
             // Danish -> English
             lcd.print("Back to English");
             lcd.setCursor(0,1);
             lcd.print("Mode: English");
           } else {
             // English -> French
             lcd.print("Initializing FR");
             lcd.setCursor(0,1);
             lcd.print("Init. Francais");
           }
           
           delay(1500); // Visual effect

           currentLangLevel--; // GO DOWN
           joyMoved = true; 
           lastActivity = now; 
           lastMenu = -1; 
           lcd.clear();
        }
      }
    }

    // Reset Joystick Deadzone
    if (jx > 300 && jx < 700 && jy > 300 && jy < 700) {
      joyMoved = false;
    }

    // -------------------------
    // C. SENSOR READING (Every 3s)
    // -------------------------
    if (now - lastSensorTime > 3000) {
      lastSensorTime = now;
      
      // Read BME680
      bme.performReading();
      g_temp  = bme.temperature; 
      g_hum   = bme.humidity;
      g_press = bme.pressure / 100.0; 
      g_gas   = bme.gas_resistance / 1000.0;
      g_iaq   = calculateIAQ(g_gas);
      
      // Read Analog Sensors
      g_uv   = (int)(analogRead(PIN_UV) * (5.0/1023.0) * 10.0);
      g_volt = analogRead(PIN_BAT) * (5.0/1023.0);
      
      // Calculate Battery Percentage
      g_batt = map((long)(g_volt*100), 340, 420, 0, 100);
      g_batt = constrain(g_batt, 0, 100);
    }

    // -------------------------
    // D. DISPLAY LOGIC (Every 200ms)
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
              lcd.print("1-Temperatur DK ");
            else if(currentLangLevel == -1) 
              lcd.print("1-Temperature FR");
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
        case 0: // Dashboard
          lcd.print("T:"); 
          lcd.print((int)g_temp); 
          lcd.print(" H:"); 
          lcd.print(g_hum); 
          lcd.print("% B:"); 
          lcd.print(g_batt); 
          lcd.print("%"); 
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
          
        case 2: // Humidity
          lcd.print(g_hum); 
          lcd.print(" % RH"); 
          break;
          
        case 3: // Pressure
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
          lcd.print("Idx:"); 
          lcd.print(g_iaq); 
          lcd.print(" "); 
          lcd.print(getIAQText(g_iaq)); 
          break;
          
        case 5: // UV
          lcd.print("Idx:"); 
          lcd.print(g_uv); 
          if(g_uv < 3) {
            lcd.print(" Low"); 
          }
          else if(g_uv < 6) {
            lcd.print(" Mod"); 
          }
          else {
            lcd.print(" High"); 
          }
          break;
          
        case 6: // Battery
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
    
    // 1. Cut Power
    powerOFF();
    
    // 2. Enable Wake Interrupt
    attachInterrupt(digitalPinToInterrupt(PIN_WAKE_BTN), wakeUpRoutine, LOW);
    
    // 3. Enter Sleep
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    
    // 4. Resume here after button press
    detachInterrupt(digitalPinToInterrupt(PIN_WAKE_BTN));
    
    // -------------------------
    // MODE: WAKE UP
    // -------------------------
    
    // 5. Restore Power
    powerON();
    lcd.clear(); 
    
    // 6. Wake Up Message (Bilingual)
    lcd.print("Hi DTU :)"); 
    
    lcd.setCursor(0, 1);
    lcd.print("Hej DTU :)"); 
    
    delay(1000); 
    lcd.clear();
    
    // 7. Reset Variables
    currentMenu = 0; 
    lastMenu = -1; 
    isAwake = true; 
    lastActivity = millis(); 
    lastLcdTime = 0;
  }
}