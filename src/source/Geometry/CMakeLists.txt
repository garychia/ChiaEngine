set(GEOMETRY_SOURCE_FILES
    2D/Point2D.hpp
    2D/Point2D.cpp
    3D/Point3D.hpp
    3D/Point3D.cpp
    Primitives.hpp
    Primitives.cpp
)
add_library(ChiaGeometry STATIC ${GEOMETRY_SOURCE_FILES})
target_include_directories(ChiaGeometry PUBLIC ${INCLUDE_DIR})
