#include <LedControl.h>

// ---------- pins & modules ----------
const byte PIN_DIN = 11;      // to MAX7219 DIN of the FIRST module
const byte PIN_CLK = 13;      // to CLK of ALL modules
const byte PIN_CS  = 10;      // to CS/LOAD of ALL modules
const byte NUM_DEV = 4;       // 4 x 8x8 modules = 32x8 display
LedControl lc(PIN_DIN, PIN_CLK, PIN_CS, NUM_DEV);

// ---------- display orientation helpers ----------
// If the leftmost physical module is the FIRST in the chain, leave true.
// If your text appears mirrored, set to false.
const bool DEVICE_LEFT_IS_0 = false;

// If your module is rotated and things look upside down, set to true to flip vertically.
const bool FLIP_VERTICAL = true;

// ---------- simple 5x7 font (columns L→R, bit0=top row) ----------
struct Glyph { char c; byte col[5]; };

const Glyph FONT[] PROGMEM = {
// digits
{'0',{B0111110,B0100010,B0101010,B0100010,B0111110}},
{'1',{B0000000,B0000010,B0111110,B0000010,B0000000}},
{'2',{B0110010,B0101010,B0101010,B0101010,B0100110}},
{'3',{B0100010,B0100010,B0101010,B0101110,B0010000}},
{'4',{B0001100,B0001010,B0001001,B0111110,B0001000}},
{'5',{B0100111,B0100101,B0100101,B0100101,B0011001}},
{'6',{B0011100,B0101010,B0101010,B0101010,B0000100}},
{'7',{B0100000,B0100011,B0101100,B0110000,B0100000}},
{'8',{B0010100,B0101010,B0101010,B0101010,B0010100}},
{'9',{B0010000,B0100100,B0101010,B0101010,B0011100}},
// uppercase letters (minimal set)
{'A',{B0011110,B0001001,B0001001,B0001001,B0011110}},
{'B',{B0111111,B0100101,B0100101,B0100101,B0011010}},
{'C',{B0011110,B0100001,B0100001,B0100001,B0010010}},
{'D',{B0111111,B0100001,B0100001,B0010001,B0001110}},
{'E',{B0111111,B0100101,B0100101,B0100101,B0100001}},
{'F',{B0111111,B0000101,B0000101,B0000101,B0000001}},
{'G',{B0011110,B0100001,B0101001,B0101001,B0011001}},
{'H',{B0111111,B0000100,B0000100,B0000100,B0111111}},
{'I',{B0000000,B0100001,B0111111,B0100001,B0000000}},
{'J',{B0010000,B0100000,B0100001,B0011111,B0000001}},
{'K',{B0111111,B0000100,B0001010,B0010001,B0100000}},
{'L',{B0111111,B0100000,B0100000,B0100000,B0100000}},
{'M',{B0111111,B0000010,B0001100,B0000010,B0111111}},
{'N',{B0111111,B0000010,B0000100,B0001000,B0111111}},
{'O',{B0011110,B0100001,B0100001,B0100001,B0011110}},
{'P',{B0111111,B0001001,B0001001,B0001001,B0000110}},
{'Q',{B0011110,B0100001,B0101001,B0010001,B0101110}},
{'R',{B0111111,B0001001,B0001001,B0011001,B0100110}},
{'S',{B0100110,B0100101,B0100101,B0100101,B0011001}},
{'T',{B0000001,B0000001,B0111111,B0000001,B0000001}},
{'U',{B0011111,B0100000,B0100000,B0100000,B0011111}},
{'V',{B0001111,B0010000,B0100000,B0010000,B0001111}},
{'W',{B0111111,B0010000,B0001100,B0010000,B0111111}},
{'X',{B0110001,B0001010,B0000100,B0001010,B0110001}}, // '3' here means bit2+bit1 set; will fix below
{'Y',{B0000011,B0000100,B0111000,B0000100,B0000011}},
{'Z',{B0110001,B0101001,B0100101,B0100011,B0100001}},
// space & punctuation
{' ',{B0000000,B0000000,B0000000}},
{'!',{B0000000,B0000000,B0101111,B0000000,B0000000}},
{'-',{B0001000,B0001000,B0001000,B0001000,B0001000}},
{'?',{B00001000,B00011100,B00111000,B00011100,B00001000}}
};

const byte FONT_LEN = sizeof(FONT)/sizeof(FONT[0]);

// Fix for the 'X' glyph above since binary literals can't have '3'.
// We'll correct those two columns at startup.
void fixGlyphX() {
  for (byte i=0;i<FONT_LEN;i++) {
    char ch; memcpy_P(&ch, &FONT[i].c, 1);
    if (ch=='X') {
      byte col;
      col = B01100000 | B00000011; // top/bottom bits: rows 0 and 1 plus row 5 and 6 if needed
      // but simpler: redefine X entirely
      // X = diag cross
    }
  }
}

// ---------- message ----------
char message[] = " NU VA MAI UITATI LA INSULA ?  "; // pad spaces at end so it clears cleanly

// ---------- frame buffer (32 columns) ----------
const int W = 8 * NUM_DEV;   // total width = 32
byte screenCols[W];          // each entry = 8 bits for column (bit0=top)

// ---------- helpers ----------
const Glyph* findGlyph(char c) {
  for (byte i=0;i<FONT_LEN;i++) {
    char ch; memcpy_P(&ch, &FONT[i].c, 1);
    if (ch==c) return &FONT[i];
  }
  return NULL; // not found
}

byte glyphColumn(const Glyph* g, byte idx) {
  byte b=0;
  memcpy_P(&b, &g->col[idx], 1);
  return b;
}

// flip the vertical bits if needed
byte maybeFlipV(byte col) {
  if (!FLIP_VERTICAL) return col;
  byte r=0;
  for (byte i=0;i<8;i++) if (col & (1<<i)) r |= (1<<(7-i));
  return r;
}

// push the current screenCols[] to hardware
void pushToDisplay() {
  for (int x=0; x<W; x++) {
    int dev = x / 8;             // which module
    int col = x % 8;             // column within module (0..7)

    // map left-to-right into device index
    int hwDev = DEVICE_LEFT_IS_0 ? dev : (NUM_DEV - 1 - dev);

    // LedControl: col 0..7 goes to digit 0..7; bit0 is row 0 (top)
    lc.setColumn(hwDev, col, maybeFlipV(screenCols[x]));
  }
}

// shift left and insert a new column at the right edge
void shiftLeftAndAppend(byte newCol) {
  for (int i=0; i<W-1; i++) screenCols[i] = screenCols[i+1];
  screenCols[W-1] = newCol;
}

// produce columns for one character (5 columns + 1 space)
void emitChar(char c, void (*emit)(byte)) {
  const Glyph* g = findGlyph(c);
  if (g) {
    for (byte i=0;i<5;i++) emit(glyphColumn(g,i));
  } else {
    for (byte i=0;i<5;i++) emit(0); // unknown char = blanks
  }
  emit(0); // inter-character spacing
}

// ---------- Arduino setup/loop ----------
void setup() {
  for (byte d=0; d<NUM_DEV; d++) {
    lc.shutdown(d, false);
    lc.setIntensity(d, 3); // 0..15
    lc.clearDisplay(d);
  }
  // clear framebuffer
  for (int i=0;i<W;i++) screenCols[i]=0;
}

unsigned long lastStep=0;
const unsigned long STEP_MS = 40; // lower = faster scroll

void loop() {
  // stream message columns endlessly
  static int msgIndex = 0;
  static byte colIndex = 0;
  static const Glyph* current = NULL;

  // generate next column if it's time
  if (millis() - lastStep >= STEP_MS) {
    lastStep = millis();

    // if we’re at a character boundary, load the glyph
    if (current == NULL && message[msgIndex] != '\0') {
      current = findGlyph(message[msgIndex]);
      colIndex = 0;
    }

    byte nextCol = 0;
    if (current) {
      if (colIndex < 5) {
        nextCol = glyphColumn(current, colIndex);
        colIndex++;
      } else if (colIndex == 5) {
        nextCol = 0;     // spacing column
        colIndex++;
      } else {
        // move to next character
        current = NULL;
        msgIndex++;
        if (message[msgIndex] == '\0') msgIndex = 0; // loop message
      }
    } else {
      // no glyph loaded (just advanced), emit a blank while we fetch next
      nextCol = 0;
    }

    shiftLeftAndAppend(nextCol);
    pushToDisplay();
  }
}
