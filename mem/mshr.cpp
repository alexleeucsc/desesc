// See LICENSE for details.

#include "mshr.hpp"

#include "config.hpp"
#include "fmt/format.h"
#include "memory_system.hpp"
#include "memrequest.hpp"
#include "snippets.hpp"

#include <iostream>

MSHR::MSHR(const std::string &n, int32_t size, int16_t lineSize, int16_t nsub)
    : name(n)
    , Log2LineSize(log2i(lineSize))
    , nEntries(size)
    , nSubEntries(nsub)
    , avgUse(fmt::format("{}_MSHR_avgUse", n))
    , avgSubUse(fmt::format("{}_MSHR_avgSubUse", n))
    , nStallConflict(fmt::format("{}_MSHR:nStallConflict", name))
    , MSHRSize(roundUpPower2(size) * 4)
    , MSHRMask(MSHRSize - 1) {
  I(size > 0 && size < 1024 * 32 * 32);

  nFreeEntries = size;

  I(lineSize >= 0 && Log2LineSize < (8 * sizeof(Addr_t) - 1));

  entry.resize(MSHRSize);
  for (int i = 0; i < MSHRSize; i++) {
      entry[i].emplace_front();
  }
  for (int32_t i = 0; i < MSHRSize; i++) {
    entry[i].front().nUse = 0;
    I(entry[i].front().cc.empty());
  }
}

bool MSHR::canAccept(Addr_t addr, Time_t startClock) const {
  //not in ccahce
  //0303
  //if incoming request comes BEFORE ANY current entry in program order,
  //it must be able to preempt and go through
  uint32_t pos = calcEntry(addr);
  for(std::list<EntryType>::const_iterator it = entry[pos].begin(); it != entry[pos].end(); ++it){
    if ((nFreeEntries <= 0) & (startClock < it->startClock)) {
      return false;
    }
  }
  // if (entry[pos].nUse >= nSubEntries) {
  //   return false;
  // }

  return true;
}

bool MSHR::canIssue(Addr_t addr, Time_t startClock) const {
  uint32_t pos = calcEntry(addr);
  //0303
  //if incoming request comes BEFORE ANY current entry in program order,
  //it must be able to preempt and go through
  for(std::list<EntryType>::const_iterator it = entry[pos].begin(); it != entry[pos].end(); ++it){
    if ((it->nUse & startClock) > (it->startClock )) {
      return false;
    }
  }
  // if (entry[pos].nUse) {
  //   return true;
  // }

  //I(entry[pos].cc.empty());

  return true;
}

void MSHR::addEntry(Addr_t addr, CallbackBase *c, MemRequest *mreq) {
  printf("adding:");
  printf("%ld",addr);
  printf(" start at ");
  printf("%ld",mreq->startClock);
  printf(" || now at ");
  printf("%ld",globalClock);
  printf("\n");
  I(mreq->isRetrying());
  I(nFreeEntries <= nEntries);
  nFreeEntries--;  // it can go negative because invalidate and writeback requests

  avgUse.sample(nEntries - nFreeEntries, mreq->has_stats());

  uint32_t pos = calcEntry(addr);

  I(c);
  for(std::list<EntryType>::iterator it = entry[pos].begin(); it != entry[pos].end(); ++it){
    if(it->startClock < mreq->startClock){
      it->cc.add(c);

      I(nFreeEntries >= 0);

      //I(it->nUse);
      it->nUse++;
      avgSubUse.sample(it->nUse, mreq->has_stats());
#ifndef NDEBUG
      //I(!it->pending_mreq.empty());
      it->pending_mreq.push_back(mreq);
      //0303
      //iterate over entries, remove relevant mreq
#endif
      break;
    }
  }
  nStallConflict.inc();
}

void MSHR::blockEntry(Addr_t addr, MemRequest *mreq) {
  printf("blockEntry:");
  printf("%ld",addr);
  printf(" start at ");
  printf("%ld",mreq->startClock);
  printf(" || now at ");
  printf("%ld",globalClock);
  printf("\n");
  I(!mreq->isRetrying());
  I(nFreeEntries <= nEntries);
  nFreeEntries--;  // it can go negative because invalidate and writeback requests

  avgUse.sample(nEntries - nFreeEntries, mreq->has_stats());

  uint32_t pos = calcEntry(addr);
  I(nFreeEntries >= 0);

  //if entry[pos]'s front is occupied, that means canAccept passed bc the new mreq
  //is older and should pre-empt the existing entry
  if(entry[pos].front().nUse){
    I(entry[pos].front().nUse == 0);
    entry[pos].front().nUse++;
    entry[pos].front().startClock = mreq->startClock;
    avgSubUse.sample(entry[pos].front().nUse, mreq->has_stats());
#ifndef NDEBUG
    I(entry[pos].front().pending_mreq.empty());
    entry[pos].front().pending_mreq.push_back(mreq);
    entry[pos].front().block_mreq = mreq;
#endif
  }else{
    entry[pos].emplace_back();
    I(entry[pos].back().nUse == 0);
    entry[pos].back().nUse++;
    entry[pos].back().startClock = mreq->startClock;
    avgSubUse.sample(entry[pos].back().nUse, mreq->has_stats());
#ifndef NDEBUG
    I(entry[pos].back().pending_mreq.empty());
    entry[pos].back().pending_mreq.push_back(mreq);
    entry[pos].back().block_mreq = mreq;
#endif
  }
}

bool MSHR::retire(Addr_t addr, MemRequest *mreq) {
  printf("retire:");
  printf("%ld",addr);
  printf(" start at ");
  printf("%ld",mreq->startClock);
  printf(" || now at ");
  printf("%ld",globalClock);
  printf("\n");
  I(mreq);
  uint32_t pos = calcEntry(addr);
  bool mreqFound = false;
  for(std::list<EntryType>::iterator it = entry[pos].begin(); it != entry[pos].end(); ++it){
    if((it->startClock == mreq->startClock)){
      mreqFound = true;
      I(it->nUse);
#ifndef NDEBUG
      I(!it->pending_mreq.empty());
      I(it->pending_mreq.front() == mreq);
      it->pending_mreq.pop_front();
      if (!it->pending_mreq.empty()) {
        MemRequest *mreq2 = it->pending_mreq.front();
        if (mreq2 != it->block_mreq) {
          I(mreq2->isRetrying());
        } else {
          I(!mreq2->isRetrying());
        }
      }
      if (it->block_mreq) {
        I(mreq == it->block_mreq);
        it->block_mreq = 0;
      }
#endif

      nFreeEntries++;
      I(nFreeEntries <= nEntries);

      I(it->nUse);
      it->nUse--;

      I(nFreeEntries >= 0);

      GI(it->nUse == 0, it->cc.empty());

      if (!it->cc.empty()) {
        it->cc.callNext();
        return true;
      }
    }
  }
  if(!mreqFound){
    std::cout << "ERROR! tried to retire an entry not currently in MSHR!\n";
  }
  return false;
}

void MSHR::dump() const {
  fmt::print("MSHR[{}]", name);
  for (int i = 0; i < MSHRSize; i++) {
    for(std::list<EntryType>::const_iterator it = entry[i].begin(); it != entry[i].end(); ++it){
      if (it->nUse) {
        fmt::print(" [{}].nUse={}", i, it->nUse);
      }
      GI(it->nUse == 0, it->cc.empty());
      // GI(entry[i].cc.empty(), entry[i].nUse==0);
    }
  }
  fmt::print("\n");
}
