// See LICENSE for details.

#pragma once

#include <queue>
#include <list>

#include "callback.hpp"
#include "dinst.hpp"
#include "estl.hpp"
#include "iassert.hpp"
#include "mshr_entry.hpp"
#include "pool.hpp"
#include "stats.hpp"

class MemRequest;

class MSHR {
private:
protected:
  const std::string name;
  const uint32_t    Log2LineSize;
  const int32_t     nEntries;
  const int32_t     nSubEntries;

  int32_t nFreeEntries;

  Stats_avg avgUse;
  Stats_avg avgSubUse;

  Addr_t calcLineAddr(Addr_t addr) const { return addr >> Log2LineSize; }

  Stats_cntr nStallConflict;

  const int32_t MSHRSize;
  const int32_t MSHRMask;

  // If a non-integer type is defined, the MSHR template should accept
  // a hash function as a parameter

  // Running crafty, I verified that the current hash function
  // performs pretty well (due to the extra space on MSHRSize). It
  // performs only 5% worse than an oversize prime number (noise).
  uint32_t calcEntry(Addr_t paddr) const {
    uint64_t p = paddr >> Log2LineSize;
    return (p ^ (p >> 11)) & MSHRMask;
  }

  class EntryType {
  public:
    CallbackContainer cc;
    Time_t startClock;
    int32_t           nUse;
#ifndef NDEBUG
    std::deque<MemRequest *> pending_mreq;
    MemRequest              *block_mreq;
#endif
  };

  std::vector<std::list<EntryType>> entry;

public:
  MSHR(const std::string &name, int32_t size, int16_t lineSize, int16_t nSubEntries);
  virtual ~MSHR() {}
  bool hasFreeEntries() const { return (nFreeEntries > 0); }

  bool canAccept(Addr_t paddr, Time_t startClock) const;
  //bool canIssue(Addr_t addr, Addr_t pc) const;
  bool canIssue(Addr_t addr, Time_t startClock) const;
  void addEntry(Addr_t addr, CallbackBase *c, MemRequest *mreq);
  void blockEntry(Addr_t addr, MemRequest *mreq);
  bool retire(Addr_t addr, MemRequest *mreq);
  void dump() const;
};
