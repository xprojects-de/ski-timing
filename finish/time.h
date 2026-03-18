#ifndef TIME_H
#define TIME_H

#include <Wire.h>

class Time {

  private:
    unsigned long _startTime;
    unsigned long _lastTime;
    bool _isRunning;

  public:
    Time();
    ~Time();

    void trigger();
    String draw();
    
    static String formatTime(unsigned long ms);

};

#endif