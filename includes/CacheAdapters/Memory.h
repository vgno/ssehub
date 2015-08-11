#ifndef MEMORY_H
#define MEMORY_H

#include "CacheInterface.h"

class Memory : public CacheInterface {
  public:
    Memory(int length);
};
#endif
