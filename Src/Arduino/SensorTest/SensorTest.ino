#include <LiquidCrystal.h>

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(2, 7, 8, 13, A2, A3);

// Motor Driver Pins
const int ENA = 9;
const int IN1 = 3;
const int IN2 = 4;
const int ENB = 10;
const int IN3 = 5;
const int IN4 = 6;

const int TRIG_PIN = 12;
const int ECHO_PIN = 11;

// Parameters - ADJUST THESE FOR YOUR CAR
const int OBSTACLE_CM = 20;       // React earlier
const int DRIVE_PWM = 200;
const int TURN_90_TIME = 450;     // Time to turn 90 degrees (adjust this!)
const int TURN_180_TIME = 900;    // Time to turn 180 degrees (2x of 90)

// LCD timing
unsigned long lastLcdUpdate = 0;
const int LCD_UPDATE_INTERVAL = 300;  // Update LCD every 300ms

long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long us = pulseIn(ECHO_PIN, HIGH, 20000UL);  // Shorter timeout
  
  if (us == 0) return 999;
  
  long cm = us / 58;
  return (cm >= 2 && cm <= 200) ? cm : 999;
}

void setMotors(int leftSpeed, int rightSpeed) {
  // Clamp values
  if (leftSpeed > 255) leftSpeed = 255;
  if (leftSpeed < -255) leftSpeed = -255;
  if (rightSpeed > 255) rightSpeed = 255;
  if (rightSpeed < -255) rightSpeed = -255;

  // Left motor
  if (leftSpeed >= 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, leftSpeed);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, -leftSpeed);
  }

  // Right motor
  if (rightSpeed >= 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, rightSpeed);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, -rightSpeed);
  }
}

void moveForward() {
  setMotors(DRIVE_PWM, DRIVE_PWM);
}

void stopMotors() {
  setMotors(0, 0);
}

// Pivot right 90 degrees (in place)
void pivotRight90() {
  setMotors(DRIVE_PWM, -DRIVE_PWM);
  delay(TURN_90_TIME);
  stopMotors();
}

// Pivot left 90 degrees (in place)
void pivotLeft90() {
  setMotors(-DRIVE_PWM, DRIVE_PWM);
  delay(TURN_90_TIME);
  stopMotors();
}

// Spin 180 degrees (in place)
void spin180() {
  setMotors(DRIVE_PWM, -DRIVE_PWM);
  delay(TURN_180_TIME);
  stopMotors();
}

// Check right: pivot right, measure, pivot back
long checkRight() {
  pivotRight90();
  delay(100);
  long dist = readDistanceCm();
  pivotLeft90();  // Return to original direction
  delay(50);
  return dist;
}

// Check left: pivot left, measure, pivot back
long checkLeft() {
  pivotLeft90();
  delay(100);
  long dist = readDistanceCm();
  pivotRight90();  // Return to original direction
  delay(50);
  return dist;
}

// Turn right and continue (don't turn back)
void turnRight() {
  pivotRight90();
}

// Turn left and continue (don't turn back)
void turnLeft() {
  pivotLeft90();
}

void updateLcd(long fwd, long rightD, long leftD, const char* action) {
  // Only update every LCD_UPDATE_INTERVAL ms
  if (millis() - lastLcdUpdate < LCD_UPDATE_INTERVAL) {
    return;
  }
  lastLcdUpdate = millis();
  
  // Line 1
  lcd.setCursor(0, 0);
  lcd.print("F:");
  lcd.print(fwd);
  lcd.print(" R:");
  lcd.print(rightD);
  lcd.print("    ");

  // Line 2
  lcd.setCursor(0, 1);
  lcd.print("L:");
  lcd.print(leftD);
  lcd.print(" ");
  lcd.print(action);
  lcd.print("      ");
}

void setup() {
  Serial.begin(9600);
  Serial.println("=== Obstacle Robot ===");
  
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Obstacle Robot");
  lcd.setCursor(0, 1);
  lcd.print("Ready!");
  
  stopMotors();
  delay(1000);
  lcd.clear();
}

void loop() {
  long fwd = readDistanceCm();
  long rightD = 999;
  long leftD = 999;

  // Forward is clear - keep moving
  if (fwd > OBSTACLE_CM) {
    moveForward();
    updateLcd(fwd, rightD, leftD, "FWD");
  } 
  else {
    // Forward blocked - stop immediately
    stopMotors();
    
    Serial.println("FWD blocked - checking right");
    rightD = checkRight();
    
    if (rightD > OBSTACLE_CM) {
      // Right is open - turn right 90°
      Serial.println("Right open - turning right");
      updateLcd(fwd, rightD, leftD, "RIGHT");
      turnRight();
    } 
    else {
      // Right blocked - check left
      Serial.println("Right blocked - checking left");
      leftD = checkLeft();
      
      if (leftD > OBSTACLE_CM) {
        // Left is open - turn left 90°
        Serial.println("Left open - turning left");
        updateLcd(fwd, rightD, leftD, "LEFT");
        turnLeft();
      } 
      else {
        // All blocked - spin 180°
        Serial.println("All blocked - spinning 180");
        updateLcd(fwd, rightD, leftD, "BACK");
        spin180();
      }
    }
  }

  delay(100);  // Small delay for stability
}