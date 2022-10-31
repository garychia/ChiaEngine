set(DATA_SOURCE_FILES
    Array.hpp
    DynamicArray.hpp
    HashTable.hpp
    List.hpp
    Maybe.hpp
    Pair.hpp
    Pointers.hpp
    Set.hpp
    Str.hpp
    String.hpp
)
add_library(ChiaData INTERFACE ${DATA_SOURCE_FILES})
