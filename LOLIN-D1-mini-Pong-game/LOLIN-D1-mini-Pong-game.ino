/*
Display button value on OLED
The libraries can be added via the library manager: make sure you have verion:
"Adafruit SSD1306 Wemos Mini OLED" by "Adafruit + mcauser", see below
*/

// adding libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "paj7620.h"

#include <Wire.h>
#include <LOLIN_I2C_MOTOR.h>
#define PWM_FREQUENCY 1000


#define OLED_RESET -1  // GPIO1
#define KEYA D3


LOLIN_I2C_MOTOR motor(DEFAULT_I2C_MOTOR_ADDRESS);

Adafruit_SSD1306 display(OLED_RESET);

int buttonA;

// Screen border values
int maxPixelX = 63;
int maxPixelY = 47;

// Line (paddle) properties
int paddleX = 23;
int paddleY = 47;
int paddleLength = 16;

// Ball properties
int ballCenterX;
int ballCenterY;
int ballRadius = 3;
int ballVelocityX = 1;
int ballVelocityY = 1;

// boundaries ball
int leftBound = ballRadius;
int rightBound = maxPixelX - ballRadius;
int upperBound = ballRadius;
int lowerBound = maxPixelY - ballRadius;
bool inBounds = (ballCenterX > leftBound) && (ballCenterX < rightBound) && (ballCenterY > upperBound) && (ballCenterY < (lowerBound - 1));

// game properties
bool gameOver = false;
int score = 0;

// representation of the last chosen direction to move in
int lastPressedButton = 0;

void setup() {
  // Setting up Paj7620 motion detector
  uint8_t error = 0;
  error = paj7620Init();  // initialize Paj7620 registers

  if (error) {
    Serial.print("INIT ERROR,CODE:");
    Serial.println(error);
  } else {
    Serial.println("INIT OK");
  }

  // wait till motor is ready
  while (motor.PRODUCT_ID != PRODUCT_ID_I2C_MOTOR) {
    motor.getInfo();
  }

  pinMode(KEYA, INPUT);

  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C address
  delay(1000);

  // spawn ball at a random position on the screen
  setRandomBallCoordinates();
}


void loop() {

  while (!gameOver) {
    // initialising LED screen
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.print(score);

    // drawing the paddle
    display.drawFastHLine(paddleX, paddleY, paddleLength, WHITE);

    // drawing the bal (circle)
    display.fillCircle(ballCenterX, ballCenterY, ballRadius, WHITE);

    // choose paddle direction
    paddleDirection();

    // the logic of moving the paddle
    if (lastPressedButton == 1 && paddleX > 0) {
      paddleX = paddleX - 1;
    } else if (lastPressedButton == 2 && paddleX < 63 - paddleLength) {
      paddleX = paddleX + 1;
    }

    // The logic of bouncing the ball of the borders
    if (hitRight() || hitLeft()) {
      ballVelocityX = ballVelocityX * -1;
    }

    if (hitTop()) {
      ballVelocityY = ballVelocityY * -1;
    }

    if (hitPaddle()) {

      score++;
      releaseCandy();

      ballVelocityY = ballVelocityY * -1;

      if (hitPaddleLeftSide()) {
        ballVelocityX = abs(ballVelocityX) * (-1);
      } else {
        ballVelocityX = abs(ballVelocityX);
      }
    }

    if (hitBottom()) {
      gameOver = true;
    }

    // changing the velocity/direction of the ball
    ballCenterX = ballCenterX + ballVelocityX;
    ballCenterY = ballCenterY + ballVelocityY;

    display.display();  // required to refresh the screen contents
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.println("Game Over");
  display.print("Your score: ");
  display.println(score);
  display.println("Press A to play");
  display.display();

  if (!digitalRead(KEYA)) {
    resetGame();
  }
}

void releaseCandy() {
  motor.changeFreq(MOTOR_CH_BOTH, PWM_FREQUENCY);
  motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CCW);
  for (int duty = 40; duty <= 45; duty += 1) {
    motor.changeDuty(MOTOR_CH_A, duty);
    // delay(200);
  }
  motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_STANDBY);
  // delay(500);
}

void paddleDirection() {
  uint8_t data = 0, error;

  error = paj7620ReadReg(0x43, 1, &data);  // Read Bank_0_Reg_0x43/0x44 for gesture result.
  if (!error) {
    switch (data)  // When different gestures be detected, the variable 'data' will be set to different values by paj7620ReadReg(0x43, 1, &data).
    {
      case GES_RIGHT_FLAG:
        // paddleX = paddleX + 1;
        lastPressedButton = 2;
        break;
      case GES_LEFT_FLAG:
        // paddleX = paddleX - 1;
        lastPressedButton = 1;
        break;
      default:
        break;
    }
  }
}


// hitting the right border
bool hitRight() {
  return ballCenterX == maxPixelX - ballRadius;
}

// hitting the left border
bool hitLeft() {
  return ballCenterX == ballRadius;
}

// hitting the top border
bool hitTop() {
  return ballCenterY == ballRadius;
}

// hitting the paddle
bool hitPaddle() {
  return (ballCenterY == paddleY - 1 - ballRadius)
         && (ballCenterX >= paddleX) && (ballCenterX <= paddleX + paddleLength);
}

// hitting the left side of the paddle
// to change direction appropriately
bool hitPaddleLeftSide() {
  return ballCenterX <= paddleX + paddleLength / 2;
}

// hitting the bottom of the screen, aka game over
bool hitBottom() {
  return ballCenterY == maxPixelY;
}

// resetting the game
void resetGame() {
  gameOver = false;
  score = 0;
  setRandomBallCoordinates();
  paddleInMiddle();
}

// spwaning the ball at random coordinates.
void setRandomBallCoordinates() {
  // In line ballCenterX the random range is [ballRadius+1 ; maxPixelX - 4]
  ballCenterX = ballRadius + 1 + (rand() % (maxPixelX - 4 - ballRadius));
  ballCenterY = ballRadius + 1 + (rand() % 2);
  ballVelocityY = 1;
}

// puts the paddle in the middle of the screen
void paddleInMiddle() {
  paddleX = 23;
}

/* MOTOR CODE
#include <Wire.h>
#include <LOLIN_I2C_MOTOR.h>
#define PWM_FREQUENCY 1000

LOLIN_I2C_MOTOR motor(DEFAULT_I2C_MOTOR_ADDRESS);
//I2C address 0x30 SEE NOTE BELOW

void setup() {
  //wait until motor shield ready.
  while (motor.PRODUCT_ID != PRODUCT_ID_I2C_MOTOR) {
    motor.getInfo();
  }
}

void loop() {
  motor.changeFreq(MOTOR_CH_BOTH, PWM_FREQUENCY);
  motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CCW);
  for (int duty = 40; duty <= 100; duty += 1) {
    motor.changeDuty(MOTOR_CH_A, duty);
    delay(200);
  }
  motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_STANDBY);
  delay(500);
}
*/
