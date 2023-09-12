/*
Display button value on OLED
The libraries can be added via the library manager: make sure you have verion:
"Adafruit SSD1306 Wemos Mini OLED" by "Adafruit + mcauser", see below
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "paj7620.h"

#define KEYA D3
#define KEYB D4
#define OLED_RESET -1  // GPIO1

// Motion sensor
#define GES_REACTION_TIME		500				// You can adjust the reaction time according to the actual circumstance.
#define GES_ENTRY_TIME			800				// When you want to recognize the Forward/Backward gestures, your gestures' reaction time must less than GES_ENTRY_TIME(0.8s). 
#define GES_QUIT_TIME			1000


Adafruit_SSD1306 display(OLED_RESET);

// Screen border values
int maxPixelX = 63;
int maxPixelY = 47;


// Line (paddle) properties
int paddleX = 23;
int paddleY = 47;
int paddleLength = 16;

int buttonA;  // = 0;
int buttonB;  // = 0;

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

int lastPressedButton = 0;



void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C address
  delay(1000);

  uint8_t error = 0;
  error = paj7620Init();  // initialize Paj7620 registers

  if (error) {
    Serial.print("INIT ERROR,CODE:");
    Serial.println(error);
  } else {
    Serial.println("INIT OK");
  }
  pinMode(KEYA, INPUT);
  pinMode(KEYB, INPUT);

  setRandomBallCoordinates();
}



void loop() {







  while (!gameOver) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.print(score);

    // drawing a paddle
    display.drawFastHLine(paddleX, paddleY, paddleLength, WHITE);

    // drawing a circle
    display.drawCircle(ballCenterX, ballCenterY, ballRadius, WHITE);
    display.fillCircle(ballCenterX, ballCenterY, ballRadius, WHITE);

    // reading input from buttons
    buttonA = digitalRead(KEYA);
    buttonB = digitalRead(KEYB);

    // // checking for last pressed key
    // if (buttonA != 1) {
    //   lastPressedButton = 1;
    // } else if (buttonB != 1) {
    //   lastPressedButton = 2;
    // }

    // // the logic of moving the paddle
    // if (lastPressedButton == 1 && paddleX > 0) {
    //   paddleX = paddleX - 1;
    //   lastPressedButton = 1;
    // } else if (lastPressedButton == 2 && paddleX < 63 - paddleLength) {
    //   paddleX = paddleX + 1;
    //   lastPressedButton = 2;
    // }

    movePaddle();

    if (hitRight() || hitLeft()) {
      ballVelocityX = ballVelocityX * -1;
    }

    if (hitTop()) {
      ballVelocityY = ballVelocityY * -1;
    }

    if (hitPaddle()) {
      score++;
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

  delay(100);
}


void movePaddle() {
  if (GES_LEFT_FLAG != 1) {
    lastPressedButton = 1;
  } else if (GES_RIGHT_FLAG != 1) {
    lastPressedButton = 2;
  }
  uint8_t data = 0, data1 = 0, error;

  error = paj7620ReadReg(0x43, 1, &data);  // Read Bank_0_Reg_0x43/0x44 for gesture result.
  if (!error) {
    switch (data)  // When different gestures be detected, the variable 'data' will be set to different values by paj7620ReadReg(0x43, 1, &data).
    {
      case GES_RIGHT_FLAG:
        delay(GES_ENTRY_TIME);
        paj7620ReadReg(0x43, 1, &data);
        // move paddle to the right
        paddleX = paddleX + 1;
        lastPressedButton = 2;

        break;
      case GES_LEFT_FLAG:
        delay(GES_ENTRY_TIME);
        paj7620ReadReg(0x43, 1, &data);
        // move paddle to the left
        paddleX = paddleX - 1;
        lastPressedButton = 1;
        break;
      default:
        paj7620ReadReg(0x44, 1, &data1);

        Serial.println("do nothing");

        break;
    }
  }
  delay(100);
}


bool hitRight() {
  return ballCenterX == maxPixelX - ballRadius;
}

bool hitLeft() {
  return ballCenterX == ballRadius;
}

bool hitTop() {
  return ballCenterY == ballRadius;
}

bool hitPaddle() {
  return (ballCenterY == paddleY - 1 - ballRadius)
         && (ballCenterX >= paddleX) && (ballCenterX <= paddleX + paddleLength);
}

bool hitPaddleLeftSide() {
  return ballCenterX <= paddleX + paddleLength / 2;
}

bool hitBottom() {
  return ballCenterY == maxPixelY;
}

void resetGame() {
  gameOver = false;
  score = 0;
  setRandomBallCoordinates();
  paddleInMiddle();
}

void setRandomBallCoordinates() {
  // In line ballCenterX the random range is [ballRadius+1 ; maxPixelX - 4]
  ballCenterX = ballRadius + 1 + (rand() % (maxPixelX - 4 - ballRadius));
  ballCenterY = ballRadius + 5 + (rand() % maxPixelY / 2);
  ballVelocityY = 1;
}

void paddleInMiddle() {
  paddleX = 23;
}
