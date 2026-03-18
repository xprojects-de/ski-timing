#include "time.h"

// Konstruktor: Initialisiert die Werte
Time::Time() {
    _startTime = 0;
    _lastTime = 0;
    _isRunning = false;
}

Time::~Time() {}

void Time::trigger() {

    if (!_isRunning) {

      _startTime = millis();
      _isRunning = true;

      return;

    };

    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - _startTime;

    _lastTime = elapsed;
    _startTime = currentTime;

}

String Time::formatTime(unsigned long ms) {

    int seconds = ms / 1000;
    int hundredths = (ms % 1000) / 10;
    String res = String(seconds) + ".";
    if (hundredths < 10) res += "0";
    res += String(hundredths);

    return res;

}

String Time::draw() {
    unsigned long currentElapsed = _isRunning ? (millis() - _startTime) : 0;    
    return formatTime(_lastTime) + " / " + formatTime(currentElapsed);
}
