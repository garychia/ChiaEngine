#ifndef SYSTEM_OPERATION_TEST_HPP
#define SYSTEM_OPERATION_TEST_HPP

#include "Test.hpp"
#include "System/Operation/Function.hpp"
#include "System/Operation/Event.hpp"

namespace operationtest
{

static int g_functionResult = 0;
static void FreeFunctionVoid() { g_functionResult = 42; }
static int FreeFunctionInt(int x) { return x * 2; }

static int g_orderLog[8] = {0};
static int g_orderCount = 0;
static int g_invokeCount = 0;

// Distinct subscriber objects — different addresses, so the OLD hash-table
// implementation dispatched them in address order; the NEW one must dispatch
// in subscribe order.
class OrderSubscriberA
{
  public:
    void OnFire() { g_orderLog[g_orderCount++] = 1; }
    void OnFireInt(int) { g_orderLog[g_orderCount++] = 1; }
};

class OrderSubscriberB
{
  public:
    void OnFire() { g_orderLog[g_orderCount++] = 2; }
    void OnFireInt(int) { g_orderLog[g_orderCount++] = 2; }
};

class OrderSubscriberC
{
  public:
    void OnFire() { g_orderLog[g_orderCount++] = 3; }
    void OnFireInt(int) { g_orderLog[g_orderCount++] = 3; }
};

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

    void OnTestEvent() { g_functionResult = 100; }
    void OnTestEventInt(int v) { memberValue = v; }
    void OnIncrement() { g_invokeCount++; }

    bool Run() noexcept override
    {
        g_invokeCount = 0;

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

        TEST_MESSAGE("Event with args");
        Event<void(int)> evt2;
        evt2.Subscribe(this, &SystemOperationTest::OnTestEventInt);
        memberValue = 0;
        evt2.Invoke(55);
        EXPECT_TRUE(memberValue == 55, "Event with args works.", true);

        TEST_MESSAGE("Event clear");
        Event<void()> evt3;
        evt3.Subscribe(this, &SystemOperationTest::OnTestEvent);
        evt3.Clear();
        g_functionResult = 0;
        evt3.Invoke();
        EXPECT_TRUE(g_functionResult == 0, "Clear removes all subscribers.", true);

        // ────────────────────────────────────────────────────────────
        // #79 regression tests: subscription-ordered dispatch, duplicate
        // subscribe replaces, unsubscribe removes only target, no leaks.
        // ────────────────────────────────────────────────────────────

        TEST_MESSAGE("Event dispatch in SUBSCRIPTION order (#79)");
        {
            OrderSubscriberA a;
            OrderSubscriberB b;
            OrderSubscriberC c;
            Event<void()> orderEvt;

            g_orderCount = 0;
            orderEvt.Subscribe(&a, &OrderSubscriberA::OnFire);
            orderEvt.Subscribe(&b, &OrderSubscriberB::OnFire);
            orderEvt.Subscribe(&c, &OrderSubscriberC::OnFire);
            orderEvt.Invoke();

            EXPECT_TRUE(g_orderCount == 3, "All three subscribers fired.", true);
            EXPECT_TRUE(g_orderLog[0] == 1, "Subscriber A fired FIRST (subscribe order).", true);
            EXPECT_TRUE(g_orderLog[1] == 2, "Subscriber B fired SECOND.", true);
            EXPECT_TRUE(g_orderLog[2] == 3, "Subscriber C fired THIRD.", true);
        }

        TEST_MESSAGE("Event duplicate subscribe replaces, not adds (#79)");
        {
            OrderSubscriberA a;
            OrderSubscriberB b;
            Event<void()> dupEvt;

            dupEvt.Subscribe(&a, &OrderSubscriberA::OnFire);
            dupEvt.Subscribe(&b, &OrderSubscriberB::OnFire);
            // Re-subscribe A: must REPLACE the existing A slot (same object).
            dupEvt.Subscribe(&a, &OrderSubscriberA::OnFire);
            EXPECT_TRUE(dupEvt.Length() == 2, "Duplicate subscribe replaced, not added.", true);

            // A fires once, B fires once — order preserved.
            g_orderCount = 0;
            dupEvt.Invoke();
            EXPECT_TRUE(g_orderCount == 2, "Exactly two fires, not three.", true);
            EXPECT_TRUE(g_orderLog[0] == 1 && g_orderLog[1] == 2, "Order A then B preserved.", true);
        }

        TEST_MESSAGE("Event unsubscribe removes ONLY the target (#79)");
        {
            OrderSubscriberA a;
            OrderSubscriberB b;
            OrderSubscriberC c;
            Event<void()> unSubEvt;

            unSubEvt.Subscribe(&a, &OrderSubscriberA::OnFire);
            unSubEvt.Subscribe(&b, &OrderSubscriberB::OnFire);
            unSubEvt.Subscribe(&c, &OrderSubscriberC::OnFire);
            unSubEvt.Unsubscribe(&b);

            g_orderCount = 0;
            unSubEvt.Invoke();
            EXPECT_TRUE(g_orderCount == 2, "Only two subscribers after unsubscribing B.", true);
            EXPECT_TRUE(g_orderLog[0] == 1, "A still first.", true);
            EXPECT_TRUE(g_orderLog[1] == 3, "C still third (B removed, order kept).", true);
        }

        TEST_MESSAGE("Event Clear then reuse stays empty (#79)");
        {
            OrderSubscriberA a;
            Event<void()> clearEvt;
            clearEvt.Subscribe(&a, &OrderSubscriberA::OnFire);
            clearEvt.Clear();
            g_orderCount = 0;
            clearEvt.Invoke();
            EXPECT_TRUE(g_orderCount == 0, "After Clear, no subscribers fire.", true);
            // Re-subscribe after clear (container must be reusable).
            clearEvt.Subscribe(&a, &OrderSubscriberA::OnFire);
            clearEvt.Invoke();
            EXPECT_TRUE(g_orderCount == 1, "Re-subscribe after Clear works.", true);
        }

        TEST_MESSAGE("Event lifetime: dtor cleans up all callbacks (#79)");
        {
            // In this scope, evt subscribes and the dtor fires at the closing brace.
            // Any double-free/leak in the dtor path fails here (or in ASan builds).
            Event<void(int)> lifetimeEvt;
            lifetimeEvt.Subscribe(this, &SystemOperationTest::OnTestEventInt);
            lifetimeEvt.Subscribe(this, &SystemOperationTest::OnTestEventInt); // dup replaced
            lifetimeEvt.Invoke(5);
            EXPECT_TRUE(memberValue == 5, "Still invokes after dup subscribe.", true);
        }

        SUCCESS_MESSAGE("SystemOperation");
        return true;
    }

  private:
};

} // namespace operationtest

#endif