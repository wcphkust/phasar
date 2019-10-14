file(GLOB_RECURSE PASSES_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Analysis
)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_passes
		SHARED
		${PASSES_SRC}
	)
else()
	add_phasar_library(phasar_passes
		STATIC
		${PASSES_SRC}
	)
endif()

include(GenerateExportHeader)
generate_export_header(phasar_passes EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/phasar_passes_export.h)

target_include_directories(phasar_passes
  PUBLIC
  ${CMAKE_BINARY_DIR}/exports/
)

target_link_libraries(phasar_passes
  LINK_PUBLIC
  ${Boost_LOG_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
)

set_target_properties(phasar_passes
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
