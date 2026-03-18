#ifndef TIME_H
#define TIME_H

#include <Wire.h>

class Time {

  private:
    unsigned long _startTime;
    unsigned long _limitMS;
    unsigned long _lastTime;
    bool _isRunning;

  public:
    Time(unsigned long limit);
    ~Time();

    void start();
    void reset();
    void update();
    String draw();
    
    static String formatTime(unsigned long ms);

};

#endif