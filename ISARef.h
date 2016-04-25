#ifndef ISAREF_H
#define ISAREF_H

extern "C" {
#include "acpp.h"
}
#include <cstdint>
#include <memory>
#include <vector>

class ISARef {
public:
  ISARef(ac_dec_format *FormatList, ac_dec_instr *InstrList,
         uint32_t Wordsize);
};

#endif
