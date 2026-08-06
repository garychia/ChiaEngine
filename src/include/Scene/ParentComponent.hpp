#ifndef PARENT_COMPONENT_HPP
#define PARENT_COMPONENT_HPP

#include "System/World/Entity.hpp"

// 階層連線:只存 parent 一個欄位(設計 D1-A)。
// - parent.raw == 0(root sentinel,沿用 Entity::FromRaw(0) 慣例)= 根節點。
// - children 一律由掃描本 pool 推導,不另存 adjacency → reparent / destroy 後
//   child 清單恆正確(AC2),任何時刻都不會與 World 狀態漂移。
// sizeof == 4,無 padding,trivially copyable → raw-byte hash 穩定。
struct ParentComponent
{
    Entity parent;
};

#endif // PARENT_COMPONENT_HPP