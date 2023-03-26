compiler = meson.get_compiler('cpp')

dependencies = []



# find a supported format library

code = '''
#include <format>
static_assert(__cpp_lib_format >= 201907L);
'''

if not compiler.compiles(code)
  dependencies += dependency('fmt')
  format_library = 'fmt::format'
else
  format_library = 'std::format'
endif
summary('logcerr::format', format_library)




sources = [
  'src/core.cpp',
  'src/format.cpp'
]

headers = [
  'include/logcerr/log.hpp',
]

include_directories = ['include', '.']



logcerr = library(
  'logcerr',
  sources,
  dependencies:        dependencies,
  include_directories: include_directories,
  install:             true
)

install_headers(headers, subdir: 'logcerr')



pkg = import('pkgconfig')
pkg.generate(logcerr)



logcerr_dep = declare_dependency(
  link_with:           logcerr,
  dependencies:        dependencies,
  include_directories: include_directories
)
