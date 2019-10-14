file(GLOB_RECURSE CONFIG_SRC *.h *.cpp)

set(LLVM_LINK_COMPONENTS
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_config
    SHARED
    ${CONFIG_SRC}
  )
else()
  add_phasar_library(phasar_config
    STATIC
    ${CONFIG_SRC}
  )
endif()

include(GenerateExportHeader)
generate_export_header(phasar_config EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/phasar_config_export.h)

target_include_directories(phasar_config
  PUBLIC
  ${CMAKE_BINARY_DIR}/exports
)

target_link_libraries(phasar_config
  LINK_PUBLIC
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_config
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
