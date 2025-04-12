#include <utl/progress_tracker.h>

#include <filesystem>

#include "generated/benchmark_dir.h"
#include "gtest/gtest.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  std::clog.rdbuf(std::cout.rdbuf());

  auto const progress_tracker = utl::activate_progress_tracker("benchmark");
  auto const silencer = utl::global_progress_bars{true};
  fs::current_path(BENCHMARK_EXECUTION_DIR);

  ::testing::InitGoogleTest(&argc, argv);
  auto test_result = RUN_ALL_TESTS();

  return test_result;
}