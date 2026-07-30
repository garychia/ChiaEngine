#include <iostream>
#include "Data/List.hpp"
int main() {
    List<int> list5;
    list5.Append(10); list5.Append(20); list5.Append(30);
    std::cout << "After appends: " << list5.Length() << std::endl;
    list5.RemoveFirst();
    std::cout << "After RemoveFirst: " << list5.Length() << std::endl;
    auto itr5 = list5.First();
    std::cout << "First element: " << *itr5 << std::endl;
    list5.RemoveLast();
    std::cout << "After RemoveLast: " << list5.Length() << std::endl;
    std::cout << "IsEmpty: " << list5.IsEmpty() << std::endl;
    auto itr6 = list5.First();
    std::cout << "Remaining: " << *itr6 << std::endl;
    return 0;
}
