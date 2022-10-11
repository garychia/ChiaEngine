set(DATA_SOURCE_FILES
    Array.hpp
    DynamicArray.hpp
    HashTable.hpp
    List.hpp
    Pointers.hpp
    Str.hpp
    String.hpp
)
add_library(ChiaData INTERFACE ${DATA_SOURCE_FILES})
