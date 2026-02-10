cmake_minimum_required(VERSION 3.15)
project(openjph_jni LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- Find JNI ---
find_package(JNI REQUIRED)

# --- Fetch OpenJPH ---
include(FetchContent)
FetchContent_Declare(
    openjph
    GIT_REPOSITORY https://github.com/aous72/OpenJPH.git
    GIT_TAG        0.26.0
    GIT_SHALLOW    TRUE
)
# Build OpenJPH as a static library
set(OJPH_BUILD_EXECUTABLES OFF CACHE BOOL "" FORCE)
set(OJPH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(OJPH_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(openjph)

# --- JNI shared library ---
add_library(openjph_jni SHARED
    src/openjph_wrapper.cpp
    src/org_dcm4che3_openjph_OpenJPH.cpp
)

target_include_directories(openjph_jni PRIVATE
    ${JNI_INCLUDE_DIRS}
    ${openjph_SOURCE_DIR}/src/core/common
    src
)

# Link OpenJPH static library
target_link_libraries(openjph_jni PRIVATE openjph)

# Strip the lib prefix on Windows
if(WIN32)
    set_target_properties(openjph_jni PROPERTIES PREFIX "")
endif()

# Install
install(TARGETS openjph_jni
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION lib
)
