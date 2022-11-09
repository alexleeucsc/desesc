// See LICENSE for details.

#pragma once

//#define WAVESNAP_EN

#include <stdint.h>

// Generic Processor Interface.
//
// This class is a generic interface for Processors. It has been
// design for Traditional and SMT processors in mind. That's the
// reason why it manages the execution engine (RDEX).

#include "Cluster.h"
#include "ClusterManager.h"
#include "FastQueue.h"
#include "LSQ.h"
#include "Pipeline.h"
#include "Prefetcher.h"
#include "Resource.h"
#include "SCB.h"
#include "estl.h"

#include "callback.hpp"
#include "emul_base.hpp"
#include "stats.hpp"
#include "iassert.hpp"
#include "instruction.hpp"
#include "snippets.hpp"
#include "wavesnap.hpp"

class GMemorySystem;
class BPredictor;


class GProcessor {
private:
protected:
  // Per instance data
  const uint32_t cpu_id;

  const int32_t FetchWidth;
  const int32_t IssueWidth;
  const int32_t RetireWidth;
  const int32_t RealisticWidth;
  const int32_t InstQueueSize;
  const size_t  MaxROBSize;

  Hartid_t         maxFlows;
  Emul_base     *eint;
  GMemorySystem *memorySystem;

  StoreSet           storeset;
  Prefetcher         prefetcher;

  std::unique_ptr<SCB> scb;

  FastQueue<Dinst *> rROB;  // ready/retiring/executed ROB
  FastQueue<Dinst *> ROB;

  uint32_t smt;      // 1...
  uint32_t smt_ctx;  // 0... smt_ctx = cpu_id % smt

  bool active;

  // BEGIN  Statistics
  std::array<std::unique_ptr<Stats_cntr>,MaxStall> nStall;
  std::array<std::unique_ptr<Stats_cntr>,iMAX    > nInst;

  // OoO Stats
  Stats_avg  rrobUsed;
  Stats_avg  robUsed;
  Stats_avg  nReplayInst;
  Stats_cntr nCommitted;  // committed instructions

  // "Lack of Retirement" Stats
  Stats_cntr noFetch;
  Stats_cntr noFetch2;

  Stats_cntr nFreeze;
  Stats_cntr clockTicks;

  static Time_t      lastWallClock;
  Time_t             lastUpdatedWallClock;
  Time_t             activeclock_start;
  Time_t             activeclock_end;
  static inline Stats_cntr wallclock("OS:wallclock");

  // END Statistics
  float    throttlingRatio;
  uint32_t throttling_cntr;

  uint64_t lastReplay;

  // Construction
  void buildInstStats(const std::string &txt);
  void buildUnit(const std::string &clusterName, GMemorySystem *ms, Cluster *cluster, Opcode type);
  void buildCluster(const std::string &clusterName, GMemorySystem *ms);
  void buildClusters(GMemorySystem *ms);

  GProcessor(GMemorySystem *gm, CPU_t i);
  int32_t issue(PipeQueue &pipeQ);

  virtual void retire();

  virtual void       fetch(Hartid_t fid)     = 0;
  virtual StallCause addInst(Dinst *dinst) = 0;

public:
#ifdef WAVESNAP_EN
  std::unique_ptr<Wavesnap> snap;
#endif
  virtual ~GProcessor();
  int         getID() const { return cpu_id; }

  GMemorySystem *getMemorySystem() const { return memorySystem; }
  virtual void   executing(Dinst *dinst) = 0;
  virtual void   executed(Dinst *dinst)  = 0;
  virtual LSQ   *getLSQ()                = 0;
  virtual bool   isFlushing()            = 0;
  virtual bool   isReplayRecovering()    = 0;
  virtual Time_t getReplayID()           = 0;

  virtual void replay(Dinst *target){ (void)target; };  // = 0;

  bool isROBEmpty() const { return (ROB.empty() && rROB.empty()); }
  int  getROBsize() const { return (ROB.size() + rROB.size()); }
  bool isROBEmptyOnly() const { return ROB.empty(); }

  int getROBSizeOnly() const { return ROB.size(); }

  uint32_t getIDFromTop(int position) const { return ROB.getIDFromTop(position); }
  Dinst   *getData(uint32_t position) const { return ROB.getData(position); }

  void drain() { retire(); }

  // Returns the maximum number of flows this processor can support
  Hartid_t getMaxFlows(void) const { return maxFlows; }

  void report(const std::string &str);

  // Different types of cores extend this function. See SMTProcessor and
  // Processor.
  virtual bool advance_clock(Hartid_t fid) = 0;

  void set_emul(Emul_base *e) { eint = e; }

  void freeze(Time_t nCycles) {
    nFreeze.add(nCycles);
    clockTicks.add(nCycles);
  }

  void setActive() { active = true; }
  void clearActive() {
    I(isROBEmpty());
    active = false;
  }
  bool isActive() const { return active; }

  void setWallClock(bool en = true) {
    // FIXME: Periods of no fetch do not advance clock.

    trackactivity();

    if (lastWallClock == globalClock || !en)
      return;

    lastWallClock = globalClock;
    wallclock.inc(en);
  }
  static Time_t getWallClock() { return lastWallClock; }

  void trackactivity() {
    if (activeclock_end == (lastWallClock - 1)) {
    } else {
      if (activeclock_start != activeclock_end) {
        // MSG("\nCPU[%d]\t%lld\t%lld\n"
        //    ,cpu_id
        //    ,(long long int) activeclock_start
        //    ,(long long int) activeclock_end
        //    );
      }
      activeclock_start = lastWallClock;
    }
    activeclock_end = lastWallClock;
  }

  void dumpactivity() {
    // MSG("\nCPU[%d]\t%lld\t%lld\n"
    //    ,cpu_id
    //    ,(long long int) activeclock_start
    //    ,(long long int) activeclock_end
    //   );
  }

  StoreSet           *getSS() { return &storeset; }
  Prefetcher         *getPrefetcher() { return &prefetcher; }
  FastQueue<Dinst *> *getROB() { return &ROB; }
  SCB                *getSCB() { return scb; }
};
