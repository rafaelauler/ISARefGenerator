
extern "C" {
#include "acpp.h"
}

#include "ISARef.h"
#include <iostream>
#include <cstdint>

int
main(int argc, char **argv) {
  acppInit(0);

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <modelname.ac>\n";
    return -1;
  }

  if (!acppLoad(argv[1])) {
    std::cerr << "Could not open ISA input file: " << argv[1] << "\n";
    std::cerr << "Parser terminated unsuccessfully.\n";
    return -1;
  }

  if (acppRun()) {
    std::cerr << "Parser terminated unsuccessfully.\n";
    return -1;
  }
  acppUnload();

  if (isa_filename == nullptr) {
    std::cerr << "No ISA file defined";
  }

  if (!acppLoad(isa_filename)) {
    std::cerr << "Could not open ISA input file: " << isa_filename << "\n";
    std::cerr << "Parser terminated unsuccessfully.\n";
    return -1;
  }

  if (acppRun()) {
    std::cerr << "Parser terminated unsuccessfully for " << isa_filename
              << "\n";
  }

  if (wordsize == 0) {
    std::cerr << "Warning: No wordsize defined. Default value is 32 bits.\n";
    wordsize = 32;
  }

  ISARef Ref(format_ins_list, instr_list, wordsize);


  acppUnload();
  return 0;
}
