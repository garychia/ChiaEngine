#include "Data/DataModule.hpp"
#include "Geometry/GeometryModule.hpp"
int main() {
    DataModule dm;
    GeometryModule gm;
    bool ok = dm.Run();
    ok = gm.Run() && ok;
    return ok ? 0 : 1;
}