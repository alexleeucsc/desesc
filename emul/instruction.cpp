// See LICENSE for details.

#include "instruction.hpp"

#include "fmt/format.h"
#include "iassert.hpp"

static const char *opcode2NameTable[] = {
    "iSubInvalid",
    //-----------------
    "iRALU",
    //-----------------
    "iAALU",
    //-----------------
    "iBALU_LBRANCH",
    "iBALU_RBRANCH",
    "iBALU_LJUMP",
    "iBALU_RJUMP",
    "iBALU_LCALL",
    "iBALU_RCALL",
    "iBALU_RET",
    //-----------------
    "iLALU_LD",
    //-----------------
    "iSALU_ST",
    "iSALU_LL",
    "iSALU_SC",
    "iSALU_ADDR",
    //-----------------
    "iCALU_FPMULT",
    "iCALU_FPDIV",
    "iCALU_FPALU",
    "iCALU_MULT",
    "iCALU_DIV",
    //-----------------
};

const char *Instruction::opcode2Name(Opcode op) { return opcode2NameTable[op]; }

void Instruction::dump() const { fmt::print(get_asm()); }

std::string Instruction::get_asm() const {
  if (dst2 != LREG_InvalidOutput) {
    if (src2 == LREG_R0) {
      return fmt::format("r{}, r{} = {} r{}", dst1, dst2, opcode2NameTable[opcode], src1);
    }

    return fmt::format("r{}, r{} = {} r{} r{}", dst1, dst2, opcode2NameTable[opcode], src1, src2);
  }

  if (src2 == LREG_R0) {
    return fmt::format("r{} = {} r{}", dst1, opcode2NameTable[opcode], src1);
  }

  return fmt::format("r{} = {} r{} r{}", dst1, opcode2NameTable[opcode], src1, src2);
}

void Instruction::set(Opcode opcode_, RegType src1_, RegType src2_, RegType dst1_, RegType dst2_) {
  I(opcode_ != iOpInvalid);

  opcode = opcode_;
  src1   = src1_;
  src2   = src2_;
  if (dst1_ == LREG_NoDependence) {
    dst1 = LREG_InvalidOutput;
  } else {
    dst1 = dst1_;
  }
  if (dst2_ == LREG_NoDependence) {
    dst2 = LREG_InvalidOutput;
  } else {
    dst2 = dst2_;
  }
}
