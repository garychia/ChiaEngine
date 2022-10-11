add_subdirectory(IO)
set(SYSTEM_TEST_SOURCE_FILES
    SystemModule.hpp
    SystemModule.cpp
)
add_library(SystemTest STATIC ${SYSTEM_TEST_SOURCE_FILES})
target_include_directories(SystemTest PUBLIC
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/src
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
)
target_link_libraries(SystemTest PUBLIC IOTest)
