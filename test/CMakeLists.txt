# The tests: googletest's, as a module.
#
# googletest is j4niwzis/googletest-modules, the fork that builds it as one:
# a test here imports std, and a header that includes the standard library
# textually would give a translation unit two views of it. It comes from
# cmake-everywhere, pinned to a commit, and never from the machine -- a
# googletest installed anywhere is the one that is a header.
#
# cmake-everywhere is asked without being installed as the dependency
# provider: which provider a build has is for whoever builds loom, and only
# the tests need anything fetched.
if(NOT COMMAND cme_find_package)
  set(CME_WITHOUT_PROVIDER ON)
  include(${PROJECT_SOURCE_DIR}/cmake/get_cme.cmake)
endif()
cme_declare_port(NAME googletest-modules PROVIDES GTest
  GITHUB_REPOSITORY j4niwzis/googletest-modules
  GIT_TAG 706920f1d230f0316d271466d8885735be3e300e
  LICENSE BSD-3-Clause
  SYSTEM NEVER
  TARGETS gtest_main)
cme_options(googletest-modules
  "BUILD_GMOCK OFF" "INSTALL_GTEST OFF" "gtest_build_modules ON"
  "gtest_build_tests OFF" "gtest_build_samples OFF")
cme_find_package(GTest REQUIRED)
# A copy from a prefix names it GTest::gtest_main, and one built here
# gtest_main.
if(TARGET GTest::gtest_main)
  set(loom_gtest GTest::gtest_main)
else()
  set(loom_gtest gtest_main)
endif()

# Every CONSTEXPR_TEST run by the compiler as well, as a constant expression,
# as alef's are. CI turns it on; off by default, because it is the slow half.
option(LOOM_CONSTEXPR_TESTS "Run every CONSTEXPR_TEST while it is compiled as well" OFF)
get_property(loom_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
foreach(one IN ITEMS loom)
  add_executable(loom-${one}-test ${one}_test.cc)
  target_link_libraries(loom-${one}-test PRIVATE loom ${loom_gtest})
  add_test(NAME ${one} COMMAND loom-${one}-test)
  if(NOT LOOM_CONSTEXPR_TESTS)
    continue()
  endif()
  # A target for each test, compiling it alone with its body as a
  # static_assert: what a test that is not a constant expression builds, to
  # show what the compiler says of it.
  target_compile_definitions(loom-${one}-test PRIVATE
    LOOM_CONSTEXPR_TESTS
    "LOOM_TEST_CMAKE=\"${CMAKE_COMMAND}\""
    "LOOM_TEST_BUILD_DIR=\"${CMAKE_BINARY_DIR}\""
    $<${loom_multi_config}:LOOM_TEST_CONFIG="$<CONFIG>">)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${one}_test.cc)
  file(STRINGS ${one}_test.cc declared REGEX "^CONSTEXPR_TEST\\(")
  foreach(line IN LISTS declared)
    if(line MATCHES "^CONSTEXPR_TEST\\(([A-Za-z0-9_]+), *([A-Za-z0-9_]+)\\)")
      set(test ${CMAKE_MATCH_1}.${CMAKE_MATCH_2})
      add_library(loom-constexpr-${test} OBJECT EXCLUDE_FROM_ALL ${one}_test.cc)
      target_link_libraries(loom-constexpr-${test} PRIVATE loom ${loom_gtest})
      target_compile_definitions(loom-constexpr-${test} PRIVATE
        LOOM_CONSTEXPR_TESTS "LOOM_CONSTEXPR_DIAGNOSE=\"${test}\"")
    endif()
  endforeach()
endforeach()
