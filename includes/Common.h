#undef NDEBUG
#include <glog/logging.h>

#define ulong unsigned long
#define INC_LONG(longVar) (longVar < 0x7FFFFFFFL) ? longVar++ : longVar = 1
