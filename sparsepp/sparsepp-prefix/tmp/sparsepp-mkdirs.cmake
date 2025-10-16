# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-src")
  file(MAKE_DIRECTORY "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-src")
endif()
file(MAKE_DIRECTORY
  "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-build"
  "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-prefix"
  "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-prefix/tmp"
  "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-prefix/src/sparsepp-stamp"
  "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-prefix/src"
  "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-prefix/src/sparsepp-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-prefix/src/sparsepp-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-prefix/src/sparsepp-stamp${cfgdir}") # cfgdir has leading slash
endif()
