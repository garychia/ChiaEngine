# ChiaEngine

A cross-platform 3D game engine built from scratch in modern C++ (C++17).

## Overview

ChiaEngine is a modular game engine supporting **DirectX**, **Vulkan**, and **OpenGL** rendering backends. It features custom containers, a math library, geometry primitives, input handling, and a GUI system — all designed with minimal external dependencies.

| Backend | Windows | Linux | macOS |
|---------|---------|-------|-------|
| DirectX | ✅ | — | — |
| Vulkan | ✅ | ✅ | ✅ |
| OpenGL | ✅ | ✅ | ✅ |

> **Note:** Currently only the DirectX backend has a working OpenGL implementation; Vulkan and OpenGL backends are stubs awaiting implementation.

## Requirements

| Tool | Version |
|------|---------|
| CMake | ≥ 3.22 |
| C++ Compiler | GCC ≥ 10, Clang ≥ 12, MSVC 2022 |
| GLFW | 3.3.8 (auto-fetched) |
| GLM | 0.9.9.8 (auto-fetched) |

### Linux Dependencies

```bash
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### OpenGL Backend Setup

When building with the OpenGL backend (default on non-Windows without Vulkan SDK), you need **GLAD** (OpenGL function loader). Generate the files at [glad.dav1d.de](https://glad.dav1d.de/) with:

| Setting | Value |
|---------|-------|
| API | `gl` Version `3.3` |
| Profile | `Core` |
| Options | ✅ Generate a loader |

Place the generated `glad/glad.h` and `glad.c` in `src/include/3rdparty/glad/`.

## Building

```bash
git clone https://github.com/garychia/ChiaEngine.git
cd ChiaEngine
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j$(nproc)
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `DIRECTX_ENABLED` | ON (Windows) | Use DirectX rendering backend |
| `VULKAN_ENABLED` | ON (non-Windows) | Use Vulkan rendering backend |
| `OPENGL_ENABLED` | OFF | Use OpenGL rendering backend |

## Project Structure

```
ChiaEngine/
├── CMakeLists.txt           # Root build configuration
├── src/
│   ├── include/             # Public headers
│   │   ├── App/             # Application loop & setup
│   │   ├── Data/            # Custom containers
│   │   │   ├── Array.hpp        # Static array
│   │   │   ├── DynamicArray.hpp # Dynamic array
│   │   │   ├── List.hpp         # Doubly-linked list
│   │   │   ├── Set.hpp          # Hash set
│   │   │   ├── HashTable.hpp    # Hash table (key-value)
│   │   │   ├── Str.hpp          # Generic string
│   │   │   ├── String.hpp       # UTF-16 string
│   │   │   ├── Pointers.hpp     # Ptr, SharedPtr, WeakPtr
│   │   │   ├── Maybe.hpp        # Optional value
│   │   │   └── Pair.hpp         # Key-value pair
│   │   ├── Display/         # Rendering & windowing
│   │   │   ├── DirectX/         # DirectX 11 backend
│   │   │   ├── Vulkan/          # Vulkan backend (stub)
│   │   │   ├── GLFW/            # GLFW window support
│   │   │   ├── GUI/             # GUI system
│   │   │   └── Images/          # Image assets
│   │   ├── Geometry/        # 2D/3D primitives
│   │   ├── Math/            # Math library
│   │   ├── System/          # I/O, input, debug
│   │   └── Types/           # Type utilities
│   ├── source/              # Implementation files
│   └── ChiaApp/             # Editor application
├── test/
│   ├── Data/                # Container tests
│   └── System/              # System tests
└── build/                   # Build output
```

## Tests

ChiaEngine includes unit tests for core data structures. Build and run:

```bash
cmake --build build -j$(nproc)
./build/bin/ChiaEngineTests
```

### Test Coverage

| Component | Status |
|-----------|--------|
| `Array` | ✅ Full |
| `DynamicArray` | ✅ Full |
| `List` | ✅ Full |
| `Set` | ✅ Full |
| `HashTable` | ✅ Full |
| `String` | ✅ Full |
| `FileIO` | ✅ Full |
| `Pointers` (Ptr, SharedPtr, WeakPtr) | ✅ Full |
| `Maybe` / `Pair` | ✅ Full |

> Tests require proper X11/Vulkan/etc. setup on Linux. On a headless system, syntax-only verification can be done with:
> ```bash
> g++ -std=c++17 -fsyntax-only -I src/include -I test -c test/Data/DataModule.cpp
> ```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing`)
5. Open a Pull Request

## License

Distributed under the MIT License. See `LICENSE` for more information.