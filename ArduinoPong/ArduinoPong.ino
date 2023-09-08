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
int centerY= 20;
int radius = 3;
int velocityX = 3;
int velocityY = 3;


// boundaries ball
int leftBound = centerX - radius;
int rightBound = centerX + radius;
int upperBound = centerY - radius;
int lowerBound = centerY + radius;
bool inBounds = ((leftBound > 0) && (rightBound < maxPixelX) && (upperBound > 0) && (lowerBound < (maxPixelY - 1)));






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
  display.drawCircle(centerX, centerY, radius, WHITE);  // (Xcenter, Ycenter, radius)
  display.fillCircle(centerX, centerY, radius, WHITE);


bool inBounds = ((leftBound > 0) && (rightBound < maxPixelX) && (upperBound > 0) && (lowerBound < (maxPixelY - 1)));
  Serial.print(inBounds);


  if (inBounds != 1) {
    centerX = centerX + 1;
    centerY = centerY + 1;
  }
 


  /*
// '1' should become a velocity variable


initially
centerX = centerX + 1
centerY = centerY + 1




if (hit wallRight || hit wallLeft){
  velocityX = - velocityX
  }


else if (hit top || hit bottom){
  velocityY = - velocityY
  }


  centerX update
  centerY update




*/




  display.display();  // required to refresh the screen contents
  delay(50);
}




/*
// HELLO WORLD CODE


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  // initialize pin as output
}


void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // switch LED on (HIGH voltage)
  delay(1000);                      // wait 1000 ms, i.e. one second
  digitalWrite(LED_BUILTIN, LOW);   // switch LED off (LOW voltage)
  delay(1000);                      // wait a second
}
*/



