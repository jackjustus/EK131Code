#include <Wire.h> 
#include <LiquidCrystal_I2C.h>


// USER CONFIG
int tempAlarmThresholdF = 80;
bool DEBUG_ENABLED = false;

// DEVICE SETUP
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
int buzzerFrequency = 500;        // Current frequency of the buzzer
int buzzerStep = 10;              // Step size for frequency change
bool buzzerIncreasing = true;     // Direction of frequency change
unsigned long buzzerPrevMillis = 0; // Stores last update time
const int buzzerInterval = 10;    // Time interval between frequency changes (in ms)
unsigned long ledPrevMillis = 0;
const int ledInterval = 200;  // LED blink interval (ms)
bool ledState = false;

// PIN SETUP
#define TMP_36_PIN A0
#define BUZZER_PIN 6
#define LED_PIN 7

// GENERAL SETUP 
String status = "INITAlIZING";


void setup()
{
  // Initialize the lcd 
  lcd.init();                      
  lcd.noBacklight();
  

  // Init buzzer
  pinMode(BUZZER_PIN, OUTPUT); // Set the buzzer pin as an output
  pinMode(LED_PIN, OUTPUT); // Set LED pin as output

  Serial.begin(9600);
  Serial.println("System Initialized. Type 'DEBUG ON' or 'DEBUG OFF' to toggle debug mode.");
}


void loop()
{
  checkSerialCommand(); // Check for user input to enable/disable debug mode

  // Calculate Temperature Values
  int tmp36reading = analogRead(TMP_36_PIN);  
  // Convert ADC reading (0–1023) to voltage (0–5V)
  float tmp36voltage = tmp36reading * (5.0 / 1023.0);

  Serial.print("Voltage: ");
  Serial.println(tmp36voltage, 3);   // print with 3 decimal places

  // TMP36: 0.5 V @ 0 °C, 10 mV/°C
  float tmp36tempC = (tmp36voltage - 0.5) * 100.0;
  Serial.print("Temperature (°C): ");
  Serial.println(tmp36tempC, 1);     // print with 1 decimal place

  // Convert to °F
  float tmp36tempF = tmp36tempC * 9.0 / 5.0 + 32.0;
  Serial.print("Temperature (°F): ");
  Serial.println(tmp36tempF, 1);

  Serial.print("Temp: ");
  Serial.println(tmp36tempF);

  bool alarmActive = false;

  // Debug functionality
  if (!DEBUG_ENABLED) {
    // Check alarm status to determine behavior
    if (isTemperatureOverThreshold(tmp36tempF)) {
      // ALARM ON
      alarmActive = true;
      status = "WARNING";
    } else {
      // ALARM OFF
      alarmActive = false;
      status = "NOMINAL";
    }
  } else {
    // DEBUG FUNCTIONALITY
    status = "DEBUG";
    alarmActive = true;
  }

  // Handle Alarm
  updateBuzzerTone(alarmActive);
  updateLED(alarmActive);

  updateLCD(status, tmp36tempF, alarmActive);
}

bool isTemperatureOverThreshold(float tmp36tempF) {
  if (tmp36tempF < tempAlarmThresholdF || tmp36tempF > 90.0) {
    return true;
  }
  return false;
}

void updateLCD(String status, float tempF, bool alarmActive) {
  static unsigned long previousBlinkMillis = 0;
  static unsigned long previousClearMillis = 0;
  const unsigned long blinkInterval = 500;  // Blink interval in milliseconds
  const unsigned long clearInterval = 250;  // Clear interval in milliseconds
  static bool backlightState = true;
  unsigned long currentMillis = millis();

  // Clear and reset display at set intervals
  if (currentMillis - previousClearMillis >= clearInterval) {
    previousClearMillis = currentMillis;
    lcd.clear();

    // Print a message to the LCD.
    lcd.setCursor(0, 0); // First line
    lcd.print("Status: ");
    lcd.print(status);

    lcd.setCursor(0, 1); // Second line
    lcd.print("Temp=");
    lcd.print(tempF);
    lcd.print("*F");
    // Serial.println("clearing");
  }



  // Handle backlight blinking if alarm is active
  if (alarmActive) {
    if (currentMillis - previousBlinkMillis >= blinkInterval) {
      previousBlinkMillis = currentMillis;
      backlightState = !backlightState;

      if (backlightState) {
        lcd.backlight();  // Turn on backlight
      } else {
        lcd.noBacklight(); // Turn off backlight
      }
    }
  } else {
    lcd.backlight(); // Keep backlight on when no alarm
  }
}

void updateBuzzerTone(bool buzzerEnabled) {

  if (buzzerEnabled) {
    unsigned long buzzerCurrentMillis = millis();

    if (buzzerCurrentMillis - buzzerPrevMillis >= buzzerInterval) {
        buzzerPrevMillis = buzzerCurrentMillis;

        // Set the buzzer frequency
        tone(BUZZER_PIN, buzzerFrequency);

        // Update frequency for next cycle
        if (buzzerIncreasing) {
            buzzerFrequency += buzzerStep;
            if (buzzerFrequency >= 2000) {
                buzzerIncreasing = false;  // Start decreasing
            }
        } else {
            buzzerFrequency -= buzzerStep;
            if (buzzerFrequency <= 500) {
                buzzerIncreasing = true;  // Start increasing
            }
        }
    }
  } else {
    // Buzzer not enabled
    noTone(BUZZER_PIN);
  }
}

// Function to check Serial input and toggle DEBUG mode
void checkSerialCommand() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove whitespace

    if (command.equalsIgnoreCase("DEBUG ON")) {
      DEBUG_ENABLED = true;
      Serial.println("Debug Mode Enabled.");
    } 
    else if (command.equalsIgnoreCase("DEBUG OFF")) {
      DEBUG_ENABLED = false;
      Serial.println("Debug Mode Disabled.");
    }
  }
}

// LED Blinking Function (Non-blocking)
void updateLED(bool alarmActive) {
  unsigned long ledCurrentMillis = millis();

  if (alarmActive) {
    if (ledCurrentMillis - ledPrevMillis >= ledInterval) {
      ledPrevMillis = ledCurrentMillis;
      ledState = !ledState;  // Toggle LED state
      digitalWrite(LED_PIN, ledState);
    }
  } else {
    digitalWrite(LED_PIN, LOW);  // Keep LED off if no alarm
  }
}