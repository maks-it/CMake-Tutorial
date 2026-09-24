cmake_minimum_required(VERSION 3.24)

project(Tutorial
  VERSION 1.0
  DESCRIPTION "CMake tutorial fork with a working shared library"
  LANGUAGES CXX
)

include(GNUInstallDirs)
include(CTest)
include(CMakePackageConfigHelpers)

option(BUILD_SHARED_LIBS "Build MathFunctions as a shared library (.so / .dll)" ON)
option(USE_MYMATH "Use the tutorial sqrt implementation" ON)

# Put the executable and the shared library in the same directory so the
# loader finds the library without PATH changes (Windows DLL and Linux .so).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

if(APPLE)
  set(CMAKE_INSTALL_RPATH "@executable_path/../${CMAKE_INSTALL_LIBDIR}")
elseif(UNIX)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

configure_file(
  "${PROJECT_SOURCE_DIR}/TutorialConfig.h.in"
  "${PROJECT_BINARY_DIR}/TutorialConfig.h"
)

add_subdirectory(MathFunctions)

add_executable(Tutorial tutorial.cxx)
target_compile_features(Tutorial PRIVATE cxx_std_17)
target_link_libraries(Tutorial PRIVATE MathFunctions::MathFunctions)
target_include_directories(Tutorial PRIVATE "${PROJECT_BINARY_DIR}")

add_executable(AddDemo add_demo.cxx)
target_compile_features(AddDemo PRIVATE cxx_std_17)
target_link_libraries(AddDemo PRIVATE MathFunctions::MathFunctions)

install(TARGETS Tutorial AddDemo
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
  add_test(NAME Runs COMMAND Tutorial 25)

  add_test(NAME Usage COMMAND Tutorial)
  set_tests_properties(Usage
    PROPERTIES PASS_REGULAR_EXPRESSION "Usage:.*number"
  )

  function(do_test target arg result)
    add_test(NAME Comp${arg} COMMAND ${target} ${arg})
    set_tests_properties(Comp${arg}
      PROPERTIES PASS_REGULAR_EXPRESSION "${result}"
    )
  endfunction()

  do_test(Tutorial 4 "4 is 2")
  do_test(Tutorial 9 "9 is 3")
  do_test(Tutorial 5 "5 is 2.236")
  do_test(Tutorial 7 "7 is 2.645")
  do_test(Tutorial 25 "25 is 5")
  do_test(Tutorial -25 "-25 is 0")
  do_test(Tutorial 0.0001 "0.0001 is 0.01")

  add_test(NAME AddDemoRuns COMMAND AddDemo 4)
  set_tests_properties(AddDemoRuns
    PROPERTIES PASS_REGULAR_EXPRESSION "4 \\+ 9 = 13"
  )
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${Tutorial_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${Tutorial_VERSION_MINOR}")
include(CPack)

install(EXPORT MathFunctionsTargets
  FILE MathFunctionsTargets.cmake
  NAMESPACE MathFunctions::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MathFunctions
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/MathFunctionsConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MathFunctions
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/MathFunctionsConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/MathFunctionsConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/MathFunctionsConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MathFunctions
)

export(EXPORT MathFunctionsTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/MathFunctionsTargets.cmake"
  NAMESPACE MathFunctions::
)
