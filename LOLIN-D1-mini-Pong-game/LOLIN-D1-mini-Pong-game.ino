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

// libraries and constants for the motor
#include <LOLIN_I2C_MOTOR.h>
#define PWM_FREQUENCY 1000

// for the wifi connection
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#define LISTEN_PORT 80

const char* ssid = "NDL_24G";       // name of local WiFi network in the NDL
const char* password = "RT-AC66U";  // the password of the WiFi network

MDNSResponder mdns;
ESP8266WebServer server(LISTEN_PORT);
String webPage = "<h1>Pong game :) </h1>";

bool ledOn = false;
bool resetOn = false;

bool leftOn = false;
bool rightOn = false;


#define OLED_RESET -1  // GPIO1
#define KEYA D3


LOLIN_I2C_MOTOR motor(DEFAULT_I2C_MOTOR_ADDRESS);

Adafruit_SSD1306 display(OLED_RESET);



// input from the button
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
int paddleDirection = 0; //1 is left, 2 is right

void setup() {

  Serial.begin(115200);
  // Setting up Paj7620 motion detector
  uint8_t error = 0;
  error = paj7620Init();  // initialize Paj7620 registers

  if (error) {
    Serial.print("INIT ERROR,CODE:");
    Serial.println(error);
  } else {
    Serial.println("INIT OK");
  }

  // setting up motor
  while (motor.PRODUCT_ID != PRODUCT_ID_I2C_MOTOR) {
    motor.getInfo();
  }

  // setting up buttons
  pinMode(KEYA, INPUT);


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C address
  delay(1000);


  // setting up wifi connection

  pinMode(LED_BUILTIN, OUTPUT);  // the LED
  pinMode(LED_BUILTIN, OUTPUT);  // the Button

  digitalWrite(LED_BUILTIN, ledOn);  // the actual status is inverted
  digitalWrite(LED_BUILTIN, resetOn);  // the actual status is inverted

  printWebPage();


  WiFi.begin(ssid, password);  // make the WiFi connection
  Serial.println("Start connecting.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // for some reason without this delay nothing worked for me
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.print(ssid);
  Serial.print(". IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  // make handlers for input from WiFi connection
  server.on("/", []() {
    server.send(200, "text/html", webPage);
  });


// this part reads info from the button 'press me' into the ide
  server.on("/button1", []() {
    server.send(200, "text/html", webPage);
    ledOn = !ledOn;
    Serial.print("led ");
    Serial.println(ledOn);
    digitalWrite(LED_BUILTIN, !ledOn);
    delay(100);
  });

  // this part reads info from the button 'reset' 
  // TODO FIX IT
  server.on("/button2", []() {
    server.send(200, "text/html", webPage);
    resetGame();
    Serial.print("reset ");
    Serial.println(resetOn);
    digitalWrite(LED_BUILTIN, !resetOn);
    delay(100);
  });

  // Handling the "LEFT" button
  server.on("/button3", []() {
    server.send(200, "text/html", webPage);
    paddleDirection = 1;
    Serial.print("left ");
    Serial.println(paddleDirection);
    delay(100);
  });

  // Handling the "RIGHT" button
    server.on("/button4", []() {
    server.send(200, "text/html", webPage);
    paddleDirection = 2;
    Serial.print("right ");
    Serial.println(paddleDirection);
    delay(100);
  });

  server.begin();  // start the server for WiFi input
  Serial.println("HTTP server started");


  // spawn ball at a random position on the screen
  setRandomBallCoordinates();
}


void loop() {
  // wifi connection
  server.handleClient();
  mdns.update();


  printWebPage();

  // HERE WE NEED TO REFRESH THE PAGE

  /*
    To refresh the web page we need to include java script to access our website and push new information to the site. 
    This is something we'll probably need to use for our actual project, so it would be nice if could make an attempt with this project. 
    It is optional for this project as it's not clearly mentioned in the reader/guide. We'll need to use AJAX/javscript for this.
  */


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
    pickPaddleDirection();

    // the logic of moving the paddle
    if (paddleDirection == 1 && paddleX > 0) {
      paddleX = paddleX - 1;
    } else if (paddleDirection == 2 && paddleX < 63 - paddleLength) {
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

void printWebPage() {
  webPage = "<h1>Pong game :) </h1>";
  webPage += "<p>Press me <a href=\"button1\">";
  webPage += "<button style=\"background-color:blue;color:white;\">";
  webPage += "LED</button></a></p>";

  webPage += "<p>Reset game <a href=\"button2\">";
  webPage += "<button style=\"background-color:black;color:white;\">";
  webPage += "RESET</button></a></p>";

  webPage += "<p>Move <a href=\"button3\">";
  webPage += "<button style=\"background-color:green;color:white;\">";
  webPage += "LEFT</button></a>";

  webPage += "<a href=\"button4\">";
  webPage += "<button style=\"background-color:green;color:white;\">";
  webPage += "         RIGHT</button></a></p>";

  webPage += "<p>Your  score is: " + String(score) + "</p>";
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

void pickPaddleDirection() {
  uint8_t data = 0, error;

  error = paj7620ReadReg(0x43, 1, &data);  // Read Bank_0_Reg_0x43/0x44 for gesture result.
  if (!error) {
    switch (data)  // When different gestures be detected, the variable 'data' will be set to different values by paj7620ReadReg(0x43, 1, &data).
    {
      case GES_RIGHT_FLAG:
        // paddleX = paddleX + 1;
        paddleDirection = 2;
        break;
      case GES_LEFT_FLAG:
        // paddleX = paddleX - 1;
        paddleDirection = 1;
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
  paddleDirection = 0;
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
