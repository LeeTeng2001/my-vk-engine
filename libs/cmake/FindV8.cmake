# FindV8.cmake - Locate V8 JavaScript engine installation

include(FindPackageHandleStandardArgs)

# Fedora-specific paths (modify if needed)
set(_V8_INCLUDE_PATHS
  /usr/include
)

set(_V8_LIB_PATHS
  /usr/lib64
)

# Find headers
find_path(V8_INCLUDE_DIR
  NAMES v8.h
  PATHS ${_V8_INCLUDE_PATHS}
  NO_DEFAULT_PATH
)

find_path(V8_CPPGC_INCLUDE_DIR
  NAMES cppgc/common.h
  PATHS ${_V8_INCLUDE_PATHS}
  NO_DEFAULT_PATH
)

find_path(V8_LIBPLATFORM_INCLUDE_DIR
  NAMES libplatform/libplatform.h
  PATHS ${_V8_INCLUDE_PATHS}
  NO_DEFAULT_PATH
)

# Find libraries
find_library(V8_LIBRARY
  NAMES v8
  PATHS ${_V8_LIB_PATHS}
  NO_DEFAULT_PATH
)

find_library(V8_LIBBASE_LIBRARY
  NAMES v8_libbase
  PATHS ${_V8_LIB_PATHS}
  NO_DEFAULT_PATH
)

find_library(V8_LIBPLATFORM_LIBRARY
  NAMES v8_libplatform
  PATHS ${_V8_LIB_PATHS}
  NO_DEFAULT_PATH
)

# Combine results
set(V8_INCLUDE_DIRS
  ${V8_INCLUDE_DIR}
  ${V8_CPPGC_INCLUDE_DIR}
  ${V8_LIBPLATFORM_INCLUDE_DIR}
)

set(V8_LIBRARIES
  ${V8_LIBRARY}
  ${V8_LIBBASE_LIBRARY}
  ${V8_LIBPLATFORM_LIBRARY}
)

# Handle version
if(EXISTS "${V8_INCLUDE_DIR}/v8-version.h")
  file(STRINGS "${V8_INCLUDE_DIR}/v8-version.h" V8_VERSION_MAJOR REGEX "^#define V8_MAJOR_VERSION ([0-9]+)$")
  file(STRINGS "${V8_INCLUDE_DIR}/v8-version.h" V8_VERSION_MINOR REGEX "^#define V8_MINOR_VERSION ([0-9]+)$")
  string(REGEX REPLACE "^#define V8_MAJOR_VERSION ([0-9]+)$" "\\1" V8_VERSION_MAJOR "${V8_VERSION_MAJOR}")
  string(REGEX REPLACE "^#define V8_MINOR_VERSION ([0-9]+)$" "\\1" V8_VERSION_MINOR "${V8_VERSION_MINOR}")
  set(V8_VERSION "${V8_VERSION_MAJOR}.${V8_VERSION_MINOR}")
endif()

# Create imported target
if(V8_INCLUDE_DIRS AND V8_LIBRARIES AND NOT TARGET V8::V8)
  add_library(V8::V8 SHARED IMPORTED)
  
  set_target_properties(V8::V8 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${V8_INCLUDE_DIRS}"
    IMPORTED_LOCATION "${V8_LIBRARY}"
    INTERFACE_LINK_LIBRARIES "${V8_LIBBASE_LIBRARY};${V8_LIBPLATFORM_LIBRARY};pthread;dl"
  )
endif()

# Handle standard arguments
find_package_handle_standard_args(V8::V8
  REQUIRED_VARS V8_INCLUDE_DIRS V8_LIBRARIES
  VERSION_VAR V8_VERSION
)