#include "Data/DataModule.hpp"
#include "Geometry/GeometryModule.hpp"
#include "System/SystemModuleStandalone.hpp"
int main() {
    DataModule dm;
    GeometryModule gm;
    SystemModuleStandalone sm;
    bool ok = dm.Run();
    ok = gm.Run() && ok;
    ok = sm.Run() && ok;
    return ok ? 0 : 1;
}