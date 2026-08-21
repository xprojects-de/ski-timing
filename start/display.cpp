#include "display.h"

static SSD1306Wire displayF(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

Display::Display() {
}

Display::~Display() {}

void Display::init() {
  displayF.init();
}

void Display::clear() {
  displayF.clear();
}

void Display::display() {
  displayF.display();
}

void Display::setFont(const uint8_t *fontData) {
  displayF.setFont(fontData);
}

void Display::setTextAlignment(DISPLAY_TEXT_ALIGNMENT textAlignment) {
  displayF.setTextAlignment(textAlignment);
}

void Display::drawString(int16_t x, int16_t y, String text) {
  displayF.drawString(x, y, text);
}
