cmake_minimum_required(VERSION 4.3.4 FATAL_ERROR)

# loom: a Matrix client, as C++26 modules -- sans I/O: what to send made from
# typed requests, and what comes back read straight into types with knot. The
# HTTP is the caller's.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/import-std.cmake)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_EXTENSIONS OFF)

project(loom VERSION 0.1 LANGUAGES CXX)

option(LOOM_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})

if(NOT COMMAND cme_find_package)
  set(CME_WITHOUT_PROVIDER ON)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/get_cme.cmake)
endif()

# knot, for the JSON: a cme port, pinned to a commit, fetched from GitHub.
# It is private, so git has to be able to authenticate. A checkout is used
# instead with -DCPM_knot_SOURCE=<path>.
cme_declare_port(NAME knot PROVIDES knot
  GITHUB_REPOSITORY j4niwzis/knot
  GIT_TAG 3ba3b6fb14fcafb21753371752af6b4567d23b9b
  LICENSE LicenseRef-proprietary
  SYSTEM NEVER
  TARGETS knot)
cme_options(knot "KNOT_TESTS OFF")
cme_find_package(knot REQUIRED)

set(LOOM_CONSTEXPR_STEPS 300000000 CACHE STRING
  "clang's -fconstexpr-steps for loom and what imports it")

# One module to a target, as in knot, chevron and tern.
function(loom_module target file)
  add_library(${target})
  target_sources(${target}
    PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES ${file})
  target_compile_features(${target} PUBLIC cxx_std_26)
  if(ARGN)
    target_link_libraries(${target} PUBLIC ${ARGN})
  endif()
  target_compile_options(${target} PUBLIC
    $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-fconstexpr-steps=${LOOM_CONSTEXPR_STEPS}>)
endfunction()

loom_module(loom_ids src/ids.cc)
loom_module(loom_events src/events.cc knot)
loom_module(loom_sync src/sync.cc loom_events knot)
loom_module(loom_api src/api.cc loom_ids loom_events loom_sync knot)
loom_module(loom src/loom.cc loom_ids loom_events loom_sync loom_api)

add_library(loom::ids ALIAS loom_ids)
add_library(loom::events ALIAS loom_events)
add_library(loom::sync ALIAS loom_sync)
add_library(loom::api ALIAS loom_api)
add_library(loom::loom ALIAS loom)

if(LOOM_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
