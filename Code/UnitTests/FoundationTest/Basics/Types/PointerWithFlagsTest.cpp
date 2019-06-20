#include <FoundationTestPCH.h>

#include <Foundation/Types/PointerWithFlags.h>

EZ_CREATE_SIMPLE_TEST(Basics, PointerWithFlags)
{
  struct Dummy
  {
    float a = 3.0f;
    int b = 7;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "General")
  {
    ezPointerWithFlags<Dummy, 2> ptr;

    EZ_TEST_INT(ptr.GetFlags(), 0);
    ptr.SetFlags(3);
    EZ_TEST_INT(ptr.GetFlags(), 3);

    EZ_TEST_BOOL(ptr == nullptr);
    EZ_TEST_BOOL(!ptr);

    EZ_TEST_INT(ptr.GetFlags(), 3);
    ptr.SetFlags(2);
    EZ_TEST_INT(ptr.GetFlags(), 2);

    Dummy d1, d2;
    ptr = &d1;
    d2.a = 4;
    d2.b = 8;

    EZ_TEST_BOOL(ptr.GetPtr() == &d1);
    EZ_TEST_BOOL(ptr.GetPtr() != &d2);

    EZ_TEST_INT(ptr.GetFlags(), 2);
    ptr.SetFlags(1);
    EZ_TEST_INT(ptr.GetFlags(), 1);

    EZ_TEST_BOOL(ptr == &d1);
    EZ_TEST_BOOL(ptr != &d2);
    EZ_TEST_BOOL(ptr);


    EZ_TEST_FLOAT(ptr->a, 3.0f, 0.0f);
    EZ_TEST_INT(ptr->b, 7);

    ptr = &d2;

    EZ_TEST_INT(ptr.GetFlags(), 1);
    ptr.SetFlags(3);
    EZ_TEST_INT(ptr.GetFlags(), 3);

    EZ_TEST_BOOL(ptr != &d1);
    EZ_TEST_BOOL(ptr == &d2);
    EZ_TEST_BOOL(ptr);

    ptr = nullptr;
    EZ_TEST_BOOL(!ptr);
    EZ_TEST_BOOL(ptr == nullptr);

    EZ_TEST_INT(ptr.GetFlags(), 3);
    ptr.SetFlags(0);
    EZ_TEST_INT(ptr.GetFlags(), 0);

    ezPointerWithFlags<Dummy, 2> ptr2 = ptr;
    EZ_TEST_BOOL(ptr == ptr2);

    EZ_TEST_BOOL(ptr2.GetPtr() == ptr.GetPtr());
    EZ_TEST_BOOL(ptr2.GetFlags() == ptr.GetFlags());

    ptr2.SetFlags(3);
    EZ_TEST_BOOL(ptr2.GetPtr() == ptr.GetPtr());
    EZ_TEST_BOOL(ptr2.GetFlags() != ptr.GetFlags());

    // the two Ptrs still compare equal (pointer part is equal, even if flags are different)
    EZ_TEST_BOOL(ptr == ptr2);
  }
}
