// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#include "logcerr/log.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <sstream>
#include <vector>

#include <unistd.h>




namespace {
  [[nodiscard]] bool determine_colored() {
    return isatty(STDERR_FILENO) != 0;
  }
}





namespace { namespace global_state {
  auto                             start  {std::chrono::high_resolution_clock::now()};

  std::atomic<logcerr::color_mode> color  {logcerr::color_mode::auto_detect};
  std::atomic<bool>                colored{determine_colored()};

  std::atomic<logcerr::severity>   level  {logcerr::severity::log};

  std::atomic<size_t>              merge  {2};

  std::mutex                       thread_names_mutex;
  std::vector<std::thread::id>     thread_ids  {std::this_thread::get_id()};
  std::vector<std::string>         thread_names{"main"};
}}





namespace {
  [[nodiscard]] std::string to_string(std::thread::id thread_id) {
    std::stringstream stream;
    stream << thread_id;
    return stream.str();
  }



  [[nodiscard]] std::optional<size_t> find_thread_index_unguarded(std::thread::id t_id)
  {
    auto iter = std::ranges::find(global_state::thread_ids, t_id);
    if (iter == global_state::thread_ids.end()) {
      return {};
    }

    return iter - global_state::thread_ids.begin();
  }
}





bool logcerr::is_colored() noexcept {
  return global_state::colored;
}

void logcerr::colorize(color_mode mode) {
  switch (mode) {
    case color_mode::never:
      global_state::colored = false;
      break;
    case color_mode::auto_detect:
      global_state::colored = determine_colored();
      break;
    case color_mode::always:
      global_state::colored = true;
      break;
    default:
      throw std::invalid_argument{"expected a vaild color mode"};
  }
  global_state::color = mode;
}

logcerr::color_mode logcerr::colorize() noexcept {
  return static_cast<logcerr::color_mode>(global_state::color.load());
}





void logcerr::output_level(severity lowest_level) noexcept {
  global_state::level = lowest_level;
}

logcerr::severity logcerr::output_level() noexcept {
  return global_state::level.load();
}

bool logcerr::is_outputted(severity lev) noexcept {
  return static_cast<std::underlying_type_t<severity>>(lev) >=
    static_cast<std::underlying_type_t<severity>>(output_level());
}





void logcerr::merge_after(size_t merge) noexcept {
  global_state::merge = merge;
}

size_t logcerr::merge_after() noexcept {
  return global_state::merge;
}





void logcerr::thread_name(std::string_view name, std::thread::id thread_id) {
  const std::lock_guard<std::mutex> lock{global_state::thread_names_mutex};

  if (auto index = find_thread_index_unguarded(thread_id)) {
    global_state::thread_names[*index] = name;

  } else {
    global_state::thread_names.emplace_back(name);
    global_state::thread_ids.emplace_back(thread_id);
  }
}



std::string logcerr::thread_name(std::thread::id thread_id) {
  const std::lock_guard<std::mutex> lock{global_state::thread_names_mutex};

  if (auto index = find_thread_index_unguarded(thread_id)) {
    return global_state::thread_names[*index];
  }

  global_state::thread_names.emplace_back(to_string(thread_id));
  global_state::thread_ids.emplace_back(thread_id);

  return global_state::thread_names.back();
}





std::chrono::milliseconds logcerr::elapsed() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::high_resolution_clock::now() - global_state::start);
}
