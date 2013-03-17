#include <PCH.h>

//////////////////////////////////////////////////////////////////////
// Start of the definition of a example Enum
// It takes quite some lines of code to define a enum, 
// but it could be encapsulated into an preprocessor macro if wanted
struct ezTestEnumBase
{
  enum Enum
  {
    No = 0,
    Yes = 1,
    DefaultInit = No //Default initialization
  };
};

typedef ezEnum<ezTestEnumBase,   //The base for the enum
  ezUInt8           //The storage type for the enum
> ezTestEnum;      //The name of the final enum
// End of the definition of a example enum
///////////////////////////////////////////////////////////////////////

struct ezTestEnum2Base
{
  enum Enum
  {
    Bit1 = 1 << 0,
    Bit2 = 1 << 1,
    DefaultInit = Bit1
  };
};

typedef ezEnum<ezTestEnum2Base, ezUInt16> ezTestEnum2;

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
  EZ_TEST_BLOCK(true, "Default initialized enum")
  {
    ezTestEnum e1;
  }

  EZ_TEST_BLOCK(true, "Enum with explicit initialization")
  {
    ezTestEnum e2(ezTestEnum::Yes);
  }

  EZ_TEST_BLOCK(true, "This tests if the default initialization works and if the implicit conversion works")
  {
    ezTestEnum e1;
    ezTestEnum e2(ezTestEnum::Yes);

    EZ_TEST(e1 == ezTestEnum::No);
    EZ_TEST(e2 == ezTestEnum::Yes);
  }

  EZ_TEST_BLOCK(true, "Function call tests")
  {
    ezTestEnum e1;

    TakeEnum1(e1);
    TakeEnum2(e1);
  }

  EZ_TEST_BLOCK(true, "Assignment of different values")
  {
    ezTestEnum e1, e2;

    e1 = ezTestEnum::Yes;
    e2 = ezTestEnum::No;
    EZ_TEST(e1 == ezTestEnum::Yes);
    EZ_TEST(e2 == ezTestEnum::No);

    e1 = e2;
    EZ_TEST(e1 == ezTestEnum::No);
  }

  EZ_TEST_BLOCK(true, "Test the | operator")
  {
    ezTestEnum2 e3(ezTestEnum2::Bit1);
    ezTestEnum2 e4(ezTestEnum2::Bit2);
    ezUInt16 uiBits = (e3 | e4);
    EZ_TEST(uiBits == (ezTestEnum2::Bit1 | ezTestEnum2::Bit2));
  }


  EZ_TEST_BLOCK(true, "Test the & operator")
  {
    ezTestEnum2 e3(ezTestEnum2::Bit1);
    ezTestEnum2 e4(ezTestEnum2::Bit2);
    ezUInt16 uiBits = (e3 | e4) & e4;
    EZ_TEST(uiBits == ezTestEnum2::Bit2);
  }

  EZ_TEST_BLOCK(true, "Test conversion to int")
  {
    ezTestEnum e1;
    int iTest = e1;
    EZ_TEST(iTest == ezTestEnum::No);
  }
}

