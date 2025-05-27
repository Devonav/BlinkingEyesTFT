# Blinking Eyes on ESP32-S3 T-Display

This project runs a smooth eye-blinking animation on the LILYGO T-Display-S3 using the `TFT_eSPI` library. It features:

- Smooth blinking and pupil tracking
- Centered and adjustable eye spacing
- Dynamic moods (color shifting)
- Optional tracking mode (pupils follow a path)
- Fully compatible with TFT_eSPI and Adafruit/ST7789

## Hardware
- LILYGO T-Display-S3 ESP32-S3 board
- Built-in 1.9" ST7789 TFT display (320x170)

##  Features
- Adjustable eye spacing (`EYE_SPACING`) 
- Optional eye tracking mode (`t` key in Serial Monitor)
- Winking, random blinking, and mood-based colors

## Controls
- `t` – Toggle tracking mode via Serial Monitor
- `FRAME_DELAY`, `BLINK_FRAMES` – Customize blink speed
- `EYE_WIDTH`, `EYE_HEIGHT` – Resize the eyes

## 🎞️ Demo

![Eyes Animation](eyes_move.gif)

