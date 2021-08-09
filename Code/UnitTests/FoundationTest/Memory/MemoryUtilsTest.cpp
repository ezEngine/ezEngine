#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HybridArray.h>

struct ezConstructTest
{
public:
  static ezHybridArray<void*, 10> s_dtorList;

  ezConstructTest() { m_iData = 42; }

  ~ezConstructTest() { s_dtorList.PushBack(this); }

  ezInt32 m_iData;
};
ezHybridArray<void*, 10> ezConstructTest::s_dtorList;

EZ_CHECK_AT_COMPILETIME(sizeof(ezConstructTest) == 4);


struct PODTest
{
  EZ_DECLARE_POD_TYPE();

  PODTest() { m_iData = -1; }

  ezInt32 m_iData;
};

static const ezUInt32 uiSize = sizeof(ezConstructTest);

EZ_CREATE_SIMPLE_TEST(Memory, MemoryUtils)
{
  ezConstructTest::s_dtorList.Clear();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Construct")
  {
    ezUInt8 uiRawData[uiSize * 5] = {0};
    ezConstructTest* pTest = (ezConstructTest*)(uiRawData);

    ezMemoryUtils::Construct<ezConstructTest>(pTest + 1, 2);

    EZ_TEST_INT(pTest[0].m_iData, 0);
    EZ_TEST_INT(pTest[1].m_iData, 42);
    EZ_TEST_INT(pTest[2].m_iData, 42);
    EZ_TEST_INT(pTest[3].m_iData, 0);
    EZ_TEST_INT(pTest[4].m_iData, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeConstructorFunction")
  {
    ezMemoryUtils::ConstructorFunction func = ezMemoryUtils::MakeConstructorFunction<ezConstructTest>();
    EZ_TEST_BOOL(func != nullptr);

    ezUInt8 uiRawData[uiSize] = {0};
    ezConstructTest* pTest = (ezConstructTest*)(uiRawData);

    (*func)(pTest);

    EZ_TEST_INT(pTest->m_iData, 42);

    func = ezMemoryUtils::MakeConstructorFunction<PODTest>();
    EZ_TEST_BOOL(func != nullptr);

    func = ezMemoryUtils::MakeConstructorFunction<ezInt32>();
    EZ_TEST_BOOL(func == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DefaultConstruct")
  {
    ezUInt32 uiRawData[5]; // not initialized here

    ezMemoryUtils::DefaultConstruct(uiRawData + 1, 2);

    EZ_TEST_INT(uiRawData[1], 0);
    EZ_TEST_INT(uiRawData[2], 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeDefaultConstructorFunction")
  {
    ezMemoryUtils::ConstructorFunction func = ezMemoryUtils::MakeDefaultConstructorFunction<ezInt32>();
    EZ_TEST_BOOL(func != nullptr);

    ezInt32 iTest = 2;

    (*func)(&iTest);

    EZ_TEST_INT(iTest, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Construct Copy(Array)")
  {
    ezUInt8 uiRawData[uiSize * 5] = {0};
    ezConstructTest* pTest = (ezConstructTest*)(uiRawData);

    ezConstructTest copy[2];
    copy[0].m_iData = 43;
    copy[1].m_iData = 44;

    ezMemoryUtils::CopyConstructArray<ezConstructTest>(pTest + 1, copy, 2);

    EZ_TEST_INT(pTest[0].m_iData, 0);
    EZ_TEST_INT(pTest[1].m_iData, 43);
    EZ_TEST_INT(pTest[2].m_iData, 44);
    EZ_TEST_INT(pTest[3].m_iData, 0);
    EZ_TEST_INT(pTest[4].m_iData, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Construct Copy(Element)")
  {
    ezUInt8 uiRawData[uiSize * 5] = {0};
    ezConstructTest* pTest = (ezConstructTest*)(uiRawData);

    ezConstructTest copy;
    copy.m_iData = 43;

    ezMemoryUtils::CopyConstruct<ezConstructTest>(pTest + 1, copy, 2);

    EZ_TEST_INT(pTest[0].m_iData, 0);
    EZ_TEST_INT(pTest[1].m_iData, 43);
    EZ_TEST_INT(pTest[2].m_iData, 43);
    EZ_TEST_INT(pTest[3].m_iData, 0);
    EZ_TEST_INT(pTest[4].m_iData, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeCopyConstructorFunction")
  {
    ezMemoryUtils::CopyConstructorFunction func = ezMemoryUtils::MakeCopyConstructorFunction<ezConstructTest>();
    EZ_TEST_BOOL(func != nullptr);

    ezUInt8 uiRawData[uiSize] = {0};
    ezConstructTest* pTest = (ezConstructTest*)(uiRawData);

    ezConstructTest copy;
    copy.m_iData = 43;

    (*func)(pTest, &copy);

    EZ_TEST_INT(pTest->m_iData, 43);

    func = ezMemoryUtils::MakeCopyConstructorFunction<PODTest>();
    EZ_TEST_BOOL(func != nullptr);

    func = ezMemoryUtils::MakeCopyConstructorFunction<ezInt32>();
    EZ_TEST_BOOL(func != nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Destruct")
  {
    ezUInt8 uiRawData[uiSize * 5] = {0};
    ezConstructTest* pTest = (ezConstructTest*)(uiRawData);

    ezMemoryUtils::Construct<ezConstructTest>(pTest + 1, 2);

    EZ_TEST_INT(pTest[0].m_iData, 0);
    EZ_TEST_INT(pTest[1].m_iData, 42);
    EZ_TEST_INT(pTest[2].m_iData, 42);
    EZ_TEST_INT(pTest[3].m_iData, 0);
    EZ_TEST_INT(pTest[4].m_iData, 0);

    ezConstructTest::s_dtorList.Clear();
    ezMemoryUtils::Destruct<ezConstructTest>(pTest, 4);
    EZ_TEST_INT(4, ezConstructTest::s_dtorList.GetCount());

    if (ezConstructTest::s_dtorList.GetCount() == 4)
    {
      EZ_TEST_BOOL(ezConstructTest::s_dtorList[0] == &pTest[3]);
      EZ_TEST_BOOL(ezConstructTest::s_dtorList[1] == &pTest[2]);
      EZ_TEST_BOOL(ezConstructTest::s_dtorList[2] == &pTest[1]);
      EZ_TEST_BOOL(ezConstructTest::s_dtorList[3] == &pTest[0]);
      EZ_TEST_INT(pTest[4].m_iData, 0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeDestructorFunction")
  {
    ezMemoryUtils::DestructorFunction func = ezMemoryUtils::MakeDestructorFunction<ezConstructTest>();
    EZ_TEST_BOOL(func != nullptr);

    ezUInt8 uiRawData[uiSize] = {0};
    ezConstructTest* pTest = (ezConstructTest*)(uiRawData);

    ezMemoryUtils::Construct(pTest, 1);
    EZ_TEST_INT(pTest->m_iData, 42);

    ezConstructTest::s_dtorList.Clear();
    (*func)(pTest);
    EZ_TEST_INT(1, ezConstructTest::s_dtorList.GetCount());

    if (ezConstructTest::s_dtorList.GetCount() == 1)
    {
      EZ_TEST_BOOL(ezConstructTest::s_dtorList[0] == pTest);
    }

    func = ezMemoryUtils::MakeDestructorFunction<PODTest>();
    EZ_TEST_BOOL(func == nullptr);

    func = ezMemoryUtils::MakeDestructorFunction<ezInt32>();
    EZ_TEST_BOOL(func == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy")
  {
    ezUInt8 uiRawData[5] = {1, 2, 3, 4, 5};
    ezUInt8 uiRawData2[5] = {6, 7, 8, 9, 0};

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 2);
    EZ_TEST_INT(uiRawData[2], 3);
    EZ_TEST_INT(uiRawData[3], 4);
    EZ_TEST_INT(uiRawData[4], 5);

    ezMemoryUtils::Copy(uiRawData + 1, uiRawData2 + 2, 3);

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 8);
    EZ_TEST_INT(uiRawData[2], 9);
    EZ_TEST_INT(uiRawData[3], 0);
    EZ_TEST_INT(uiRawData[4], 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move")
  {
    ezUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 2);
    EZ_TEST_INT(uiRawData[2], 3);
    EZ_TEST_INT(uiRawData[3], 4);
    EZ_TEST_INT(uiRawData[4], 5);

    ezMemoryUtils::CopyOverlapped(uiRawData + 1, uiRawData + 3, 2);

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 4);
    EZ_TEST_INT(uiRawData[2], 5);
    EZ_TEST_INT(uiRawData[3], 4);
    EZ_TEST_INT(uiRawData[4], 5);

    ezMemoryUtils::CopyOverlapped(uiRawData + 1, uiRawData, 4);

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 1);
    EZ_TEST_INT(uiRawData[2], 4);
    EZ_TEST_INT(uiRawData[3], 5);
    EZ_TEST_INT(uiRawData[4], 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezUInt8 uiRawData1[5] = {1, 2, 3, 4, 5};
    ezUInt8 uiRawData2[5] = {1, 2, 3, 4, 5};
    ezUInt8 uiRawData3[5] = {1, 2, 3, 4, 6};

    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(uiRawData1, uiRawData2, 5));
    EZ_TEST_BOOL(!ezMemoryUtils::IsEqual(uiRawData1, uiRawData3, 5));
    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(uiRawData1, uiRawData3, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ZeroFill")
  {
    ezUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 2);
    EZ_TEST_INT(uiRawData[2], 3);
    EZ_TEST_INT(uiRawData[3], 4);
    EZ_TEST_INT(uiRawData[4], 5);

    // T*, size_t N overload
    ezMemoryUtils::ZeroFill(uiRawData + 1, 3);

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 0);
    EZ_TEST_INT(uiRawData[2], 0);
    EZ_TEST_INT(uiRawData[3], 0);
    EZ_TEST_INT(uiRawData[4], 5);

    // T[N] overload
    ezMemoryUtils::ZeroFill(uiRawData);

    EZ_TEST_INT(uiRawData[0], 0);
    EZ_TEST_INT(uiRawData[1], 0);
    EZ_TEST_INT(uiRawData[2], 0);
    EZ_TEST_INT(uiRawData[3], 0);
    EZ_TEST_INT(uiRawData[4], 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PatternFill")
  {
    ezUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 2);
    EZ_TEST_INT(uiRawData[2], 3);
    EZ_TEST_INT(uiRawData[3], 4);
    EZ_TEST_INT(uiRawData[4], 5);

    // T*, size_t N overload
    ezMemoryUtils::PatternFill(uiRawData + 1, 0xAB, 3);

    EZ_TEST_INT(uiRawData[0], 1);
    EZ_TEST_INT(uiRawData[1], 0xAB);
    EZ_TEST_INT(uiRawData[2], 0xAB);
    EZ_TEST_INT(uiRawData[3], 0xAB);
    EZ_TEST_INT(uiRawData[4], 5);

    // T[N] overload
    ezMemoryUtils::PatternFill(uiRawData, 0xCD);

    EZ_TEST_INT(uiRawData[0], 0xCD);
    EZ_TEST_INT(uiRawData[1], 0xCD);
    EZ_TEST_INT(uiRawData[2], 0xCD);
    EZ_TEST_INT(uiRawData[3], 0xCD);
    EZ_TEST_INT(uiRawData[4], 0xCD);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compare")
  {
    ezUInt32 uiRawDataA[3] = {1, 2, 3};
    ezUInt32 uiRawDataB[3] = {3, 4, 5};

    EZ_TEST_INT(uiRawDataA[0], 1);
    EZ_TEST_INT(uiRawDataA[1], 2);
    EZ_TEST_INT(uiRawDataA[2], 3);
    EZ_TEST_INT(uiRawDataB[0], 3);
    EZ_TEST_INT(uiRawDataB[1], 4);
    EZ_TEST_INT(uiRawDataB[2], 5);

    EZ_TEST_BOOL(ezMemoryUtils::Compare(uiRawDataA, uiRawDataB, 3) < 0);
    EZ_TEST_BOOL(ezMemoryUtils::Compare(uiRawDataA + 2, uiRawDataB, 1) == 0);
    EZ_TEST_BOOL(ezMemoryUtils::Compare(uiRawDataB, uiRawDataA, 3) > 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddByteOffset")
  {
    ezInt32* pData1 = nullptr;
    pData1 = ezMemoryUtils::AddByteOffset(pData1, 13);
    EZ_TEST_BOOL(pData1 == reinterpret_cast<ezInt32*>(13));

    const ezInt32* pData2 = nullptr;
    const ezInt32* pData3 = ezMemoryUtils::AddByteOffset(pData2, 17);
    EZ_TEST_BOOL(pData3 == reinterpret_cast<ezInt32*>(17));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Align / IsAligned")
  {
    {
      ezInt32* pData = (ezInt32*)1;
      EZ_TEST_BOOL(!ezMemoryUtils::IsAligned(pData, 4));
      pData = ezMemoryUtils::Align(pData, 4);
      EZ_TEST_BOOL(pData == reinterpret_cast<ezInt32*>(0));
      EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pData, 4));
    }
    {
      ezInt32* pData = (ezInt32*)2;
      EZ_TEST_BOOL(!ezMemoryUtils::IsAligned(pData, 4));
      pData = ezMemoryUtils::Align(pData, 4);
      EZ_TEST_BOOL(pData == reinterpret_cast<ezInt32*>(0));
      EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pData, 4));
    }
    {
      ezInt32* pData = (ezInt32*)3;
      EZ_TEST_BOOL(!ezMemoryUtils::IsAligned(pData, 4));
      pData = ezMemoryUtils::Align(pData, 4);
      EZ_TEST_BOOL(pData == reinterpret_cast<ezInt32*>(0));
      EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pData, 4));
    }
    {
      ezInt32* pData = (ezInt32*)4;
      EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pData, 4));
      pData = ezMemoryUtils::Align(pData, 4);
      EZ_TEST_BOOL(pData == reinterpret_cast<ezInt32*>(4));
      EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pData, 4));
    }
  }
}
