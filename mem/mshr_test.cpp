#include "gmock/gmock.h"
#include "gtest/gtest.h"

// This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "mshr.hpp"

#include <cstdio>
#include <exception>

#include "fmt/format.h"
#include "iassert.hpp"

#include "config.hpp"
#include "report.hpp"
#include "ccache.hpp"
#include "dinst.hpp"
#include "gprocessor.hpp"
#include "instruction.hpp"
#include "memobj.hpp"
#include "memrequest.hpp"
#include "memstruct.hpp"
#include "memory_system.hpp"
#include "callback.hpp"

#include <fstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

Gmemory_system *myGMS    = 0;

static void setup_config() {
  std::ofstream file;

  file.open("cachecore.toml");

  file <<
"[soc]\n"
"core = [\"c0\",\"c0\"]\n"
"[c0]\n"
"type  = \"ooo\"\n"
"caches        = true\n"
"dl1           = \"dl1_cache DL1\"\n"
"il1           = \"dl1_cache IL1\"\n"
"[dl1_cache]\n"
"type       = \"cache\"\n"
"cold_misses = true\n"
"size       = 32768\n"
"line_size  = 64\n"
"delay      = 5\n"
"miss_delay = 2\n"
"assoc      = 4\n"
"repl_policy = \"lru\"\n"
"port_occ   = 1\n"
"port_num   = 1\n"
"port_banks = 32\n"
"send_port_occ = 1\n"
"send_port_num = 1\n"
"max_requests  = 32\n"
"allocate_miss = true\n"
"victim        = false\n"
"coherent      = true\n"
"inclusive     = true\n"
"directory     = false\n"
"nlp_distance = 2\n"
"nlp_degree   = 1       # 0 disabled\n"
"nlp_stride   = 1\n"
"drop_prefetch = true\n"
"prefetch_degree = 0    # 0 disabled\n"
"mega_lines1K    = 8    # 8 lines touched, triggers mega/carped prefetch\n"
"lower_level = \"privl2 L2 sharedby 2\"\n"
"[privl2]\n"
"type       = \"cache\"\n"
"cold_misses = true\n"
"size       = 1048576\n"
"line_size  = 64\n"
"delay      = 13\n"
"miss_delay = 7\n"
"assoc      = 4\n"
"repl_policy = \"lru\"\n"
"port_occ   = 1\n"
"port_num   = 1\n"
"port_banks = 32\n"
"send_port_occ = 1\n"
"send_port_num = 1\n"
"max_requests  = 32\n"
"allocate_miss = true\n"
"victim        = false\n"
"coherent      = true\n"
"inclusive     = true\n"
"directory     = false\n"
"nlp_distance = 2\n"
"nlp_degree   = 1       # 0 disabled\n"
"nlp_stride   = 1\n"
"drop_prefetch = true\n"
"prefetch_degree = 0    # 0 disabled\n"
"mega_lines1K    = 8    # 8 lines touched, triggers mega/carped prefetch\n"
"lower_level = \"l3 l3 shared\"\n"
"[l3]\n"
"type       = \"nice\"\n"
"line_size  = 64\n"
"delay      = 31\n"
"cold_misses = false\n"
"lower_level = \"\"\n"
    ;

  file.close();
}

void            initialize() {
  setup_config();
  Report::init();
  Config::init("cachecore.toml");
  myGMS = new Memory_system(0);
}

class MSHR_test : public ::testing::Test {
protected:
  std::shared_ptr<MSHR> mshr_obj;
  void SetUp() override {
    initialize();
    setup_config();
    myGMS -> build_memory_system();
    //mock cache
    int32_t size = 64;
    int16_t lineSize = 8;
    int16_t nSubEntries = 2;
    mshr_obj = std::make_shared<MSHR>("mshrtest",size,lineSize,nSubEntries);
    //CCache ccache_obj = new CCache( myGMS, "section", "name" );
    //CCache ccache_obj = myGMS->getDL1();
    MemObj* ccache_obj = myGMS->getDL1();
  }

  void TearDown() override {
    // Graph_library::sync_all();
  }
};

TEST_F(MSHR_test, trivial) {
    //test seq 1: send req at addr 16,
    
    //send a request
    MemRequest* req1 = new MemRequest();
    req1 -> startClock = 0;
    //can_accept
    mshr_obj->blockEntry(16,req1);

}
