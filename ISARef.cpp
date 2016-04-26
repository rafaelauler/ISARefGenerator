#include "ISARef.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <locale>
#include <set>
#include <stack>
#include <streambuf>
#include <string>

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

int FindDecVal(const ac_dec_list *DecList, int FieldId) {
  for (const ac_dec_list *DecPtr = DecList; DecPtr != nullptr;
       DecPtr = DecPtr->next) {
    if (DecPtr->id != FieldId)
      continue;
    return DecPtr->value;
  }
  return -1;
}

void PrintBin(std::ostream &O, uint32_t Num, uint32_t Len) {
  std::vector<char> Vec;

  do {
    Vec.push_back((Num %2) ? '1' : '0');
    Num >>= 1;
  } while (Num);

  std::reverse(Vec.begin(), Vec.end());
  uint32_t NumPrint = Vec.size();
  while (NumPrint < Len) {
    O << '0';
    ++NumPrint;
  }
  for (const auto Ch : Vec)
    O << Ch;
}

namespace {

// trim from start (in place)
inline void ltrim(std::string &s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace)))
              .base(),
          s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}
}

void ISARef::ReadISACppFile(const std::string &FileName) {
  std::ifstream IF(FileName);
  if (!IF)
    return;

  std::string ISAFile;
  IF.seekg(0, IF.end);
  ISAFile.reserve(IF.tellg());
  IF.seekg(0, IF.beg);
  ISAFile.assign((std::istreambuf_iterator<char>(IF)),
                 std::istreambuf_iterator<char>());
  IF.close();

  std::string::size_type Pos = 0;

  while ((Pos = ISAFile.find("ac_behavior", Pos)) != std::string::npos) {
    const auto LParenPos = ISAFile.find_first_of("(", Pos) + 1;
    const auto RParenPos = ISAFile.find_first_of(")", Pos);
    std::string InstrName = ISAFile.substr(LParenPos, RParenPos - LParenPos);
    trim(InstrName);

    Pos = ISAFile.find_first_of("{", RParenPos);
    const auto CodeStart = Pos;
    int NumBrackets = 1;
    while (NumBrackets) {
      Pos = ISAFile.find_first_of("{}", Pos + 1);
      if (Pos == std::string::npos)
        break;
      if (ISAFile[Pos] == '{') {
        ++NumBrackets;
        continue;
      }
      --NumBrackets;
    }
    std::string Impl = ISAFile.substr(CodeStart + 1, Pos - CodeStart - 1);
    trim(Impl);
    InstrImpls[InstrName] = Impl;
  }
}

void ISARef::PrintIns(ac_dec_instr *Instr) {
  std::cout << "\\section{" << Instr->mnemonic << "}\n\n";

  ac_dec_format *Fmt = FindFormat(FormatList, Instr->format);

  constexpr float BitWidth = .5;

  std::cout
      << "% Draw the instruction encoding diagram\n"
      << "\\begin{tikzpicture}\n"
      << "\\draw[fill=white]";

  float CurX = .0;
  bool High = false;

  std::cout << "% Draw boxes and their contents\n";
  for (ac_dec_field *FieldPtr = Fmt->fields; FieldPtr != nullptr;
       FieldPtr = FieldPtr->next) {
    const float Width = FieldPtr->size * BitWidth;
    std::cout << "(" << CurX << "," << (High? "1.5" : "0.0") << ") rectangle"
              << " node [align=center] {";
    std::cout << FieldPtr->name;
    const int Val = FindDecVal(Instr->dec_list, FieldPtr->id);
    if (Val >= 0) {
      std::cout << "\\\\";
      PrintBin(std::cout, Val, FieldPtr->size);
    }
    std::cout << "}\n";
    CurX += Width;
    High = !High;
  }
  std::cout << "(" << CurX << "," << (High ? "1.5" : "0.0") << ");\n";

  std::cout << "% Draw bit numbers\n";
  CurX = .0;
  int CurBit = 31;
  for (ac_dec_field *FieldPtr = Fmt->fields; FieldPtr != nullptr;
       FieldPtr = FieldPtr->next) {
    const float Width = FieldPtr->size * BitWidth;
    std::cout << "\\draw (" << CurX
              << ", 1.7) node [anchor=west, font=\\tiny] {" << CurBit
              << "};\n";
    if (FieldPtr->size > 1)
      std::cout << "\\draw (" << (CurX + Width)
                << ", 1.7) node [anchor=east, font=\\tiny] {"
                << (CurBit - FieldPtr->size + 1) << "};\n";
    std::cout << "\\draw (" << (CurX + Width / 2)
              << ", -0.25) node [anchor=center, font=\\tiny] {" << FieldPtr->size
              << "};\n";
    CurX += Width;
    CurBit -= FieldPtr->size;
  }
  std::cout << "\\end{tikzpicture}\n\n";
  std::cout << "\\vspace{1cm}\n";
  std::cout << "\\textbf{Encoding Format}: " << Instr->format << "\n\n";

  std::cout << "\\vspace{1cm}\n";
  std::cout << "\\textbf{Assembly Language Syntax}: \\verb|" << Instr->asm_str
            << "|\n\n";

  if (InstrImpls.count(Instr->name)) {
    std::cout << "\\vspace{1cm}\n";
    std::cout << "\\textbf{Operation (C Code)}:\n\n\\begin{verbatim}\n";
    const std::string &Str = InstrImpls[Instr->name];
    std::string::size_type Pos = 0;
    while (Pos != std::string::npos) {
      auto LinePos = Str.find_first_of("\n", Pos);
      std::string Line;
      if (LinePos == std::string::npos) {
        Line = Str.substr(Pos);
        Pos = LinePos;
      } else {
        Line = Str.substr(Pos, LinePos - Pos);
        Pos = LinePos + 1;
      }
      if (Line.find("dbg") != std::string::npos)
        continue;
      std::cout << "    " << Line << "\n";
    }
    std::cout << "\n\\end{verbatim}\n\n";
  }

  std::cout << "\\clearpage\n";
  std::cout << "---------------------------------------------------------------"
               "-----------------\n\n";
}

ISARef::ISARef(ac_dec_format *FormatList, ac_dec_instr *InstrList,
               const char *ArchName)
    : FormatList(FormatList) {

  // Populate field ids
  CreateDecoder(FormatList, InstrList);
  std::string ISAFileName = ArchName;
  ISAFileName.replace(ISAFileName.size() - 2, 3, "cpp");
  ReadISACppFile(ISAFileName);

  // Alphabetical ordering of instructions
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
