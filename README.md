# logcerr

Unspectacular, `std::format` based, thread-safe logging to `stderr` for C++.

#### Features
  * (Optional) merging of successive, identical messages
  * (Optional) colored output on linux
  * Access to the output lock to mix log and custom write operations
  * No explicit initialization required

#### Limitations
  * Calling any function except `logcerr::debugging_enabled` before or after `main` is
    undefined behaviour

## Example

```cpp
#include <logcerr/log.hpp>
#include <thread>

int main() {
  logcerr::log("Hello, world!");

  std::jthread async{[](){
    logcerr::thread_name("async");

    std::this_thread::sleep_for(std::chrono::milliseconds(4));

    logcerr::error("This is an error from another thread");
  }};

  for (int i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    logcerr::log("This message will be repeated {} times", 10);
  }
}
```

Possible output:
```
[00:00:00.000 main] Hello, world!
[00:00:00.001 main] This message will be repeated 10 times
[00:00:00.004 main] This message will be repeated 10 times (x2)
[00:00:00.004 async] *[Error]* This is an error from another thread
[00:00:00.005 main] This message will be repeated 10 times
[00:00:00.012 main] This message will be repeated 10 times (x6)
```

## Build Requirements

* GCC 12 or higher
* [fmt](https://github.com/fmtlib/fmt) if the compiler does not provide
  `__cpp_lib_format`
