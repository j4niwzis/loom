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
  GIT_TAG 49039355f299e81a0c408c38d5cbf7b9b29b4e6f
  LICENSE AGPL-3.0-only
  SYSTEM NEVER
  TARGETS knot)
cme_options(knot "KNOT_TESTS OFF")
cme_find_package(knot REQUIRED)

set(LOOM_CONSTEXPR_STEPS 300000000 CACHE STRING
  "clang's -fconstexpr-steps for loom and what imports it")

# One module to a target, as in knot, chevron and tern.
# ---- One library, and what of it is built ----
#
# loom is one target, libloom.a, with its modules in one file set: modules
# are what separates it, and archives did not need to as well. What is
# built is chosen by component, the way a consumer asks for it:
#
#   cme_find_package(loom COMPONENTS state cs-login cs-room_send)
#
# The core -- identifiers, requests and reading their answers -- is always
# there. `events` is every event the specification defines, `state` what a
# client keeps between syncs (it brings events and cs-sync), and each part
# of the client-server API is cs-<part>, one to a file of the specification
# (cs-sync, cs-login, cs-room_send...), each bringing the definitions they
# share. A build that asks for nothing builds all of it, as loom's own does.
# The components are cme features, so a consumer's COMPONENTS reach this file
# as CME_FEATURES_loom; LOOM_COMPONENTS says the same by hand.
include(cmake/generated.cmake)

set(loom_components events state)
foreach(part IN LISTS LOOM_CS_PARTS)
  list(APPEND loom_components cs-${part})
endforeach()
if(COMMAND cme_port_feature)
  cme_port_feature(loom events SUMMARY "every event the specification defines, typed by its type")
  cme_port_feature(loom state SUMMARY "what a client keeps between syncs")
  foreach(part IN LISTS LOOM_CS_PARTS)
    cme_port_feature(loom cs-${part} SUMMARY "the client-server API's ${part}")
  endforeach()
endif()

set(LOOM_COMPONENTS "" CACHE STRING
  "The components of loom to build, as a list; empty for all of them")
set(loom_asked ${LOOM_COMPONENTS})
if(DEFINED CME_FEATURES_loom)
  list(APPEND loom_asked ${CME_FEATURES_loom})
endif()
list(REMOVE_DUPLICATES loom_asked)
foreach(component IN LISTS loom_asked)
  if(NOT component IN_LIST loom_components)
    message(FATAL_ERROR "loom has no component ${component}")
  endif()
endforeach()
if(NOT loom_asked)
  set(loom_asked ${loom_components})
endif()
if("state" IN_LIST loom_asked)
  list(APPEND loom_asked events cs-sync)
endif()
list(REMOVE_DUPLICATES loom_asked)

set(loom_files src/ids.cc src/events.cc src/sync.cc src/api.cc src/loom.cc)
set(loom_parts "")
foreach(component IN LISTS loom_asked)
  if(component MATCHES "^cs-(.+)$")
    list(APPEND loom_parts ${CMAKE_MATCH_1})
  endif()
endforeach()
# The API's definitions name the events, so any part of it brings them.
if(loom_parts OR "events" IN_LIST loom_asked)
  list(APPEND loom_files src/ev/events.cc)
endif()
if(loom_parts)
  list(APPEND loom_files src/cs/definitions.cc)
  foreach(part IN LISTS loom_parts)
    list(APPEND loom_files src/cs/${part}.cc)
  endforeach()
endif()
if("state" IN_LIST loom_asked)
  list(APPEND loom_files src/state.cc)
endif()
list(LENGTH loom_parts loom_part_count)
message(STATUS "loom: ${loom_part_count} parts of the API; ${loom_asked}")

add_library(loom)
target_sources(loom PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES ${loom_files})
target_compile_features(loom PUBLIC cxx_std_26)
target_link_libraries(loom PUBLIC knot)
target_compile_options(loom PUBLIC
  $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-fconstexpr-steps=${LOOM_CONSTEXPR_STEPS}>)

# The names the targets used to have, each the one library now.
add_library(loom::loom ALIAS loom)
foreach(name IN ITEMS ids events sync api ev state)
  add_library(loom::${name} ALIAS loom)
  add_library(loom_${name} ALIAS loom)
endforeach()
add_library(loom_cs_definitions ALIAS loom)
foreach(part IN LISTS loom_parts)
  add_library(loom_cs_${part} ALIAS loom)
endforeach()

if(LOOM_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
