#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

// --- hardware setup ---
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW   // common for FC-16 modules
#define MAX_DEVICES   4                     // how many 8x8 blocks? set 1 if you have a single module
#define CS_PIN        10                    // chip select (LOAD). DIN=11, CLK=13 via SPI

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// message you want to scroll
char msg[] = "Ce faci ?  ";

void setup() {
  P.begin();
  P.setIntensity(3);        // 0 (dim) to 15 (bright). go easy if USB powered
  P.displayClear();
  // If your text is upside-down, uncomment the next line:
  // P.setZoneEffect(0, true, PA_FLIP_UD);

  // center, left scroll, speed=40 (lower is slower)
  P.displayText(msg, PA_CENTER, 40, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void loop() {
  if (P.displayAnimate()) {
    P.displayReset();       // loop the animation forever
  }
}
