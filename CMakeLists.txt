###############################################################################
#  ShadowCopy - CMake Build Configuration
#  
#  This project uses cycfi/elements for UI rendering
###############################################################################
cmake_minimum_required(VERSION 3.15.0)

project(ShadowCopy LANGUAGES C CXX)

# Set C++20 standard (required by Elements)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include FetchContent for downloading dependencies
include(FetchContent)

# Fetch cycfi/elements library
message(STATUS "Fetching cycfi/elements library...")
FetchContent_Declare(
    elements
    GIT_REPOSITORY https://github.com/cycfi/elements.git
    GIT_TAG        master
)

# Configure Elements options
set(ELEMENTS_BUILD_EXAMPLES OFF CACHE BOOL "Don't build Elements examples")
set(ELEMENTS_HOST_UI_LIBRARY "win32" CACHE STRING "Use Win32 backend for Windows")

FetchContent_MakeAvailable(elements)

# Define the executable
add_executable(ShadowCopy WIN32
    ShadowCopy.cpp
    ShadowCopy.h
    framework.h
    targetver.h
    Resource.h
    ShadowCopy.rc
)

# Include directories
target_include_directories(ShadowCopy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link Elements library
target_link_libraries(ShadowCopy PRIVATE
    cycfi::elements
)

# Link Windows libraries
target_link_libraries(ShadowCopy PRIVATE
    comctl32
    dwmapi
    uxtheme
    shlwapi
    advapi32
    shell32
    ole32
    version
    wininet
    crypt32
)

# Set subsystem to Windows
set_target_properties(ShadowCopy PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Copy resources to build directory
add_custom_command(TARGET ShadowCopy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/ShadowCopy.ico"
    "$<TARGET_FILE_DIR:ShadowCopy>/ShadowCopy.ico"
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/small.ico"
    "$<TARGET_FILE_DIR:ShadowCopy>/small.ico"
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/tray_connected.ico"
    "$<TARGET_FILE_DIR:ShadowCopy>/tray_connected.ico"
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/tray_no_internet.ico"
    "$<TARGET_FILE_DIR:ShadowCopy>/tray_no_internet.ico"
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/tray_no_winrar.ico"
    "$<TARGET_FILE_DIR:ShadowCopy>/tray_no_winrar.ico"
)

# Installation rules
install(TARGETS ShadowCopy
    RUNTIME DESTINATION bin
)

install(FILES
    ShadowCopy.ico
    small.ico
    tray_connected.ico
    tray_no_internet.ico
    tray_no_winrar.ico
    DESTINATION bin
)
