#include "ISARef.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
#include <stack>

extern "C" {
// XXX: We define a dummy GetBits here to please the linker when using
// the ac_decoder library. We use the CreateDecoder function but we do not
// use the decoder per se, so we don't need a working GetBits().
void GetBits() __attribute__((weak));

void GetBits() {
  std::cerr << "Fatal error: GetBits() unimplemented.\n";
  exit(-1);
}

ac_dec_field *FindDecField(ac_dec_field *fields, int id);

}

void PrintIns(ac_dec_instr *Instr) {
  std::cout << Instr->mnemonic << "\n";
}

ISARef::ISARef(ac_dec_format *FormatList, ac_dec_instr *InstrList,
               uint32_t Wordsize) {

  // Alphabetical ordering of instructions' ID
  auto cmp = [](const ac_dec_instr *left, const ac_dec_instr *right) {
    const auto l = left->mnemonic;
    const auto r = right->mnemonic;
    const auto LenL = strlen(l);
    const auto LenR = strlen(r);
    const auto Len = LenL < LenR ? LenL : LenR;
    return !std::lexicographical_compare(l, l + Len, r, r + Len);
  };
  std::priority_queue<ac_dec_instr *, std::vector<ac_dec_instr *>,
                      decltype(cmp)>
      Queue(cmp);

  for (ac_dec_instr *InstrPtr = InstrList; InstrPtr != nullptr;
       InstrPtr = InstrPtr->next) {
    Queue.push(InstrPtr);
  }

  while (!Queue.empty()) {
    PrintIns(Queue.top());
    Queue.pop();
  }
}

