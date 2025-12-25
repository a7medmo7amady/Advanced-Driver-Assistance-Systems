/***************** PIN DEFINITIONS *****************/

// L298N - Left side motors
#define ENA 5
#define IN1 6
#define IN2 7

// L298N - Right side motors
#define ENB 9
#define IN3 10
#define IN4 11

// Ultrasonic HC-SR04
#define TRIG_PIN 2
#define ECHO_PIN 3

/***************** MOTION PARAMETERS *****************/

#define SPEED_FWD      180
#define SPEED_TURN     160

#define OBSTACLE_CM    20          // < 20 cm => direction "closed"
#define ROTATE_TIME_MS 400         // ~90° turn (tune this)
#define SPIN_TIME_MS   (ROTATE_TIME_MS * 4) // ~360° spin (tune)
#define MOVE_STEP_MS   300         // how long to move forward per decision

#define BLOCK_TIME_MS  10000UL      // remember blocked directions for 10s

/***************** DIRECTION MEMORY *****************/

enum DirIndex {
  DIR_FWD = 0,
  DIR_LEFT = 1,
  DIR_RIGHT = 2
};

struct DirBlock {
  bool blocked;
  unsigned long until_ms;
};

DirBlock dirBlock[3] = {
  { false, 0 },
  { false, 0 },
  { false, 0 }
};

/***************** MOTOR CONTROL *****************/

void setLeft(int dir, int speed) {
  // dir: +1 forward, -1 backward, 0 stop
  if (dir > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (dir < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    speed = 0;
  }
  analogWrite(ENA, speed);
}
// Moving to the Left Dir
void setRight(int dir, int speed) {
  if (dir > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (dir < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    speed = 0;
  }
  analogWrite(ENB, speed);
}

void moveForward(uint8_t speed)  { setLeft(+1, speed); setRight(+1, speed); }
void turnLeft(uint8_t speed)     { setLeft(-1, speed); setRight(+1, speed); }
void turnRight(uint8_t speed)    { setLeft(+1, speed); setRight(-1, speed); }
void stopMotors()                { setLeft(0, 0);      setRight(0, 0);      }

/**
 * "Backward" is implemented as a 360° spin in place, NOT actual reverse.
 * You can spin left or right; here we spin left.
 */
void spin360(uint8_t speed) {
  turnLeft(speed);
  delay(SPIN_TIME_MS);   // tune for ~360° rotation
  stopMotors();
}

/***************** ULTRASONIC *****************/

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) {
    // timeout = no echo, treat as far away
    return 400;
  }
  long distance = duration / 58; // microseconds → cm approx
  return distance;
}

/**
 * Scan front, left and right using ONE ultrasonic sensor.
 * Robot:
 *   1) starts facing "front"
 *   2) measure front
 *   3) turn left ~90°, measure left
 *   4) turn right ~180°, measure right
 *   5) turn left ~90° to come back to original orientation
 */
void scanDirections(long *front, long *left, long *right) {
  stopMotors();
  delay(100);
  *front = readDistanceCM();

  // look left
  turnLeft(SPEED_TURN);
  delay(ROTATE_TIME_MS);
  stopMotors();
  delay(100);
  *left = readDistanceCM();

  // look right (from left position: rotate right 180°)
  turnRight(SPEED_TURN);
  delay(ROTATE_TIME_MS * 2);
  stopMotors();
  delay(100);
  *right = readDistanceCM();

  // back to original front (turn left 90°)
  turnLeft(SPEED_TURN);
  delay(ROTATE_TIME_MS);
  stopMotors();
  delay(100);
}

/***************** DIRECTION MEMORY LOGIC *****************/

/**
 * Update and check availability of a direction.
 * - If sensor says "closed" now => mark as blocked for BLOCK_TIME_MS.
 * - While blocked and time not expired => treat as unavailable.
 * - Once timer expires and sensor says "open" => available again.
 */
bool directionAvailable(DirIndex idx, bool isOpenNow) {
  unsigned long now = millis();
  DirBlock &b = dirBlock[idx];

  // If currently measured closed => (re)block for 5s
  if (!isOpenNow) {
    b.blocked = true;
    b.until_ms = now + BLOCK_TIME_MS;
    return false;
  }

  // Direction is open according to sensor, but might still be in "blocked memo"
  if (b.blocked) {
    if (now < b.until_ms) {
      // still in blocked window
      return false;
    } else {
      // block expired, un-block it
      b.blocked = false;
    }
  }

  return true;
}

/***************** DECISION LOGIC *****************/

void decideAndMove() {
  long frontDist, leftDist, rightDist;
  scanDirections(&frontDist, &leftDist, &rightDist);

  bool frontOpenNow = (frontDist > OBSTACLE_CM);
  bool leftOpenNow  = (leftDist  > OBSTACLE_CM);
  bool rightOpenNow = (rightDist > OBSTACLE_CM);

  bool frontOK = directionAvailable(DIR_FWD,  frontOpenNow);
  bool leftOK  = directionAvailable(DIR_LEFT, leftOpenNow);
  bool rightOK = directionAvailable(DIR_RIGHT,rightOpenNow);

  // Debug via Serial (optional)
  Serial.print("F="); Serial.print(frontDist);
  Serial.print(" L="); Serial.print(leftDist);
  Serial.print(" R="); Serial.print(rightDist);
  Serial.print(" | Avail: F="); Serial.print(frontOK);
  Serial.print(" L="); Serial.print(leftOK);
  Serial.print(" R="); Serial.println(rightOK);

  // Priority: Forward > Left > Right
  if (frontOK) {
    moveForward(SPEED_FWD);
    delay(MOVE_STEP_MS);
    return;
  }

  if (leftOK) {
    // turn left to the chosen direction then go forward
    turnLeft(SPEED_TURN);
    delay(ROTATE_TIME_MS);
    moveForward(SPEED_FWD);
    delay(MOVE_STEP_MS);
    return;
  }

  if (rightOK) {
    // turn right to the chosen direction then go forward
    turnRight(SPEED_TURN);
    delay(ROTATE_TIME_MS);
    moveForward(SPEED_FWD);
    delay(MOVE_STEP_MS);
    return;
  }

  // No direction available:
  // instead of going backward, we do a 360° spin and try again next loop.
  spin360(SPEED_TURN);
}

/***************** SETUP & LOOP *****************/

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(9600);
}

void loop() {
  decideAndMove();
}
