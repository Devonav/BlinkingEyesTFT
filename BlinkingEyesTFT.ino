#include <TFT_eSPI.h>
#include <SPI.h>
#include <algorithm>

TFT_eSPI tft = TFT_eSPI();

// Eye and blink constants
#define EYE_WIDTH 30
#define EYE_HEIGHT 50
#define EYE_SPACING 80

#define BLINK_FRAMES 20
#define FRAME_DELAY 4
#define OPEN_TIME_MIN 1000
#define OPEN_TIME_MAX 4000

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 170

int blinkState = 0;
bool isBlinking = false;
unsigned long lastBlinkTime = 0;
unsigned long openTime = OPEN_TIME_MIN;

int16_t eyeY;
int16_t eyePositions[2];

uint16_t *buffer = nullptr;
int bufferWidth = 0;
int bufferHeight = 0;
int bufferX = 0;
int bufferY = 0;

// Pupil tracking
int16_t pupilOffsetX[2] = {0, 0};
int16_t pupilOffsetY[2] = {0, 0};
unsigned long lastPupilMove = 0;
const int pupilMoveInterval = 1000;
const int maxPupilOffset = 9;

// Mood system
struct Mood {
  const char* name;
  uint16_t eyeColor;
  uint16_t bgColor;
};

Mood moods[] = {
  {"Happy", TFT_YELLOW, TFT_BLACK},
  {"Sleepy", TFT_BLUE, TFT_NAVY},
  {"Angry", TFT_RED, TFT_DARKGREY},
  {"Calm", TFT_WHITE, TFT_BLACK},
  {"Excited", TFT_CYAN, TFT_MAGENTA},
  {"Neutral", TFT_WHITE, TFT_BLACK}
};

uint16_t currentEyeColor = TFT_WHITE;
uint16_t currentBgColor = TFT_BLACK;
unsigned long lastMoodChange = 0;
const int moodChangeInterval = 1000;

// Winking
bool isWinking = false;
int winkEyeIndex = -1;

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
void drawEyebrows() {
  for (int i = 0; i < 2; i++) {
    int16_t browX = eyePositions[i];
    int16_t browY = bufferY - 10; // a bit above the top of the buffer
    int16_t browLength = 20;

    // Static angled line (you can make this dynamic later)
    tft.drawLine(browX - browLength / 2, browY,
                 browX + browLength / 2, browY - 5,
                 currentEyeColor);  // Match eye color or pick your own
  }
}


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
  drawEyebrows();

}

void setup() {
  Serial.begin(115200);
  delay(1000);

  tft.init();
  tft.setRotation(1);  // Landscape
  tft.fillScreen(TFT_BLACK);

  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);

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
  lastMoodChange = millis();
  openTime = random(OPEN_TIME_MIN, OPEN_TIME_MAX);
}

void loop() {
  unsigned long currentTime = millis();

  // Mood update
  if (currentTime - lastMoodChange >= moodChangeInterval) {
    int moodIndex = random(0, sizeof(moods) / sizeof(Mood));
    currentEyeColor = moods[moodIndex].eyeColor;
    currentBgColor = moods[moodIndex].bgColor;
    lastMoodChange = currentTime;
      // Fill the full display background


    Serial.print("Mood changed: ");
    Serial.println(moods[moodIndex].name);
  }

  // Pupil motion
  if (currentTime - lastPupilMove >= pupilMoveInterval && !isBlinking) {
    for (int i = 0; i < 2; i++) {
      pupilOffsetX[i] = random(-maxPupilOffset, maxPupilOffset + 1);
      pupilOffsetY[i] = random(-maxPupilOffset / 2, maxPupilOffset / 2 + 1);
    }
    lastPupilMove = currentTime;
  }

  // Blink or wink start
  if (!isBlinking && (currentTime - lastBlinkTime >= openTime)) {
    if (random(10) < 2) {  // 20% chance of wink
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
    float blinkProgress;
    if (blinkState < BLINK_FRAMES / 2) {
      blinkProgress = (float)blinkState / (BLINK_FRAMES / 2);
      blinkProgress = 1.0 - (blinkProgress * blinkProgress);
    } else {
      blinkProgress = (float)(blinkState - BLINK_FRAMES / 2) / (BLINK_FRAMES / 2);
      blinkProgress = blinkProgress * (2 - blinkProgress);
    }

    int16_t currentHeight = std::max((int16_t)1, (int16_t)(EYE_HEIGHT * blinkProgress));
    drawEyesWithPupils(currentHeight);
    blinkState++;

    if (blinkState >= BLINK_FRAMES) {
      isBlinking = false;
      lastBlinkTime = millis();
      openTime = random(OPEN_TIME_MIN, OPEN_TIME_MAX);
      if (random(10) == 0) openTime = 300;  // occasional double-blink
    }

    delay(FRAME_DELAY);
  } else {
    delay(FRAME_DELAY * 4);
  }
}
