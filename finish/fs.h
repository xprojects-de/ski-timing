#ifndef FS_H
#define FS_H

#include <Wire.h>
#include <Arduino.h>

class Fs {

private:
  bool _initialized;
  int getLastId();

public:
  Fs();
  ~Fs();

  void init();
  bool isInit();
  bool append(float time);
  String readAll();
  bool clear();
};

#endif