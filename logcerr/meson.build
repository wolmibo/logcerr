compiler = meson.get_compiler('cpp')

dependencies = []



# find a supported format library

code = '''
#include <format>
static_assert(__cpp_lib_format >= 201907L);
'''

if not compiler.compiles(code, args: '-std=c++20')
  dependencies += dependency('fmt')
  format_library = 'fmt'
else
  format_library = 'std'
endif
summary('format library', format_library)




sources = [
  'src/core.cpp',
  'src/format.cpp'
]

headers = [
  'include/logcerr/log.hpp',
]

include_directories = ['include', '.']



install_project = not meson.is_subproject() or get_option('install_as_subproject')

if meson.is_subproject()
  summary('install', install_project)
endif



logcerr = library(
  'logcerr',
  sources,
  dependencies:        dependencies,
  include_directories: include_directories,
  install:             install_project
)

if install_project
  install_headers(headers, subdir: 'logcerr')



  pkg = import('pkgconfig')
  pkg.generate(logcerr)
endif


logcerr_dep = declare_dependency(
  link_with:           logcerr,
  dependencies:        dependencies,
  include_directories: include_directories
)
