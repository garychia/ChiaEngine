#ifndef SYSTEM_OPERATION_TEST_HPP
#define SYSTEM_OPERATION_TEST_HPP

#include "Test.hpp"
#include "System/Operation/Function.hpp"
#include "System/Operation/Event.hpp"

static int g_functionResult = 0;
static void FreeFunctionVoid() { g_functionResult = 42; }
static int FreeFunctionInt(int x) { return x * 2; }

class SystemOperationTest : public Test
{
  private:
    int memberValue = 0;

  public:
    SystemOperationTest() : Test("SystemOperation")
    {
    }

    void SetMemberValue(int v) { memberValue = v; }
    int GetMemberValue() const { return memberValue; }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Function default ctor");
        Function<void()> emptyFunc;
        EXPECT_TRUE(!emptyFunc.Valid(), "Default function is invalid.", true);

        TEST_MESSAGE("Function with free function");
        Function<void()> funcVoid(FreeFunctionVoid);
        EXPECT_TRUE(funcVoid.Valid(), "Function should be valid.", true);
        g_functionResult = 0;
        funcVoid();
        EXPECT_TRUE(g_functionResult == 42, "Calling function sets result.", true);

        TEST_MESSAGE("Function with params and return");
        Function<int(int)> funcInt(FreeFunctionInt);
        EXPECT_TRUE(funcInt(5) == 10, "f(5) = 10.", true);
        EXPECT_TRUE(funcInt(-3) == -6, "f(-3) = -6.", true);

        TEST_MESSAGE("MemberFunction default ctor");
        MemberFunction<SystemOperationTest, void(int)> memEmpty;
        EXPECT_TRUE(!memEmpty.Valid(), "Default is invalid.", true);

        TEST_MESSAGE("MemberFunction call");
        MemberFunction<SystemOperationTest, void(int)> memSet(this, &SystemOperationTest::SetMemberValue);
        EXPECT_TRUE(memSet.Valid(), "Valid member function.", true);
        memberValue = 0;
        memSet(99);
        EXPECT_TRUE(memberValue == 99, "Member function set value.", true);

        TEST_MESSAGE("Callback with member function");
        Callback<void(int)> cbMember(this, &SystemOperationTest::SetMemberValue);
        EXPECT_TRUE(cbMember.Valid(), "Member callback valid.", true);
        memberValue = 0;
        cbMember(77);
        EXPECT_TRUE(memberValue == 77, "Callback calls member function.", true);

        TEST_MESSAGE("Callback::Set with free function");
        Callback<void()> cbFree;
        cbFree.Set(FreeFunctionVoid);
        EXPECT_TRUE(cbFree.Valid(), "Callback valid after Set.", true);
        g_functionResult = 0;
        cbFree();
        EXPECT_TRUE(g_functionResult == 42, "Callback calls free function.", true);

        TEST_MESSAGE("Event subscribe and invoke");
        Event<void()> evt;
        g_functionResult = 0;
        evt.Subscribe(this, &SystemOperationTest::OnTestEvent);
        evt.Invoke();
        EXPECT_TRUE(g_functionResult == 100, "Event invokes subscriber.", true);
        g_functionResult = 0;

        TEST_MESSAGE("Event unsubscribe");
        evt.Unsubscribe(this);
        evt.Invoke();
        EXPECT_TRUE(g_functionResult == 0, "After unsubscribe, no effect.", true);

        TEST_MESSAGE("Event multiple subscribers");
        Event<void(int)> evt2;
        evt2.Subscribe(this, &SystemOperationTest::OnTestEventInt);
        memberValue = 0;
        evt2.Invoke(55);
        EXPECT_TRUE(memberValue == 55, "Event with args works.", true);

        TEST_MESSAGE("Event clear");
        Event<void()> evt3;
        evt3.Subscribe(this, &SystemOperationTest::OnTestEvent);
        evt3.Subscribe(this, &SystemOperationTest::OnTestEvent);
        evt3.Clear();
        g_functionResult = 0;
        evt3.Invoke();
        EXPECT_TRUE(g_functionResult == 0, "Clear removes all subscribers.", true);

        SUCCESS_MESSAGE("SystemOperation");
        return true;
    }

  private:
    void OnTestEvent() { g_functionResult = 100; }
    void OnTestEventInt(int v) { memberValue = v; }
};

#endif