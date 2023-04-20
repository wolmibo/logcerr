// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#include "logcerr/log.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <span>
#include <vector>





namespace { namespace global_state {
  std::mutex output_mutex;

  bool       last_message_returned{false}; // guarded by output_mutex



  class terminator_t {
    public:
      terminator_t(const terminator_t&) = delete;
      terminator_t(terminator_t&&)      = delete;
      terminator_t& operator=(const terminator_t&) = delete;
      terminator_t& operator=(terminator_t&&)      = delete;

      terminator_t() = default;

      ~terminator_t() {
        logcerr::interrupt_merging();
      }
  } terminator;
}}





namespace {
  [[nodiscard]] std::vector<std::string_view> split(std::string_view input) {
    if (input.empty()) {
      return {input};
    }

    std::vector<std::string_view> output;

    while (true) {
      auto pos = input.find('\n');
      output.emplace_back(input.substr(0, pos));

      if (pos == std::string_view::npos) {
        break;
      }

      input.remove_prefix(pos + 1);
    }

    while (output.size() > 1 && output.back().empty()) {
      output.pop_back();
    }

    return output;
  }





  void write(std::ostream& out, std::string_view message) {
    out.write(message.data(), std::ssize(message));
  }





  [[nodiscard]] std::string format_extra(
      bool              use_color,
      logcerr::severity level,
      std::string_view  message,
      std::string_view  terminal
  ) {
    using namespace logcerr::impl::format;

    if (!use_color) {
      return format("\r  | {}", message);
    }

    switch (level) {
      case logcerr::severity::error:
        return format("\r  \x1b[2m|\x1b[0m \x1b[1m{}{}\x1b[0m", message, terminal);
      case logcerr::severity::debug:
        return format("\r  \x1b[2m| {}{}\x1b[0m", message, terminal);
      default:
        return format("\r  \x1b[2m|\x1b[0m {}{}", message, terminal);
    }
  }





  [[nodiscard]] std::array<long, 4> h_min_sec_ms(std::chrono::milliseconds time) {
    const long millis{time.count()};

    static constexpr long minute_s{60};
    static constexpr long hour_min{60};
    static constexpr long second_ms{1000};

    static constexpr long minute_ms{minute_s * second_ms};
    static constexpr long hour_ms  {hour_min * minute_ms};

    return {
      (millis / hour_ms),
      (millis / minute_ms) % hour_min,
      (millis / second_ms) % minute_s,
      millis               % second_ms
    };
  }



  [[nodiscard]] std::string format_main(
      bool                      use_color,
      logcerr::severity         level,
      std::chrono::milliseconds time,
      std::string_view          thread,
      std::string_view          message,
      std::string_view          terminal
  ) {
    using namespace logcerr::impl::format;

    const auto [h, m, s, ms] = h_min_sec_ms(time);

    if (!use_color) {
      switch (level) {
        case logcerr::severity::warning:
          return format("\r[{:02}:{:02}:{:02}.{:03} {}] [Warning] {}{}",
                          h, m, s, ms, thread, message, terminal);

        case logcerr::severity::error:
          return format("\r[{:02}:{:02}:{:02}.{:03} {}] *[Error]* {}{}",
                          h, m, s, ms, thread, message, terminal);

        default:
          return format("\r[{:02}:{:02}:{:02}.{:03} {}] {}{}",
                          h, m, s, ms, thread, message, terminal);
      }
    }


    switch (level) {
      case logcerr::severity::debug:
        return format("\r\x1b[2m[{:02}:{:02}:{:02}.{:03} {}] {}{}\x1b[0m",
                        h, m, s, ms, thread, message, terminal);

      case logcerr::severity::warning:
        return format("\r[{:02}:{:02}:{:02}.{:03} {}] \x1b[33m[Warning]\x1b[0m {}{}",
                        h, m, s, ms, thread, message, terminal);

      case logcerr::severity::error:
        return format("\r\x1b[1m[{:02}:{:02}:{:02}.{:03} {}] "
                        "\x1b[31m*[Error]*\x1b[39m {}{}\x1b[0m",
                        h, m, s, ms, thread, message, terminal);

      default:
        return format("\r[{:02}:{:02}:{:02}.{:03} {}] {}{}",
                        h, m, s, ms, thread, message, terminal);
    }
  }





  void print_message_unguarded(
    logcerr::severity                 level,
    std::span<const std::string_view> lines,
    std::string_view                  thread_name,// NOLINT(*easily-swappable-parameters)
    std::string_view                  terminal
  ) {
    const bool colored = logcerr::is_colored();

    for (auto it = lines.begin(); it != lines.end(); ++it) {
      auto term = (it + 1) == lines.end() ? terminal : std::string_view{"\n"};

      if (it == lines.begin()) {
        write(std::cerr, format_main(colored, level,
                                       logcerr::elapsed(), thread_name, *it, term));
      } else {
        write(std::cerr, format_extra(colored, level, *it, term));
      }
    }
  }





  class entry {
    public:
      entry(const entry&) = delete;

      entry(entry&& rhs) noexcept :
        level      {rhs.level},
        message    {std::move(rhs.message)},
        thread_name{std::move(rhs.thread_name)},
        lines      {split(message)}
      {}

      entry& operator=(const entry&) = delete;

      entry& operator=(entry&& rhs) noexcept {
        if (*this == rhs) {
          count++;
        } else {
          level       = rhs.level;
          message     = std::move(rhs.message);
          thread_name = std::move(rhs.thread_name);
          lines       = split(message);
          count = 1;
        }

        return *this;
      }

      ~entry() = default;

      entry(logcerr::severity lev, std::string&& msg) :
        level      {lev},
        message    {std::move(msg)},
        thread_name{logcerr::thread_name()},
        lines      {split(message)}
      {}



      [[nodiscard]] bool operator==(const entry& rhs) const {
        return level     == rhs.level
          && message     == rhs.message
          && thread_name == rhs.thread_name;
      };



      void print_unguarded(size_t merge) const {
        if (count <= merge) {
          if (global_state::last_message_returned) {
            std::cerr.put('\n');
          }

          print_message_unguarded(level, lines, thread_name, "");
        } else {
          if (lines.size() == 1) {
            write(std::cerr, format_main(logcerr::is_colored(), level,
                                        logcerr::elapsed(), thread_name, lines.front(),
                                        format_counter(merge)));
          } else if (lines.size() > 1) {
            write(std::cerr, format_extra(logcerr::is_colored(), level,
                                        lines.back(), format_counter(merge)));
          }
        }

        global_state::last_message_returned = true;
      }



    private:
      logcerr::severity level;
      std::string       message;
      std::string       thread_name;
      size_t            count{1};

      std::vector<std::string_view> lines;



      [[nodiscard]] std::string format_counter(size_t coalesce) const {
        return logcerr::format(" (x{})", count + 1 - coalesce);
      }
  };
}



namespace { namespace global_state {
  std::optional<entry> last_message; // guarded by output_mutex
}}





namespace {
  void interrupt_merging_unguarded() {
    global_state::last_message = {};
    if (global_state::last_message_returned) {
      std::cerr.put('\n');
      global_state::last_message_returned = false;
    }
  }



  void basic_print(logcerr::severity level, std::string&& message) {
    const std::lock_guard<std::mutex> lock{global_state::output_mutex};

    if (auto merge = logcerr::merge_after(); merge > 0) {
      global_state::last_message = entry{level, std::move(message)};
      global_state::last_message->print_unguarded(merge);
    } else {
      print_message_unguarded(level, split(message), logcerr::thread_name(), "\n");
      global_state::last_message_returned = false;
    }
  }
}





void logcerr::impl::print(severity level, std::string&& message) {
  basic_print(level, std::move(message));
}

void logcerr::impl::print_checked(severity level, std::string_view message) {
  if (is_outputted(level)) {
    basic_print(level, std::string{message});
  }
}





void logcerr::interrupt_merging() {
  const std::lock_guard<std::mutex> lock{global_state::output_mutex};

  interrupt_merging_unguarded();
}



void logcerr::print_raw_sync(std::ostream& out, std::string_view message) {
  const std::lock_guard<std::mutex> lock{global_state::output_mutex};

  interrupt_merging_unguarded();

  out.write(message.data(), std::ssize(message));
}



std::unique_lock<std::mutex> logcerr::output_lock() {
  std::unique_lock<std::mutex> lock{global_state::output_mutex};

  interrupt_merging_unguarded();

  return lock;
}
