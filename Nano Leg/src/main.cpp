#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define PIN 2
#define TRUE_LENGTH 44

int LED_len = 10;
int STRIP_LENGTH = TRUE_LENGTH + LED_len * 2;

void colorWipe(uint32_t c, uint8_t wait);
void rainbow(uint8_t wait);
void rainbowCycle(uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait, uint8_t spacing);
void theaterChaseRainbow(uint8_t wait);
void render(uint8_t x1, uint8_t length1, uint32_t color1, uint8_t x2, uint8_t length2, uint32_t color2);
uint32_t Wheel(byte WheelPos);
String getValue(String data, char separator, int index);

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_LENGTH, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup()
{
  strip.begin();
  strip.setBrightness(50);
  strip.show();
  Serial.begin(115200);
}

int delta_t = 40; // ms 70 led -> Period 1400 ms
float time = 0;

int x1 = 0;
float v1 = 1;

int x2 = 0;
float v2 = 1;

float data_1 = 0;
float data_2 = 0;

String data = "0.1-0.1";
int hold_1 = false;
int hold_2 = false;

uint8_t red = strip.Color(255, 0, 0);
uint8_t green = strip.Color(0, 255, 0);
uint8_t blue = strip.Color(0, 0, 255);
uint8_t magenta = strip.Color(255, 0, 255);

void loop()
{
  Serial.flush();

  if (x1 == STRIP_LENGTH and x2 == STRIP_LENGTH)
  {
    while (Serial.available() > 0)
    {
      data = Serial.readStringUntil('a');
    }
    data_1 = min(2.0, (getValue(data, '-', 0).toFloat()));
    data_2 = min(2.0, (getValue(data, '-', 1).toFloat()));
    Serial.println("Reset");
    hold_1 = false;
    hold_2 = false;
    x1 = 0;
    x2 = 0;
    time = 0;
    v1 = 1 + data_1;
    v2 = 1 + data_2;
    // Serial.print(data_1);
    // Serial.print(" ");
    // Serial.print(data_2);
    // Serial.println();
  }
  x1 = v1 * time;
  x2 = v2 * time;

  x1 %= STRIP_LENGTH;
  x2 %= STRIP_LENGTH;
  // Serial.print(time);
  // Serial.print(" Before ");
  // Serial.print(x1);
  // Serial.print(" ");
  // Serial.println(x2);

  if (x1 > (TRUE_LENGTH + LED_len))
  {
    Serial.println("Hold 1");
    hold_1 = true;
  }
  if (x2 > (TRUE_LENGTH + LED_len))
  {
    Serial.println("Hold 2");
    hold_2 = true;
  }

  if (hold_1)
  {
    x1 = STRIP_LENGTH;
  }
  if (hold_2)
  {
    x2 = STRIP_LENGTH;
  }

  // Serial.print(x1);
  // Serial.print(" ");
  // Serial.println(x2);
  render(x1, LED_len, strip.Color(0, 0, 255), x2, LED_len, strip.Color(255, 0, 0));
  delay(delta_t);
  time += 0.8;
}

int invert(int i)
{
  return i;
  return TRUE_LENGTH - i;
}

void render(uint8_t x1, uint8_t length1, uint32_t color1, uint8_t x2, uint8_t length2, uint32_t color2)
{
  // colorWipe(strip.Color(0, 0, 0), 0);
  uint8_t start1 = max(0, min(STRIP_LENGTH, x1));
  uint8_t end1 = max(0, min(STRIP_LENGTH, x1 - length1));
  uint8_t start2 = max(0, min(STRIP_LENGTH, x2));
  uint8_t end2 = max(0, min(STRIP_LENGTH, x2 - length2));

  for (uint16_t i = end1; i <= start1; i++) // Loop through x1
  {
    if (i <= start2 && i >= end2) // If overlapped with X2
    {
      uint32_t color = strip.Color(255, 0, 255);
      strip.setPixelColor(invert(i), color);
    }
    else
    {
      strip.setPixelColor(invert(i), color1);
    }
  }

  for (uint16_t i = end2; i <= start2; i++) // Loop through x2
  {
    if (i <= start1 && i >= end1) // If overlapped with X1
    {
      uint32_t color = strip.Color(255, 0, 255);
      strip.setPixelColor(invert(i), color);
    }
    else
    {
      strip.setPixelColor(invert(i), color2);
    }
  }

  strip.show();
  for (uint16_t i = end1; i <= start1; i++)
  {
    strip.setPixelColor(invert(i), 0);
  }
  for (uint16_t i = end2; i <= start2; i++)
  {
    strip.setPixelColor(invert(i), 0);
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait)
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256; j++)
  {
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++)
  { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait, uint8_t spacing)
{
  for (int j = 0; j < 10; j++)
  { // do 10 cycles of chasing
    for (int q = 0; q < spacing; q++)
    {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + spacing)
      {
        strip.setPixelColor(i + q, c); // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + spacing)
      {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait)
{
  for (int j = 0; j < 256; j++)
  { // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++)
    {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, Wheel((i + j) % 255)); // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}