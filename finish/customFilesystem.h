#ifndef CFS_H
#define CFS_H

#include <Arduino.h>
#include <functional>

class CFs {

private:
  bool _initialized;
  int _lastId;
  int getLastId();

public:
  CFs();
  ~CFs();

  void init();
  bool isInit();
  bool append(float time);
  String readAll();
  bool clear();

  bool readInChunks(std::function<bool(const char* buffer, size_t length)> chunkCallback);
};

#endif