cmake_minimum_required(VERSION 4.3.4 FATAL_ERROR)

# loom: a Matrix client, as C++26 modules -- sans I/O: what to send made from
# typed requests, and what comes back read straight into types with knot. The
# HTTP is the caller's.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/import-std.cmake)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_EXTENSIONS OFF)

project(loom VERSION 0.1.1 LANGUAGES CXX)

option(LOOM_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})

if(NOT COMMAND cme_find_package)
  set(CME_WITHOUT_PROVIDER ON)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/get_cme.cmake)
endif()

set(LOOM_CONSTEXPR_STEPS 300000000 CACHE STRING
  "clang's -fconstexpr-steps for loom and what imports it")

# splice: the variant loom's choices are, and the visiting. Declared before
# knot so both use the same commit, matching mux's pin.
set(SPLICE_CONSTEXPR_STEPS ${LOOM_CONSTEXPR_STEPS} CACHE STRING
  "clang's -fconstexpr-steps for splice, as loom sets it")
cme_declare_port(NAME splice PROVIDES splice
  GITHUB_REPOSITORY j4niwzis/splice
  GIT_TAG 8a7755a335917daf3bbfc54ef8a3a160f8445d15
  LICENSE GPL-3.0-only
  TARGETS splice)
cme_options(splice "SPLICE_TESTS OFF")
cme_find_package(splice REQUIRED)

# knot, for the JSON: a cme port, pinned to a commit, fetched from GitHub.
# It is private, so git has to be able to authenticate. A checkout is used
# instead with -DCPM_knot_SOURCE=<path>.
cme_declare_port(NAME knot PROVIDES knot
  GITHUB_REPOSITORY j4niwzis/knot
  GIT_TAG 50319a44359801517711438d9882b504922c714b
  LICENSE AGPL-3.0-only
  TARGETS knot)
cme_options(knot "KNOT_TESTS OFF")
cme_find_package(knot REQUIRED)


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
# Written by hand, not generated: what the specification does not have yet
# -- Simplified Sliding Sync (MSC4186).
list(APPEND LOOM_CS_PARTS sliding_sync)

set(loom_components events state crypto)
foreach(part IN LISTS LOOM_CS_PARTS)
  list(APPEND loom_components cs-${part})
endforeach()
if(COMMAND cme_port_feature)
  cme_port_feature(loom events SUMMARY "every event the specification defines, typed by its type")
  cme_port_feature(loom state SUMMARY "what a client keeps between syncs")
  cme_port_feature(loom crypto SUMMARY "end-to-end encryption: Olm and Megolm (vodozemac), key backup, cross-signing, verification")
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

# What each component needs besides itself, as alef's table says it: the
# events for the API's definitions, the definitions for each part of it,
# the events and the sync for the state. `definitions` is not asked for; a
# part brings it.
set(loom_needs_events "")
set(loom_needs_definitions events)
set(loom_needs_state events cs-sync)
set(loom_needs_crypto events cs-keys)
set(loom_file_events src/ev/events.cc)
set(loom_file_definitions src/cs/definitions.cc)
set(loom_file_state src/state.cc)
set(loom_file_crypto src/crypto/wire.cc src/crypto/keys.cc src/crypto/sas.cc src/crypto/crypto.cc)
foreach(part IN LISTS LOOM_CS_PARTS)
  set(loom_needs_cs-${part} definitions)
  set(loom_file_cs-${part} src/cs/${part}.cc)
endforeach()

# What was asked for, and everything it needs.
set(loom_built "")
set(loom_pending ${loom_asked})
while(loom_pending)
  list(POP_FRONT loom_pending component)
  if(component IN_LIST loom_built)
    continue()
  endif()
  list(APPEND loom_built ${component})
  list(APPEND loom_pending ${loom_needs_${component}})
endwhile()

set(loom_files src/ids.cc src/names.cc src/media.cc src/events.cc src/sync.cc src/api.cc src/loom.cc)
set(loom_part_count 0)
foreach(component IN LISTS loom_built)
  list(APPEND loom_files ${loom_file_${component}})
  if(component MATCHES "^cs-")
    math(EXPR loom_part_count "${loom_part_count} + 1")
  endif()
endforeach()
message(STATUS "loom: ${loom_part_count} parts of the API; ${loom_built}")

add_library(loom)
target_sources(loom PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES ${loom_files})
target_compile_features(loom PUBLIC cxx_std_26)
# End-to-end encryption, where asked: OpenSSL for the ciphers and hashes,
# vodozemac for Olm and Megolm.
if("crypto" IN_LIST loom_built)
  find_package(OpenSSL REQUIRED)
  if(NOT TARGET vodozemac::vodozemac)
    cme_find_package(vodozemac REQUIRED)
  endif()
  target_link_libraries(loom PUBLIC OpenSSL::Crypto vodozemac::vodozemac)
endif()
target_link_libraries(loom PUBLIC knot splice)
target_compile_options(loom PUBLIC
  $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-fconstexpr-steps=${LOOM_CONSTEXPR_STEPS}>)

# The names the targets used to have, each the one library now.
add_library(loom::loom ALIAS loom)
foreach(name IN ITEMS ids events sync api ev state)
  add_library(loom::${name} ALIAS loom)
  add_library(loom_${name} ALIAS loom)
endforeach()
add_library(loom_cs_definitions ALIAS loom)
foreach(component IN LISTS loom_built)
  if(component MATCHES "^cs-(.+)$")
    add_library(loom_cs_${CMAKE_MATCH_1} ALIAS loom)
  endif()
endforeach()

if(LOOM_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# ---- What this library is, said by the library; and installing it ----
#
# A consumer writes find_package(loom) and this says the rest: where it
# comes from, what it needs, and what it produces -- installed beside the
# library by cmake-everywhere, under share/cmake-everywhere/ports, so that a
# prefix that has loom describes it too.
if(COMMAND cme_declare_port)
  cme_declare_port(
    NAME loom
    PROVIDES loom
    VERSION ${PROJECT_VERSION}
    GITHUB_REPOSITORY j4niwzis/loom
    GIT_TAG main
    LICENSE AGPL-3.0-only
    DEPENDS knot splice
    TARGETS loom::loom)
endif()
# The library, its module interface units -- a consumer compiles its own
# binary interfaces from them -- and a package: find_package(loom) on a
# machine that has it installed finds it there, by the names it has here
# too, and what it needs again from the ports it came from.
option(LOOM_INSTALL "Install loom and its CMake package" ${PROJECT_IS_TOP_LEVEL})
if(LOOM_INSTALL)
  set(loom_aliases "")
  foreach(name IN ITEMS ids events sync api ev state)
    list(APPEND loom_aliases loom::${name} loom_${name})
  endforeach()
  list(APPEND loom_aliases loom_cs_definitions)
  foreach(component IN LISTS loom_built)
    if(component MATCHES "^cs-(.+)$")
      list(APPEND loom_aliases loom_cs_${CMAKE_MATCH_1})
    endif()
  endforeach()
  cme_install_package(loom ALIASES ${loom_aliases})
endif()
