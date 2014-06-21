#include <PCH.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>


EZ_CREATE_SIMPLE_TEST(Math, Color8UNorm)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor empty")
  {
    // Placement new of the default constructor should not have any effect on the previous data.
    ezUInt8 testBlock[4] = { 0, 64, 128, 255 };
    ezColor8UNorm* pDefCtor = ::new ((void*)&testBlock[0]) ezColor8UNorm;
    EZ_TEST_BOOL(pDefCtor->r == 0 && 
      pDefCtor->g == 64 && 
      pDefCtor->b == 128 && 
      pDefCtor->a == 255);

    // Make sure the class didn't accidentally change in size
    EZ_TEST_BOOL(sizeof(ezColor8UNorm) == sizeof(ezUInt8)*4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor components")
  {
    ezColor8UNorm init3(100, 123, 255);
    EZ_TEST_BOOL(init3.r == 100 && init3.g == 123 && init3.b == 255 && init3.a == 255);

    ezColor8UNorm init4(100, 123, 255, 42);
    EZ_TEST_BOOL(init4.r == 100 && init4.g == 123 && init4.b == 255 && init4.a == 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor copy")
  {
    ezColor8UNorm init4(100, 123, 255, 42);
    ezColor8UNorm copy(init4);
    EZ_TEST_BOOL(copy.r == 100 && copy.g == 123 && copy.b == 255 && copy.a == 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor ezColor")
  {
    ezColor8UNorm fromColor32f(ezColor::GetCornflowerBlue());
    EZ_TEST_BOOL(ezMath::IsEqual<ezUInt8>(fromColor32f.r, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().r * 255), 2) &&
      ezMath::IsEqual<ezUInt8>(fromColor32f.g, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().g * 255), 2) &&
      ezMath::IsEqual<ezUInt8>(fromColor32f.b, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().b * 255), 2) &&
      ezMath::IsEqual<ezUInt8>(fromColor32f.a, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().a * 255), 2));
  }

  // conversion
  {
    ezColor8UNorm cornflowerBlue(ezColor::GetCornflowerBlue());

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion ezColor")
    {
      ezColor color32f = static_cast<ezColor>(cornflowerBlue);
      EZ_TEST_BOOL(ezMath::IsEqual<float>(color32f.r, ezColor::GetCornflowerBlue().r, 2.0f / 255.0f) &&
        ezMath::IsEqual<float>(color32f.g, ezColor::GetCornflowerBlue().g, 2.0f / 255.0f) &&
        ezMath::IsEqual<float>(color32f.b, ezColor::GetCornflowerBlue().b, 2.0f / 255.0f) &&
        ezMath::IsEqual<float>(color32f.a, ezColor::GetCornflowerBlue().a, 2.0f / 255.0f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion ezUInt*")
    {
      const ezUInt8* pUIntsConst = static_cast<const ezUInt8*>(cornflowerBlue);
      EZ_TEST_BOOL(pUIntsConst[0] == cornflowerBlue.r && pUIntsConst[1] == cornflowerBlue.g && pUIntsConst[2] == cornflowerBlue.b && pUIntsConst[3] == cornflowerBlue.a);

      ezUInt8* pUInts = static_cast<ezUInt8*>(cornflowerBlue);
      EZ_TEST_BOOL(pUInts[0] == cornflowerBlue.r && pUInts[1] == cornflowerBlue.g && pUInts[2] == cornflowerBlue.b && pUInts[3] == cornflowerBlue.a);
    }
  }

  // comparison
  {
    ezColor8UNorm comp0(1,2,3);
    ezColor8UNorm comp1(1,2,3);
    ezColor8UNorm comp2(11,22,33);

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
    {
      EZ_TEST_BOOL(comp0.IsIdentical(comp1));
      EZ_TEST_BOOL(!comp0.IsIdentical(comp2));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator ==")
    {
      EZ_TEST_BOOL(comp0 == comp1);
      EZ_TEST_BOOL(!(comp0 == comp2));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator !=")
    {
      EZ_TEST_BOOL(comp0 != comp2);
      EZ_TEST_BOOL(!(comp0 != comp1));
    }
  }
}




EZ_CREATE_SIMPLE_TEST(Math, ColorBgra8UNorm)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor empty")
  {
    // Placement new of the default constructor should not have any effect on the previous data.
    ezUInt8 testBlock[4] = { 0, 64, 128, 255 };
    ezColorBgra8UNorm* pDefCtor = ::new ((void*) &testBlock[0]) ezColorBgra8UNorm;
    EZ_TEST_BOOL(pDefCtor->b == 0 &&
                 pDefCtor->g == 64 &&
                 pDefCtor->r == 128 &&
                 pDefCtor->a == 255);

    // Make sure the class didn't accidentally change in size
    EZ_TEST_BOOL(sizeof(ezColorBgra8UNorm) == sizeof(ezUInt8) * 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor components")
  {
    ezColorBgra8UNorm init3(100, 123, 255);
    EZ_TEST_BOOL(init3.b == 100 && init3.g == 123 && init3.r == 255 && init3.a == 255);

    ezColorBgra8UNorm init4(100, 123, 255, 42);
    EZ_TEST_BOOL(init4.b == 100 && init4.g == 123 && init4.r == 255 && init4.a == 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor copy")
  {
    ezColorBgra8UNorm init4(100, 123, 255, 42);
    ezColorBgra8UNorm copy(init4);
    EZ_TEST_BOOL(copy.b == 100 && copy.g == 123 && copy.r == 255 && copy.a == 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor ezColor")
  {
    ezColorBgra8UNorm fromColor32f(ezColor::GetCornflowerBlue());
    EZ_TEST_BOOL(ezMath::IsEqual<ezUInt8>(fromColor32f.r, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().r * 255), 2) &&
                 ezMath::IsEqual<ezUInt8>(fromColor32f.g, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().g * 255), 2) &&
                 ezMath::IsEqual<ezUInt8>(fromColor32f.b, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().b * 255), 2) &&
                 ezMath::IsEqual<ezUInt8>(fromColor32f.a, static_cast<ezUInt8>(ezColor::GetCornflowerBlue().a * 255), 2));
  }

  // conversion
  {
    ezColorBgra8UNorm cornflowerBlue(ezColor::GetCornflowerBlue());

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion ezColor")
    {
      ezColor color32f = static_cast<ezColor>(cornflowerBlue);
      EZ_TEST_BOOL(ezMath::IsEqual<float>(color32f.r, ezColor::GetCornflowerBlue().r, 2.0f / 255.0f) &&
                   ezMath::IsEqual<float>(color32f.g, ezColor::GetCornflowerBlue().g, 2.0f / 255.0f) &&
                   ezMath::IsEqual<float>(color32f.b, ezColor::GetCornflowerBlue().b, 2.0f / 255.0f) &&
                   ezMath::IsEqual<float>(color32f.a, ezColor::GetCornflowerBlue().a, 2.0f / 255.0f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Conversion ezUInt*")
    {
      const ezUInt8* pUIntsConst = static_cast<const ezUInt8*>(cornflowerBlue);
      EZ_TEST_BOOL(pUIntsConst[0] == cornflowerBlue.b && pUIntsConst[1] == cornflowerBlue.g && pUIntsConst[2] == cornflowerBlue.r && pUIntsConst[3] == cornflowerBlue.a);

      ezUInt8* pUInts = static_cast<ezUInt8*>(cornflowerBlue);
      EZ_TEST_BOOL(pUInts[0] == cornflowerBlue.b && pUInts[1] == cornflowerBlue.g && pUInts[2] == cornflowerBlue.r && pUInts[3] == cornflowerBlue.a);
    }
  }

  // comparison
  {
    ezColorBgra8UNorm comp0(1, 2, 3);
    ezColorBgra8UNorm comp1(1, 2, 3);
    ezColorBgra8UNorm comp2(11, 22, 33);

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
    {
      EZ_TEST_BOOL(comp0.IsIdentical(comp1));
      EZ_TEST_BOOL(!comp0.IsIdentical(comp2));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator ==")
    {
      EZ_TEST_BOOL(comp0 == comp1);
      EZ_TEST_BOOL(!(comp0 == comp2));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator !=")
    {
      EZ_TEST_BOOL(comp0 != comp2);
      EZ_TEST_BOOL(!(comp0 != comp1));
    }
  }
}

