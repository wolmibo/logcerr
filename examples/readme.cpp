#include <logcerr/log.hpp>
#include <thread>



int main() {
  logcerr::log("Hello, world!");

  const std::jthread async{[](){
    logcerr::thread_name("async");

    std::this_thread::sleep_for(std::chrono::milliseconds(4));

    logcerr::error("This is an error from another thread");
  }};

  const int count = 10;
  for (int i = 0; i < count; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    logcerr::log("This message will be repeated {} times", count);
  }
}
