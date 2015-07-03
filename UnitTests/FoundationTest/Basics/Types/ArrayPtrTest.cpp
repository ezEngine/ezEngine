#include <PCH.h>
#include <Foundation/Containers/DynamicArray.h>

static void testArrayPtr(ezArrayPtr<ezInt32> ints, ezInt32* pExtectedPtr, ezUInt32 uiExpectedCount)
{
  EZ_TEST_BOOL(ints.GetPtr() == pExtectedPtr);
  EZ_TEST_INT(ints.GetCount(), uiExpectedCount);
}

EZ_CREATE_SIMPLE_TEST(Basics, ArrayPtr)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Empty Constructor")
  {
    ezArrayPtr<ezInt32> Empty;

    EZ_TEST_BOOL(Empty.GetPtr() == nullptr);
    EZ_TEST_BOOL(Empty.GetCount() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<ezInt32> ap(pIntData, 3);
    EZ_TEST_BOOL(ap.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap.GetCount() == 3);

    ezArrayPtr<ezInt32> ap2(pIntData, 0);
    EZ_TEST_BOOL(ap2.GetPtr() == nullptr);
    EZ_TEST_BOOL(ap2.GetCount() == 0);

    ezArrayPtr<ezInt32> ap3(pIntData);
    EZ_TEST_BOOL(ap3.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap3.GetCount() == 5);

    ezArrayPtr<ezInt32> ap4(ap);
    EZ_TEST_BOOL(ap4.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap4.GetCount() == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "szMakeArrayPtr")
  {
    ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    testArrayPtr(ezMakeArrayPtr(pIntData, 3), pIntData, 3);
    testArrayPtr(ezMakeArrayPtr(pIntData, 0), nullptr, 0);
    testArrayPtr(ezMakeArrayPtr(pIntData), pIntData, 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=")
  {
    ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<ezInt32> ap(pIntData, 3);
    EZ_TEST_BOOL(ap.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap.GetCount() == 3);

    ezArrayPtr<ezInt32> ap2;
    ap2 = ap;

    EZ_TEST_BOOL(ap2.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap2.GetCount() == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reset")
  {
    ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<ezInt32> ap(pIntData, 3);
    EZ_TEST_BOOL(ap.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap.GetCount() == 3);

    ap.Reset();

    EZ_TEST_BOOL(ap.GetPtr() == nullptr);
    EZ_TEST_BOOL(ap.GetCount() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== / operator!=")
  {
    ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<ezInt32> ap1(pIntData, 3);
    ezArrayPtr<ezInt32> ap2(pIntData, 3);
    ezArrayPtr<ezInt32> ap3(pIntData, 4);
    ezArrayPtr<ezInt32> ap4(pIntData + 1, 3);

    EZ_TEST_BOOL(ap1 == ap2);
    EZ_TEST_BOOL(ap1 != ap3);
    EZ_TEST_BOOL(ap1 != ap4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator[]")
  {
    ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<ezInt32> ap(pIntData + 1, 3);
    EZ_TEST_INT(ap[0], 2);
    EZ_TEST_INT(ap[1], 3);
    EZ_TEST_INT(ap[2], 4);
    ap[2] = 10;
    EZ_TEST_INT(ap[2], 10);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "const operator[]")
  {
    ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    const ezArrayPtr<ezInt32> ap(pIntData + 1, 3);
    EZ_TEST_INT(ap[0], 2);
    EZ_TEST_INT(ap[1], 3);
    EZ_TEST_INT(ap[2], 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CopyFrom")
  {
    ezInt32 pIntData1[] = { 1, 2, 3, 4, 5 };
    ezInt32 pIntData2[] = { 6, 7, 8, 9, 0 };

    ezArrayPtr<ezInt32> ap1(pIntData1 + 1, 3);
    ezArrayPtr<ezInt32> ap2(pIntData2 + 2, 3);

    ap1.CopyFrom(ap2);

    EZ_TEST_INT(pIntData1[0], 1);
    EZ_TEST_INT(pIntData1[1], 8);
    EZ_TEST_INT(pIntData1[2], 9);
    EZ_TEST_INT(pIntData1[3], 0);
    EZ_TEST_INT(pIntData1[4], 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetSubArray")
  {
    ezInt32 pIntData1[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<ezInt32> ap1(pIntData1, 5);
    ezArrayPtr<ezInt32> ap2 = ap1.GetSubArray(2, 3);

    EZ_TEST_BOOL(ap2.GetPtr() == &pIntData1[2]);
    EZ_TEST_BOOL(ap2.GetCount() == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Const Conversions")
  {
    ezInt32 pIntData1[] = { 1, 2, 3, 4, 5 };
    ezArrayPtr<ezInt32> ap1(pIntData1);
    ezArrayPtr<const ezInt32> ap2(ap1);
    ezArrayPtr<const ezInt32> ap3(pIntData1);
    ap2 = ap1; // non const to const assign
    ap3 = ap2; // const to const assign
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Empty Constructor (const)")
  {
    ezArrayPtr<const ezInt32> Empty;

    EZ_TEST_BOOL(Empty.GetPtr() == nullptr);
    EZ_TEST_BOOL(Empty.GetCount() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (const)")
  {
    const ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<const ezInt32> ap(pIntData, 3);
    EZ_TEST_BOOL(ap.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap.GetCount() == 3);

    ezArrayPtr<const ezInt32> ap2(pIntData, 0);
    EZ_TEST_BOOL(ap2.GetPtr() == nullptr);
    EZ_TEST_BOOL(ap2.GetCount() == 0);

    ezArrayPtr<const ezInt32> ap3(pIntData);
    EZ_TEST_BOOL(ap3.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap3.GetCount() == 5);

    ezArrayPtr<const ezInt32> ap4(ap);
    EZ_TEST_BOOL(ap4.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap4.GetCount() == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=  (const)")
  {
    const ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<const ezInt32> ap(pIntData, 3);
    EZ_TEST_BOOL(ap.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap.GetCount() == 3);

    ezArrayPtr<const ezInt32> ap2;
    ap2 = ap;

    EZ_TEST_BOOL(ap2.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap2.GetCount() == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reset  (const)")
  {
    const ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<const ezInt32> ap(pIntData, 3);
    EZ_TEST_BOOL(ap.GetPtr() == pIntData);
    EZ_TEST_BOOL(ap.GetCount() == 3);

    ap.Reset();

    EZ_TEST_BOOL(ap.GetPtr() == nullptr);
    EZ_TEST_BOOL(ap.GetCount() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== / operator!=  (const)")
  {
    ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<ezInt32> ap1(pIntData, 3);
    ezArrayPtr<const ezInt32> ap2(pIntData, 3);
    ezArrayPtr<const ezInt32> ap3(pIntData, 4);
    ezArrayPtr<const ezInt32> ap4(pIntData + 1, 3);

    EZ_TEST_BOOL(ap1 == ap2);
    EZ_TEST_BOOL(ap3 != ap1);
    EZ_TEST_BOOL(ap1 != ap4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator[]  (const)")
  {
    const ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<const ezInt32> ap(pIntData + 1, 3);
    EZ_TEST_INT(ap[0], 2);
    EZ_TEST_INT(ap[1], 3);
    EZ_TEST_INT(ap[2], 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "const operator[] (const)")
  {
    const ezInt32 pIntData[] = { 1, 2, 3, 4, 5 };

    const ezArrayPtr<const ezInt32> ap(pIntData + 1, 3);
    EZ_TEST_INT(ap[0], 2);
    EZ_TEST_INT(ap[1], 3);
    EZ_TEST_INT(ap[2], 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetSubArray (const)")
  {
    const ezInt32 pIntData1[] = { 1, 2, 3, 4, 5 };

    ezArrayPtr<const ezInt32> ap1(pIntData1, 5);
    ezArrayPtr<const ezInt32> ap2 = ap1.GetSubArray(2, 3);

    EZ_TEST_BOOL(ap2.GetPtr() == &pIntData1[2]);
    EZ_TEST_BOOL(ap2.GetCount() == 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Iterator")
  {
    ezDynamicArray<ezInt32> a1;

    for (ezInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    ezArrayPtr<ezInt32> ptr1 = a1;

    // STL sort
    std::sort(begin(ptr1), end(ptr1));

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      EZ_TEST_BOOL(ptr1[i - 1] <= ptr1[i]);
    }

    // foreach
    ezUInt32 prev = 0;
    for(ezUInt32 val : ptr1)
    {
      EZ_TEST_BOOL(prev <= val);
      prev = val;
    }

    // const array
    const ezDynamicArray<ezInt32>& a2 = a1;

    const ezArrayPtr<const ezInt32> ptr2 = a2;

    // STL lower bound
    auto lb = std::lower_bound(begin(ptr2), end(ptr2), 400);
    EZ_TEST_BOOL(*lb == ptr2[400]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STL Reverse Iterator")
  {
    ezDynamicArray<ezInt32> a1;

    for (ezInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    ezArrayPtr<ezInt32> ptr1 = a1;

    // STL sort
    std::sort(rbegin(ptr1), rend(ptr1));

    for (ezInt32 i = 1; i < 1000; ++i)
    {
      EZ_TEST_BOOL(ptr1[i - 1] >= ptr1[i]);
    }

    // foreach
    ezUInt32 prev = 1000;
    for(ezUInt32 val : ptr1)
    {
      EZ_TEST_BOOL(prev >= val);
      prev = val;
    }

    // const array
    const ezDynamicArray<ezInt32>& a2 = a1;

    const ezArrayPtr<const ezInt32> ptr2 = a2;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(ptr2), rend(ptr2), 400);
    EZ_TEST_BOOL(*lb == ptr2[1000 - 400 - 1]);
  }

}
