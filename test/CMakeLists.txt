set(IO_TEST_FILE_PATH ${CMAKE_SOURCE_DIR}/test/System/IO/TestFiles/)
configure_file(TestInfo.h.in ${CMAKE_SOURCE_DIR}/test/TestInfo.h @ONLY)

add_subdirectory(System)
set(DATA_TEST_SOURCE_FILES
    Data/StringTest.hpp
    Data/StringTest.cpp
    Data/DataModule.hpp
    Data/DataModule.cpp
)
set(TEST_SOURCE_FILES
    test_main.cpp
    AllModules.hpp
    Test.hpp
    Module.hpp
    ${DATA_TEST_SOURCE_FILES}
)

add_executable(TestMain ${TEST_SOURCE_FILES})
target_include_directories(TestMain PRIVATE
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/src/include
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/include>
)
target_link_libraries(TestMain PRIVATE
    SystemTest
)
