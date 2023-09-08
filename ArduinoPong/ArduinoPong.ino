/*
Display button value on OLED
The libraries can be added via the library manager: make sure you have verion:
"Adafruit SSD1306 Wemos Mini OLED" by "Adafruit + mcauser", see below
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// #include <SSD1306.h>

#define KEYA D3
#define KEYB D4
#define OLED_RESET -1  // GPIO1

Adafruit_SSD1306 display(OLED_RESET);

// screen limitis
int maxPixelX = 63;
int maxPixelY = 47;


// coordinates line(paddle)
int x = 23;
int y = 47;
int length = 16;
int buttonA = 0;
int buttonB = 0;

//coordinates ball
int centerX = 20;
int centerY = 20;
int radius = 3;
int velocityX = 1;
int velocityY = 1;

// boundaries ball
int leftBound = radius;
int rightBound = maxPixelX - radius;
int upperBound = radius;
int lowerBound = maxPixelY - radius;
bool inBounds = (centerX > leftBound) && (centerX < rightBound) && (centerY > upperBound) && (centerY < (lowerBound - 1));



void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C address
  delay(1000);

  pinMode(KEYA, INPUT);
  pinMode(KEYB, INPUT);


  display.drawCircle(centerX, centerY, radius, WHITE);  // (Xcenter, Ycenter, radius)
  display.fillCircle(centerX, centerY, radius, WHITE);
}



void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  // display.setTextSize(2);
  // display.println("NDL");
  // display.setTextSize(1);

  // display.print("Key A = ");
  // display.println(digitalRead(KEYA));
  // display.print("Key B = ");
  // display.println(digitalRead(KEYB));

  display.drawFastHLine(x, y, length, WHITE);  // (x, y, length, color)
  buttonA = digitalRead(KEYA);
  buttonB = digitalRead(KEYB);

  if (buttonA != 1 && x > 0) {
    x = x - 1;
  } else if (buttonB != 1 && x < 63 - length) {
    x = x + 1;
  }


  inBounds = (centerX > leftBound) && (centerX < rightBound) && (centerY > upperBound) && (centerY < (lowerBound - 1));
  // Serial.println("bounds: " + inBounds);
  // Serial.println("left: " + (centerX > leftBound));
  // Serial.println("right: " + (centerX < rightBound));
  // Serial.println("up: " + (centerY > upperBound));
  // Serial.println("low: " + (centerY > (lowerBound - 1)));

  if (inBounds) {
    centerX = centerX + 1;
    centerY = centerY + 1;
  }

  display.drawCircle(centerX, centerY, radius, WHITE);  // (Xcenter, Ycenter, radius)
  display.fillCircle(centerX, centerY, radius, WHITE);





  if (hitRight() || hitLeft()) {
    velocityX = velocityX * -1;
  }

  else if (hitTop() || hitLower()) {
    velocityY = velocityY * -1;
  }

  // centerX update
  // centerY update

  centerX = centerX + velocityX;
  centerY = centerY + velocityY;



  display.display();  // required to refresh the screen contents
  delay(50);
}

bool hitRight() {
  return centerX == maxPixelX - radius;
}

bool hitLeft() {
  return centerX == radius;
}

bool hitTop() {
  return centerY == radius;
}

// should distinguish between paddle and lower
bool hitLower() {
  return centerY == maxPixelY - radius;
}
