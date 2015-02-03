#include <PCH.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>


EZ_CREATE_SIMPLE_TEST(Math, Color8UNorm)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor empty")
  {
    // Placement new of the default constructor should not have any effect on the previous data.
    ezUInt8 testBlock[4] = { 0, 64, 128, 255 };
    ezColorLinearUB* pDefCtor = ::new ((void*)&testBlock[0]) ezColorLinearUB;
    EZ_TEST_BOOL(pDefCtor->r == 0 && 
      pDefCtor->g == 64 && 
      pDefCtor->b == 128 && 
      pDefCtor->a == 255);

    // Make sure the class didn't accidentally change in size
    EZ_TEST_BOOL(sizeof(ezColorLinearUB) == sizeof(ezUInt8)*4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor components")
  {
    ezColorLinearUB init3(100, 123, 255);
    EZ_TEST_BOOL(init3.r == 100 && init3.g == 123 && init3.b == 255 && init3.a == 255);

    ezColorLinearUB init4(100, 123, 255, 42);
    EZ_TEST_BOOL(init4.r == 100 && init4.g == 123 && init4.b == 255 && init4.a == 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor copy")
  {
    ezColorLinearUB init4(100, 123, 255, 42);
    ezColorLinearUB copy(init4);
    EZ_TEST_BOOL(copy.r == 100 && copy.g == 123 && copy.b == 255 && copy.a == 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor ezColor")
  {
    ezColorLinearUB fromColor32f(ezColor(0.39f, 0.58f, 0.93f));
    EZ_TEST_BOOL(ezMath::IsEqual<ezUInt8>(fromColor32f.r, static_cast<ezUInt8>(ezColor(0.39f, 0.58f, 0.93f).r * 255), 2) &&
      ezMath::IsEqual<ezUInt8>(fromColor32f.g, static_cast<ezUInt8>(ezColor(0.39f, 0.58f, 0.93f).g * 255), 2) &&
      ezMath::IsEqual<ezUInt8>(fromColor32f.b, static_cast<ezUInt8>(ezColor(0.39f, 0.58f, 0.93f).b * 255), 2) &&
      ezMath::IsEqual<ezUInt8>(fromColor32f.a, static_cast<ezUInt8>(ezColor(0.39f, 0.58f, 0.93f).a * 255), 2));
  }

  // conversion
  {
    ezColorLinearUB cornflowerBlue(ezColor(0.39f, 0.58f, 0.93f));

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion ezColor")
    {
      ezColor color32f = cornflowerBlue;
      EZ_TEST_BOOL(ezMath::IsEqual<float>(color32f.r, ezColor(0.39f, 0.58f, 0.93f).r, 2.0f / 255.0f) &&
        ezMath::IsEqual<float>(color32f.g, ezColor(0.39f, 0.58f, 0.93f).g, 2.0f / 255.0f) &&
        ezMath::IsEqual<float>(color32f.b, ezColor(0.39f, 0.58f, 0.93f).b, 2.0f / 255.0f) &&
        ezMath::IsEqual<float>(color32f.a, ezColor(0.39f, 0.58f, 0.93f).a, 2.0f / 255.0f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion ezUInt*")
    {
      const ezUInt8* pUIntsConst = cornflowerBlue.GetData();
      EZ_TEST_BOOL(pUIntsConst[0] == cornflowerBlue.r && pUIntsConst[1] == cornflowerBlue.g && pUIntsConst[2] == cornflowerBlue.b && pUIntsConst[3] == cornflowerBlue.a);

      ezUInt8* pUInts = cornflowerBlue.GetData();
      EZ_TEST_BOOL(pUInts[0] == cornflowerBlue.r && pUInts[1] == cornflowerBlue.g && pUInts[2] == cornflowerBlue.b && pUInts[3] == cornflowerBlue.a);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezColorGammaUB: Constructor")
  {
    ezColorGammaUB c(50, 150, 200, 100);
    EZ_TEST_INT(c.r, 50);
    EZ_TEST_INT(c.g, 150);
    EZ_TEST_INT(c.b, 200);
    EZ_TEST_INT(c.a, 100);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezColorGammaUB: Constructor (ezColor)")
  {
    ezColorGammaUB c2 = ezColor::RebeccaPurple;

    ezColor c3 = c2;

    EZ_TEST_BOOL(c3.IsEqualRGBA(ezColor::RebeccaPurple, 0.001f));
  }
}



