#include <PCH.h>

//////////////////////////////////////////////////////////////////////
// Start of the definition of a example Enum
// It takes quite some lines of code to define a enum, 
// but it could be encapsulated into an preprocessor macro if wanted
struct ezTestEnumBase
{
  typedef ezUInt8 StorageType; // The storage type for the enum

  enum Enum
  {
    No = 0,
    Yes = 1,
    Default = No //Default initialization
  };
};

typedef ezEnum<ezTestEnumBase //The base for the enum
  > ezTestEnum;      //The name of the final enum
// End of the definition of a example enum
///////////////////////////////////////////////////////////////////////

struct ezTestEnum2Base
{
  typedef ezUInt16 StorageType;

  enum Enum
  {
    Bit1 = EZ_BIT(0),
    Bit2 = EZ_BIT(1),
    Default = Bit1
  };
};

typedef ezEnum<ezTestEnum2Base> ezTestEnum2;

//Test if the type actually has the requested size
EZ_CHECK_AT_COMPILETIME(sizeof(ezTestEnum) == sizeof(ezUInt8));
EZ_CHECK_AT_COMPILETIME(sizeof(ezTestEnum2) == sizeof(ezUInt16));

EZ_CREATE_SIMPLE_TEST_GROUP(Basics);

//This takes a c++ enum. Tests the implict conversion
void TakeEnum1(ezTestEnum::Enum value)
{
}

//This takes our own enum type
void TakeEnum2(ezTestEnum value)
{
}

EZ_CREATE_SIMPLE_TEST(Basics, Enum)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default initialized enum")
  {
    ezTestEnum e1;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enum with explicit initialization")
  {
    ezTestEnum e2(ezTestEnum::Yes);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "This tests if the default initialization works and if the implicit conversion works")
  {
    ezTestEnum e1;
    ezTestEnum e2(ezTestEnum::Yes);

    EZ_TEST_BOOL(e1 == ezTestEnum::No);
    EZ_TEST_BOOL(e2 == ezTestEnum::Yes);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Function call tests")
  {
    ezTestEnum e1;

    TakeEnum1(e1);
    TakeEnum2(e1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Assignment of different values")
  {
    ezTestEnum e1, e2;

    e1 = ezTestEnum::Yes;
    e2 = ezTestEnum::No;
    EZ_TEST_BOOL(e1 == ezTestEnum::Yes);
    EZ_TEST_BOOL(e2 == ezTestEnum::No);

    e1 = e2;
    EZ_TEST_BOOL(e1 == ezTestEnum::No);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Test the | operator")
  {
    ezTestEnum2 e3(ezTestEnum2::Bit1);
    ezTestEnum2 e4(ezTestEnum2::Bit2);
    ezUInt16 uiBits = (e3 | e4);
    EZ_TEST_BOOL(uiBits == (ezTestEnum2::Bit1 | ezTestEnum2::Bit2));
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Test the & operator")
  {
    ezTestEnum2 e3(ezTestEnum2::Bit1);
    ezTestEnum2 e4(ezTestEnum2::Bit2);
    ezUInt16 uiBits = (e3 | e4) & e4;
    EZ_TEST_BOOL(uiBits == ezTestEnum2::Bit2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Test conversion to int")
  {
    ezTestEnum e1;
    int iTest = e1;
    EZ_TEST_BOOL(iTest == ezTestEnum::No);
  }
}

