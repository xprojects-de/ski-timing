#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include "HT_SSD1306Wire.h"

class Display {

  private:

  public:
    Display();
    ~Display();

    void init();
    void clear();
    void display();
    void setFont(const uint8_t *fontData);
    void setTextAlignment(DISPLAY_TEXT_ALIGNMENT textAlignment);
    void drawString(int16_t x, int16_t y, String text);

};

#endif