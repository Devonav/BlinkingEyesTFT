#include <TFT_eSPI.h>
#include <SPI.h>
#include <algorithm>
// Display
TFT_eSPI tft = TFT_eSPI();

// Constants
#define EYE_WIDTH 30
#define EYE_HEIGHT 50
#define EYE_SPACING 55
#define BLINK_FRAMES 20
#define FRAME_DELAY 4
#define OPEN_TIME_MIN 1000
#define OPEN_TIME_MAX 4000
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 170
const int maxPupilOffsetX = 16;
const int maxPupilOffsetY = 12;

// States
int blinkState = 0;
bool isBlinking = false;
bool isWinking = false;
int winkEyeIndex = -1;
unsigned long lastBlinkTime = 0;
unsigned long openTime = OPEN_TIME_MIN;
unsigned long lastPupilMove = 0;
const int pupilMoveInterval = 1000;

// Tracking
bool trackingMode = true;
float targetX = 0;
float targetY = 0;
float angle = 0.0;
float radius = 20.0;
unsigned long lastTargetUpdate = 0;

// Eye positions
int16_t eyeY;
int16_t eyePositions[2];
int16_t pupilOffsetX[2] = {0, 0};
int16_t pupilOffsetY[2] = {0, 0};

// Buffer
uint16_t *buffer = nullptr;
int bufferWidth = 0;
int bufferHeight = 0;
int bufferX = 0;
int bufferY = 0;

// Mood colors
uint16_t currentEyeColor = TFT_WHITE;
uint16_t currentBgColor = TFT_BLACK;

// Helper: Fill ellipse into buffer
void bufferFillEllipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color) {
  x0 = x0 - bufferX;
  y0 = y0 - bufferY;

  for (int16_t y = -ry; y <= ry; y++) {
    if (y0 + y >= 0 && y0 + y < bufferHeight) {
      int16_t x = sqrt(1.0 - (float)(y * y) / (ry * ry)) * rx;
      int16_t startX = std::max(0, x0 - x);
      int16_t endX = std::min(bufferWidth - 1, x0 + x);
      for (int16_t i = startX; i <= endX; i++) {
        buffer[i + (y0 + y) * bufferWidth] = color;
      }
    }
  }
}

// Clear and flush buffer
void clearBuffer() {
  for (int i = 0; i < bufferWidth * bufferHeight; i++) {
    buffer[i] = currentBgColor;
  }
}

void flushBuffer() {
  tft.startWrite();
  tft.setAddrWindow(bufferX, bufferY, bufferWidth, bufferHeight);
  tft.pushPixels(buffer, bufferWidth * bufferHeight);
  tft.endWrite();
}

// Draw pupils and eyes
void drawEyesWithPupils(int currentHeight) {
  clearBuffer();
  for (int i = 0; i < 2; i++) {
    int eyeH = (winkEyeIndex == i && isWinking) ? 1 : currentHeight;
    bufferFillEllipse(eyePositions[i], eyeY, EYE_WIDTH, eyeH, currentEyeColor);

    if (eyeH > EYE_HEIGHT / 3) {
      int16_t pupilX = eyePositions[i] + pupilOffsetX[i];
      int16_t pupilY = eyeY + pupilOffsetY[i];
      int16_t pupilW = EYE_WIDTH / 5;
      int16_t pupilH = eyeH / 3;
      bufferFillEllipse(pupilX, pupilY, pupilW, pupilH, currentBgColor);
    }
  }
  flushBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  tft.init();
  tft.setRotation(3);  // Landscape, flipped for USB right-side
  tft.fillScreen(currentBgColor);

  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);  // Backlight on

  int16_t startX = (tft.width() - EYE_SPACING) / 2;
  eyeY = tft.height() / 2;

  for (int i = 0; i < 2; i++) {
    eyePositions[i] = startX + (i * EYE_SPACING);
  }

  bufferX = eyePositions[0] - EYE_WIDTH - 5;
  bufferY = eyeY - EYE_HEIGHT - 5;
  bufferWidth = eyePositions[1] - bufferX + EYE_WIDTH + 5;
  bufferHeight = (EYE_HEIGHT * 2) + 10;

  buffer = (uint16_t *)malloc(bufferWidth * bufferHeight * sizeof(uint16_t));
  if (!buffer) {
    Serial.println("Failed to allocate buffer memory!");
    while (1);
  }

  drawEyesWithPupils(EYE_HEIGHT);
  randomSeed(micros());

  lastBlinkTime = millis();
  openTime = random(OPEN_TIME_MIN, OPEN_TIME_MAX);
}

void loop() {
  unsigned long currentTime = millis();

  // Serial toggle for tracking mode
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't') {
      trackingMode = !trackingMode;
      Serial.println(trackingMode ? "Tracking ON" : "Tracking OFF");
      angle = 0;
    }
  }

  // Pupil tracking
  if (!isBlinking) {
    if (trackingMode) {
      if (millis() - lastTargetUpdate > 16) {
        angle += 0.02;
        targetX = sin(angle) * radius;
        targetY = cos(angle * 1.5) * radius * 0.7;

        for (int i = 0; i < 2; i++) {
          pupilOffsetX[i] = (int16_t)targetX;
          pupilOffsetY[i] = (int16_t)targetY;
        }

        lastTargetUpdate = millis();
      }
    } else if (millis() - lastPupilMove > pupilMoveInterval) {
      int16_t dx = random(-maxPupilOffsetX, maxPupilOffsetX + 1);
      int16_t dy = random(-maxPupilOffsetY, maxPupilOffsetY + 1);

      for (int i = 0; i < 2; i++) {
        pupilOffsetX[i] = dx;
        pupilOffsetY[i] = dy;
      }

      lastPupilMove = millis();
    }
  }

  // Blink or wink start
  if (!isBlinking && (currentTime - lastBlinkTime >= openTime)) {
    if (random(10) < 2) {  // 20% chance wink
      isWinking = true;
      winkEyeIndex = random(0, 2);
    } else {
      isWinking = false;
      winkEyeIndex = -1;
    }

    isBlinking = true;
    blinkState = 0;
  }

  if (isBlinking) {
    float progress;
    if (blinkState < BLINK_FRAMES / 2) {
      progress = 1.0 - pow((float)blinkState / (BLINK_FRAMES / 2), 2);
    } else {
      float p = (float)(blinkState - BLINK_FRAMES / 2) / (BLINK_FRAMES / 2);
      progress = p * (2 - p);
    }

    int16_t h = std::max((int16_t)1, (int16_t)(EYE_HEIGHT * progress));
    drawEyesWithPupils(h);
    blinkState++;

    if (blinkState >= BLINK_FRAMES) {
      isBlinking = false;
      lastBlinkTime = millis();
      openTime = random(OPEN_TIME_MIN, OPEN_TIME_MAX);
      if (random(10) == 0) openTime = 300;  // double-blink chance
    }

    delay(FRAME_DELAY);
  } else {
    drawEyesWithPupils(EYE_HEIGHT);
    delay(FRAME_DELAY * 4);
  }
}
