cmake_minimum_required(VERSION 3.22)
project(ChiaGameEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# ---- 確定性 FP 政策(#143) ----
# gcc 與 clang 對 C++ 預設 -ffp-contract=fast:允許把 a*b+c 收成一條 FMA。實測
# `g++ -std=c++17 -O2 -mfma -S` 在沒給 -ffp-contract 時就發出 vfmadd132ss,給了
# -ffp-contract=off 才變 vmulss —— 兩條 compiler leg 因此可以合法地得到不同捨入,
# World::Hash() 就會分歧,而宣言宣稱「any compiler 都同 hash」。
# sim 的位元級重現需要固定捨入;View 層沒有理由不同意,所以整包關掉收縮。
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-ffp-contract=off)
endif()

# 反向守門:誰把這些旗標塞進來,configure 就當場失敗 —— 它們會讓浮點捨入隨主機
# (FMA/AVX 路徑)或最佳化等級改變,直接破壞 World::Hash() 的可重現性。
# 放在檔案前段,所以失敗訊息來得比 X11/Vulkan 的相依檢查更早、更清楚。
set(CHIA_ALL_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_RELEASE} "
    "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS_MINSIZEREL}")
if(CHIA_ALL_CXX_FLAGS MATCHES "fast-math|Ofast|march=native")
    message(FATAL_ERROR
        "ChiaEngine determinism (#143): -ffast-math / -Ofast / -march=native are not allowed. "
        "They change float rounding per host or allow reassociation, so World::Hash() stops being "
        "reproducible across compilers and machines. Drop the flag, or narrow the determinism claim.")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/archives)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)

set(IMAGE_FILE_PATH ${CMAKE_SOURCE_DIR}/src/include/Display/Images/)
configure_file(src/source/Paths.hpp.in ${CMAKE_SOURCE_DIR}/src/include/Paths.hpp @ONLY)

include(CMakeDependentOption)
set(CHIA_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/include)

cmake_dependent_option(
    DIRECTX_ENABLED
    "Use DirectX as the rendering engine. (DEPRECATED — not wired into the Frame/IFrameExecutor architecture, build-broken; see docs/agents/directx-backend-assessment.md)"
    ON "WIN32" OFF)
cmake_dependent_option(
    VULKAN_ENABLED
    "Use Vulkan as the rendering engine."
    ON "NOT DIRECTX_ENABLED" OFF
)
cmake_dependent_option(
    OPENGL_ENABLED
    "Use OpenGL as the rendering engine. (DEPRECATED — Vulkan is the supported backend; see docs/agents/opengl-backend-assessment.md)"
    ON "NOT DIRECTX_ENABLED;NOT VULKAN_ENABLED" OFF
)

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
# glm 0.9.9.8 declares cmake_minimum_required(VERSION 3.2), and CMake >= 4.0 rejects
# anything below 3.5 outright ("Compatibility with CMake < 3.5 has been removed"), which
# made every fresh configure fail. This tells CMake to configure it anyway; the variable
# is ignored by CMake older than 3.31, so it cannot regress older toolchains.
# See #118. glm is fetched only for its headers (see the include_directories below).
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(glm)
# GLM is header-only; old CMakeLists doesn't set up include dirs, so add globally
include_directories(${glm_SOURCE_DIR})

if (NOT DIRECTX_ENABLED)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
    FetchContent_MakeAvailable(glfw)
endif()

if(DIRECTX_ENABLED)
    add_compile_definitions(DIRECTX_ENABLED)
elseif(VULKAN_ENABLED)
    find_package(Vulkan REQUIRED)
    add_compile_definitions(VULKAN_ENABLED)
else()
    message(WARNING "OPENGL_ENABLED is DEPRECATED — the OpenGL backend is not wired into the Frame/IFrameExecutor architecture (Renderer.hpp has no OpenGL branch) and needs a manually generated GLAD loader (src/include/3rdparty/glad/). Vulkan is the supported backend. See docs/agents/opengl-backend-assessment.md.")
    find_package(OpenGL REQUIRED)
    add_compile_definitions(OPENGL_ENABLED)
    list(APPEND CHIA_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/include/3rdparty)
endif()

add_subdirectory(src)
add_subdirectory(src/ChiaApp)

add_subdirectory(test)
