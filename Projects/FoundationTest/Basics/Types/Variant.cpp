#include <PCH.h>
#include <Foundation/Basics/Types/Variant.h>

EZ_CREATE_SIMPLE_TEST(Basics, Variant)
{
  EZ_TEST_BLOCK(true, "Construction/IsA/Get")
  {
    {
      ezVariant b(true);
      EZ_TEST(b.GetType() == ezVariant::Type::Bool);
      EZ_TEST(b.IsA<bool>());
      EZ_TEST(b.Get<bool>() == true);
    }

    {
      ezVariant v(ezVec3(1, 2, 3));
      EZ_TEST(v.GetType() == ezVariant::Type::Vector3);
      EZ_TEST(v.IsA<ezVec3>());
      EZ_TEST(v.Get<ezVec3>() == ezVec3(1, 2, 3));
    }

    {
      ezVariantArray a;
      a.PushBack("This");
      a.PushBack("is a");
      a.PushBack("test");

      ezVariant va(a);
      EZ_TEST(va.GetType() == ezVariant::Type::VariantArray);
      EZ_TEST(va.IsA<ezVariantArray>());

      const ezArrayPtr<ezVariant>& b = va.Get<ezVariantArray>();

      EZ_TEST(a == b);
    }
  }
}

