# CMake tutorial fork

A fork of the [CMake tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html), updated to match the code in this repository. `MathFunctions` is a shared library (`.so` on Linux, `.dll` on Windows). Addition and square root are object libraries linked into that library.

The project is already complete. The steps below explain how it is put together, in the same order as the original tutorial. Every snippet is the code in this repo.

## Requirements

Two programs, both on `PATH`:

| | Version | What it is for |
|---|---|---|
| CMake | 3.24 or newer | configures the project (`cmake`) and runs the tests (`ctest`) |
| C++17 compiler | MSVC, GCC, or Clang | builds `Tutorial`, `AddDemo`, and `MathFunctions` |

Check:

```bash
cmake --version
```

On Windows, if Visual Studio is already installed, add the **Desktop development with C++** workload from the Visual Studio Installer. It includes MSVC and CMake. Then open **Developer PowerShell for VS** (so `cmake` and the compiler are on `PATH`). If CMake was installed separately from [cmake.org](https://cmake.org/download/), a normal terminal is enough.

On Linux:

```bash
# Fedora / RHEL
sudo dnf install cmake gcc-c++

# Debian / Ubuntu
sudo apt install cmake g++
```

The default generator on Windows is Visual Studio. On Linux it is Makefiles, when Ninja is not installed. The presets do not force a generator.

## Build

Shared library, the default:

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Static libraries only:

```bash
cmake --preset static
cmake --build --preset static
ctest --preset static
```

Without presets:

```bash
cmake -S . -B build -DBUILD_SHARED_LIBS=ON
cmake --build build
ctest --test-dir build
```

`Tutorial` computes a square root. `AddDemo` calls `mathfunctions::add` in the shared library (`AddDemo 4` prints `4 + 9 = 13`).

## Step 1 - Executable and version

The starting point is an executable that computes a square root. `cmake_minimum_required` sets the CMake version, `project` sets the name, version, and language, and `add_executable` compiles `tutorial.cxx`.

```cmake
cmake_minimum_required(VERSION 3.24)

project(Tutorial
  VERSION 1.0
  DESCRIPTION "CMake tutorial fork with a working shared library"
  LANGUAGES CXX
)

add_executable(Tutorial tutorial.cxx)
target_compile_features(Tutorial PRIVATE cxx_std_17)
```

`project(... VERSION 1.0)` defines `Tutorial_VERSION_MAJOR` and `Tutorial_VERSION_MINOR`. Those values reach the source through a generated header. `TutorialConfig.h.in` lives in the source tree. CMake writes `TutorialConfig.h` into the build tree, replacing the `@...@` variables:

```c
#define Tutorial_VERSION_MAJOR @Tutorial_VERSION_MAJOR@
#define Tutorial_VERSION_MINOR @Tutorial_VERSION_MINOR@
```

```cmake
configure_file(
  "${PROJECT_SOURCE_DIR}/TutorialConfig.h.in"
  "${PROJECT_BINARY_DIR}/TutorialConfig.h"
)

target_include_directories(Tutorial PRIVATE "${PROJECT_BINARY_DIR}")
```

`tutorial.cxx` includes that header and, when the argument is missing, prints the version together with the usage line:

```cxx
std::cout << argv[0] << " Version " << Tutorial_VERSION_MAJOR << "."
          << Tutorial_VERSION_MINOR << std::endl;
std::cout << "Usage: " << argv[0] << " number" << std::endl;
```

## Step 2 - The MathFunctions library

The square root lives in `MathFunctions/`, built as its own target and linked by anything that needs it.

```cmake
add_library(MathFunctions MathFunctions.cxx)
add_library(MathFunctions::MathFunctions ALIAS MathFunctions)

target_compile_features(MathFunctions PUBLIC cxx_std_17)
target_link_libraries(Tutorial PRIVATE MathFunctions::MathFunctions)
```

The `MathFunctions::MathFunctions` alias is the same name another project gets after `find_package`. In the source, the API is the `mathfunctions` namespace:

```cxx
namespace mathfunctions {
MATHFUNCTIONS_EXPORT double sqrt(double x);
MATHFUNCTIONS_EXPORT int add(int a, int b);
}
```

`target_link_libraries(... PRIVATE ...)` applies to `Tutorial` and to `AddDemo`. `PUBLIC` on the C++ feature makes every consumer inherit the C++17 requirement. The library include paths use generator expressions, so the right path is selected for the build tree and for the install tree:

```cmake
target_include_directories(MathFunctions
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
```

### Making our implementation optional

`USE_MYMATH` chooses whether `mathfunctions::sqrt` calls `detail::mysqrt` or `std::sqrt`. The default is on. The option is stored in the CMake cache and can be changed from the command line or the GUI.

```cmake
option(USE_MYMATH "Use the tutorial sqrt implementation" ON)
```

The library stays in the build either way: `AddDemo` always needs `mathfunctions::add`. Only the body of `sqrt` changes, through a compile definition:

```cmake
target_compile_definitions(MathFunctions PRIVATE
  "$<$<BOOL:${USE_MYMATH}>:USE_MYMATH>"
)
```

```cxx
double sqrt(double x)
{
#ifdef USE_MYMATH
  return detail::mysqrt(x);
#else
  return std::sqrt(x);
#endif
}
```

To turn it off: `cmake -S . -B build -DUSE_MYMATH=OFF`.

## Step 3 - Tests and installation

`include(CTest)` defines the `BUILD_TESTING` option (on by default) and enables `ctest`. The tests live in the top-level `CMakeLists.txt`.

`Runs` checks that `Tutorial 25` exits with code zero. `Usage` runs `Tutorial` with no arguments and accepts the failure when the output matches the usage line. `do_test` repeats the same pattern for known values:

```cmake
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
```

`AddDemoRuns` checks the addition in the shared library. In a regular expression `+` is escaped, because it is a quantifier.

Installation uses the `GNUInstallDirs` paths (`bin`, `lib`, `include`), so the layout is the same on every platform:

```cmake
install(TARGETS Tutorial AddDemo
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
```

```bash
cmake --install build --prefix /tmp/tutorial
```

## Step 4 - log, exp, and the generated table

`mysqrt` prefers `exp(log(x) * 0.5)` when the system provides both functions. `check_symbol_exists` checks that at configure time. On Unix the math functions live in `libm`, so that library goes into `CMAKE_REQUIRED_LIBRARIES` before the check.

```cmake
include(CheckSymbolExists)

if(NOT WIN32)
  set(CMAKE_REQUIRED_LIBRARIES m)
endif()
check_symbol_exists(log "math.h" HAVE_LOG)
check_symbol_exists(exp "math.h" HAVE_EXP)
```

The result becomes a private compile definition on `SqrtLibrary`, enabled only when the check succeeds:

```cmake
target_compile_definitions(SqrtLibrary PRIVATE
  "$<$<BOOL:${HAVE_LOG}>:HAVE_LOG>"
  "$<$<BOOL:${HAVE_EXP}>:HAVE_EXP>"
)
```

When `log` or `exp` is missing, `mysqrt` starts from a value read out of a table generated during the build. `MakeTable` is an executable that writes `Table.h`:

```cmake
add_executable(MakeTable MakeTable.cxx)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Table.h
  COMMAND MakeTable ${CMAKE_CURRENT_BINARY_DIR}/Table.h
  DEPENDS MakeTable
)
```

Listing `Table.h` among the sources of `SqrtLibrary` tells CMake to produce that file before compiling `mysqrt.cxx`. The build directory is an include path because `Table.h` is not in the source tree:

```cmake
add_library(SqrtLibrary OBJECT
  mysqrt.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/Table.h
)
target_include_directories(SqrtLibrary PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
```

This whole block, table included, exists only while `USE_MYMATH` is on.

## Step 5 - Shared library

`BUILD_SHARED_LIBS=ON` (the default, and the `default` preset) makes `add_library(MathFunctions ...)` with no explicit type produce a shared library: `libMathFunctions.so` or `MathFunctions.dll`. The `static` preset turns it off.

```cmake
option(BUILD_SHARED_LIBS "Build MathFunctions as a shared library (.so / .dll)" ON)

set_target_properties(MathFunctions PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)
```

`VERSION` is the full filename (`.so.1.0.0`). `SOVERSION` is the ABI compatibility suffix (`.so.1`), equal to the project's major version. On GCC and Clang, `CXX_VISIBILITY_PRESET hidden` hides every symbol that is not marked for export.

`GenerateExportHeader` writes `mathfunctions_export.h` with the `MATHFUNCTIONS_EXPORT` macro. On a Windows DLL that macro is `__declspec(dllexport)` inside the library and `__declspec(dllimport)` for its consumers. On ELF, with hidden visibility, it is the default-visibility attribute. The only exported symbols are the ones in `MathFunctions.h`.

```cmake
include(GenerateExportHeader)

generate_export_header(MathFunctions
  BASE_NAME mathfunctions
  EXPORT_FILE_NAME mathfunctions_export.h
)
```

The public headers, including the generated one, are a file set. `install(... FILE_SET HEADERS ...)` copies them into `include/` together with the library:

```cmake
target_sources(MathFunctions PUBLIC
  FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  FILES
    MathFunctions.h
    ${CMAKE_CURRENT_BINARY_DIR}/mathfunctions_export.h
)
```

### Object libraries

`add` and `mysqrt` are object libraries. CMake compiles the `.cxx` files and passes the `.o` / `.obj` files into `MathFunctions`.

```cmake
add_library(AddLibrary OBJECT add.cxx)
set_target_properties(AddLibrary PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(MathFunctions PRIVATE AddLibrary)
```

`POSITION_INDEPENDENT_CODE` is set because those objects end up inside a `.so` or a `.dll`. `SqrtLibrary` has the same property. An `OBJECT` library is absorbed when `MathFunctions` is shared and when it is static.

At build time the executable and the library land in the same directory (`CMAKE_RUNTIME_OUTPUT_DIRECTORY` and `CMAKE_LIBRARY_OUTPUT_DIRECTORY` both point at `PROJECT_BINARY_DIR`). The loader finds the DLL or the `.so` with no `PATH` changes.

At install time the executable goes to `bin/` and the library goes to `lib/`. On Windows the DLL goes to `bin/`, next to the executable, through `RUNTIME DESTINATION`. On Linux and macOS the installed executable's `RPATH` points at the library directory:

```cmake
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@executable_path/../${CMAKE_INSTALL_LIBDIR}")
elseif(UNIX)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()
```

## Step 6 - A CMake package for other projects

`install(EXPORT)` writes `MathFunctionsTargets.cmake`: the description of the installed targets, under the `MathFunctions::` namespace. `Config.cmake.in` includes it. `write_basic_package_version_file` adds `MathFunctionsConfigVersion.cmake`, with `SameMajorVersion` compatibility.

```cmake
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
```

The files land in `lib/cmake/MathFunctions`. Another project uses them like this:

```cmake
find_package(MathFunctions 1.0 REQUIRED)
target_link_libraries(Consumer PRIVATE MathFunctions::MathFunctions)
```

`export(EXPORT ...)` writes the same targets file into the build tree, for a consumer that points at the build tree without installing.

## Step 7 - CPack

CPack builds an installer from the `install` rules already written. `InstallRequiredSystemLibraries` adds the compiler runtimes the current platform needs. Version and license come from the project variables:

```cmake
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${Tutorial_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${Tutorial_VERSION_MINOR}")
include(CPack)
```

After a build, from the build directory:

```bash
cpack --config CPackConfig.cmake
cpack --config CPackSourceConfig.cmake
```

The first creates the binary package. The second creates the source package.

## Presets

`CMakePresets.json` fixes the two configurations used above, without choosing a generator. `default` turns `BUILD_SHARED_LIBS` on and writes to `build/`. `static` turns it off and writes to `build-static/`, so the two builds keep separate caches.

```json
{
  "name": "default",
  "binaryDir": "${sourceDir}/build",
  "cacheVariables": {
    "BUILD_SHARED_LIBS": "ON",
    "USE_MYMATH": "ON"
  }
}
```

`cmake --preset`, `cmake --build --preset`, and `ctest --preset` all use the same name.
