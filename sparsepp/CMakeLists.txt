# code copied from https://crascit.com/2015/07/25/cmake-gtest/
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(sparsepp-download NONE)

include(ExternalProject)

ExternalProject_Add(
  sparsepp
  SOURCE_DIR "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-src"
  BINARY_DIR "/home/andrew/programming/c++/shumi-chess/sparsepp/sparsepp-build"
  GIT_REPOSITORY
    https://github.com/greg7mdp/sparsepp.git
  GIT_TAG
    master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  TEST_COMMAND ""
  )
