set(IO_TEST_SOURCE_FILES
    IOTest.hpp
    IOTest.cpp
)
add_library(IOTest STATIC ${IO_TEST_SOURCE_FILES})
target_include_directories(IOTest PUBLIC
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/src
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
)
target_link_libraries(IOTest PUBLIC ChiaSystem)
