#include "Data/DataModule.hpp"
int main() {
    DataModule dm;
    bool ok = dm.Run();
    return ok ? 0 : 1;
}
