include(CheckSymbolExists)
include(GenerateExportHeader)

add_library(MathFunctions MathFunctions.cxx)
add_library(MathFunctions::MathFunctions ALIAS MathFunctions)

generate_export_header(MathFunctions
  BASE_NAME mathfunctions
  EXPORT_FILE_NAME mathfunctions_export.h
)

target_sources(MathFunctions PUBLIC
  FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  FILES
    MathFunctions.h
    ${CMAKE_CURRENT_BINARY_DIR}/mathfunctions_export.h
)

target_compile_features(MathFunctions PUBLIC cxx_std_17)
target_include_directories(MathFunctions
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(MathFunctions PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)

# Object code linked into MathFunctions. POSITION_INDEPENDENT_CODE lets the
# same objects go inside a .so or a .dll. A plain STATIC archive would not
# be absorbed when MathFunctions itself is static.
add_library(AddLibrary OBJECT add.cxx)
target_compile_features(AddLibrary PRIVATE cxx_std_17)
set_target_properties(AddLibrary PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(MathFunctions PRIVATE AddLibrary)

if(USE_MYMATH)
  if(NOT WIN32)
    set(CMAKE_REQUIRED_LIBRARIES m)
  endif()
  check_symbol_exists(log "math.h" HAVE_LOG)
  check_symbol_exists(exp "math.h" HAVE_EXP)

  add_executable(MakeTable MakeTable.cxx)
  target_compile_features(MakeTable PRIVATE cxx_std_17)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Table.h
    COMMAND MakeTable ${CMAKE_CURRENT_BINARY_DIR}/Table.h
    DEPENDS MakeTable
  )

  add_library(SqrtLibrary OBJECT
    mysqrt.cxx
    ${CMAKE_CURRENT_BINARY_DIR}/Table.h
  )
  target_compile_features(SqrtLibrary PRIVATE cxx_std_17)
  target_include_directories(SqrtLibrary PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  set_target_properties(SqrtLibrary PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(SqrtLibrary PRIVATE
    "$<$<BOOL:${HAVE_LOG}>:HAVE_LOG>"
    "$<$<BOOL:${HAVE_EXP}>:HAVE_EXP>"
  )
  target_link_libraries(MathFunctions PRIVATE SqrtLibrary)
endif()

target_compile_definitions(MathFunctions PRIVATE
  "$<$<BOOL:${USE_MYMATH}>:USE_MYMATH>"
)

install(TARGETS MathFunctions
  EXPORT MathFunctionsTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
