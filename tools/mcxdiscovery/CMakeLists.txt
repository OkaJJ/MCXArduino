cmake_minimum_required(VERSION 3.10)
project(mcxdiscovery)

# We need to compile to the right platform for support for windows, linux and macos.
if(WIN32)
    set(HID_PLATFORM_SRC "hidapi/windows/hid.c")
    set(PLATFORM_LIBS setupapi hid)
    set(OUTPUT_SUBDIR "windows")
elseif(APPLE)
    set(HID_PLATFORM_SRC "hidapi/mac/hid.c")
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    set(PLATFORM_LIBS ${IOKIT_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
    set(OUTPUT_SUBDIR "macos")
else()
    set(HID_PLATFORM_SRC "hidapi/linux/hid.c")
    set(PLATFORM_LIBS udev)
    set(OUTPUT_SUBDIR "x86_64-linux-gnu")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${OUTPUT_SUBDIR})

add_executable(mcx-discovery 
    mcx-discovery.cpp 
    ${HID_PLATFORM_SRC}
)

# HIDapi linking
target_include_directories(mcx-discovery PRIVATE hidapi/hidapi)

target_link_libraries(mcx-discovery PRIVATE ${PLATFORM_LIBS})