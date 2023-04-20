#include <logcerr/log.hpp>

#include <future>
#include <iostream>



int main() {
  logcerr::output_level(logcerr::severity::debug);
  logcerr::merge_after(2);
  logcerr::log("hello, world!");
  logcerr::log("debugging enabled:  {}", logcerr::debugging_enabled());
  logcerr::log("colors are enabled: {}", logcerr::is_colored());
  logcerr::debug("debugging is really enabled!");

  logcerr::log("the next entry is empty");
  logcerr::log("");
  logcerr::log("this entry has one trailing new line\n");
  logcerr::log("this entry has two trailing new lines\n\n");

  auto result = std::async(std::launch::async, [](){
    logcerr::thread_name("async");

    logcerr::log("this is a message from another thread");
  });

  logcerr::debug("this is a debug message");
  logcerr::verbose("this is a verbose message");
  logcerr::log("this is a log message");
  logcerr::warn("this is a warning");
  logcerr::error("this is an error");


  const size_t count = 30;
  for (size_t i = 0; i < count; i++) {
    logcerr::log("this message is repeated {} times", count);
  }

  logcerr::error("multiline exception:\nLorem ipsum dolor sit amet,\nadipisci eunt blablablabla...");
  logcerr::log("a repeated multiline entry:");
  for (size_t i = 0; i < count; ++i) {
    logcerr::error("multiline exception:\nLorem ipsum dolor sit amet,\nadipisci eunt blablablabla...");
  }

  const size_t interrupt  = 12;
  const size_t interrupt2 = 25;
  const size_t interrupt3 = 16;
  for (size_t i = 0; i < count; ++i) {
    logcerr::debug("this sequence of {} will be interrupted at {}, {}, and {}",
        count, interrupt, interrupt2, interrupt3);
    if (i + 1 == interrupt) {
      logcerr::interrupt_merging();
    } else if (i + 1 == interrupt2) {
      logcerr::print_raw_sync(std::cerr, "interrupting for an important message outside of log\n");
    } else if (i + 1 == interrupt3) {
      auto lock = logcerr::output_lock();
      std::cerr << "manually writing to cerr within a lock guard" << std::endl;
    }
  }

  result.get();
}
