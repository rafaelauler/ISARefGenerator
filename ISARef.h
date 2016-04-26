#ifndef ISAREF_H
#define ISAREF_H

extern "C" {
#include "acpp.h"
}
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

class ISARef {
private:
  void ReadISACppFile(const std::string &FileName);
  void PrintIns(ac_dec_instr *Instr);
public:
  ISARef(ac_dec_format *FormatList, ac_dec_instr *InstrList,
         const char *ArchName);

  ac_dec_format *FormatList;
  std::map<std::string, std::string> InstrImpls;
};

#endif
