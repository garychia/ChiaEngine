set(INPUT_SOURCE_FILES
    Input/InputHandleLayer.hpp
    Input/InputHandleLayer.cpp
    Input/InputHandler.hpp
    Input/InputHandler.cpp
    Input/KeyboardHandler.hpp
    Input/KeyboardHandler.cpp
    Input/KeyCodes.hpp
    Input/keyCombination.hpp
    Input/keyCombination.cpp
    Input/MouseInfo.hpp
    Input/MouseInfo.cpp
    Input/MouseInput.hpp
    Input/MouseInput.cpp
)
set(IO_SOURCE_FILES
    IO/IO.hpp
    IO/IO.cpp
)
set(OPERATION_SOURCE_FILES
    Operation/Event.hpp
    Operation/Function.hpp
)

set(SYSTEM_SOURCE_FILES
    ${INPUT_SOURCE_FILES}
    ${IO_SOURCE_FILES}
)

if(WIN32)
    list(APPEND INPUT_SOURCE_FILES Input/Windows/WinKeyCode.hpp)
endif()

add_library(ChiaSystem STATIC ${SYSTEM_SOURCE_FILES})
target_include_directories(ChiaSystem PUBLIC
    ${INCLUDE_DIR}
)
