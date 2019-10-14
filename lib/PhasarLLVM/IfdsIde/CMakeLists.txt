file(GLOB_RECURSE IFDSIDE_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
  phasar_pointer
  phasar_controlflow
  phasar_phasarllvm_utils
  phasar_db
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_ifdside
    SHARED
    ${IFDSIDE_SRC}
  )
else()
  add_phasar_library(phasar_ifdside
    STATIC
    ${IFDSIDE_SRC}
  )
endif()

include(GenerateExportHeader)
generate_export_header(phasar_ifdside EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/phasar_ifdside_export.h)

target_include_directories(phasar_ifdside
  PUBLIC
  ${CMAKE_BINARY_DIR}/exports/
)

set_target_properties(phasar_ifdside
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
