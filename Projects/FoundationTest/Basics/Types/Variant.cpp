#include <PCH.h>
#include <Foundation/Basics/Types/Variant.h>

EZ_CREATE_SIMPLE_TEST(Basics, Variant)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invalid")
  {
    ezVariant b;
    EZ_TEST(b.GetType() == ezVariant::Type::Invalid);
    EZ_TEST(!b.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "bool")
  {
    ezVariant b(true);
    EZ_TEST(b.IsValid());
    EZ_TEST(b.GetType() == ezVariant::Type::Bool);
    EZ_TEST(b.IsA<bool>());
    EZ_TEST(b.Get<bool>() == true);

    EZ_TEST(b == ezVariant(true));
    EZ_TEST(b != ezVariant(false));

    EZ_TEST(b == true);
    EZ_TEST(b != false);

    b = false;
    EZ_TEST(b == false);

    b = ezVariant(true);
    EZ_TEST(b == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezInt32")
  {
    ezVariant b(23);
    EZ_TEST(b.IsValid());
    EZ_TEST(b.GetType() == ezVariant::Type::Int32);
    EZ_TEST(b.IsA<ezInt32>());
    EZ_TEST(b.Get<ezInt32>() == 23);

    EZ_TEST(b == ezVariant(23));
    EZ_TEST(b != ezVariant(11));

    EZ_TEST(b == 23);
    EZ_TEST(b != 24);

    b = 17;
    EZ_TEST(b == 17);

    b = ezVariant(19);
    EZ_TEST(b == 19);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezUInt32")
  {
    ezVariant b(23U);
    EZ_TEST(b.IsValid());
    EZ_TEST(b.GetType() == ezVariant::Type::UInt32);
    EZ_TEST(b.IsA<ezUInt32>());
    EZ_TEST(b.Get<ezUInt32>() == 23U);

    EZ_TEST(b == ezVariant(23));
    EZ_TEST(b != ezVariant(11));
    EZ_TEST(b == ezVariant(23U));
    EZ_TEST(b != ezVariant(11U));

    EZ_TEST(b == 23);
    EZ_TEST(b != 24);
    EZ_TEST(b == 23U);
    EZ_TEST(b != 24U);

    b = 17U;
    EZ_TEST(b == 17U);

    b = ezVariant(19U);
    EZ_TEST(b == 19U);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezInt64")
  {
    ezVariant b((ezInt64) 23);
    EZ_TEST(b.IsValid());
    EZ_TEST(b.GetType() == ezVariant::Type::Int64);
    EZ_TEST(b.IsA<ezInt64>());
    EZ_TEST(b.Get<ezInt64>() == 23);

    EZ_TEST(b == ezVariant(23));
    EZ_TEST(b != ezVariant(11));
    EZ_TEST(b == ezVariant((ezInt64) 23));
    EZ_TEST(b != ezVariant((ezInt64) 11));

    EZ_TEST(b == 23);
    EZ_TEST(b != 24);
    EZ_TEST(b == (ezInt64) 23);
    EZ_TEST(b != (ezInt64) 24);

    b = (ezInt64) 17;
    EZ_TEST(b == (ezInt64) 17);

    b = ezVariant((ezInt64) 19);
    EZ_TEST(b == (ezInt64) 19);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezUInt64")
  {
    ezVariant b((ezUInt64) 23);
    EZ_TEST(b.IsValid());
    EZ_TEST(b.GetType() == ezVariant::Type::UInt64);
    EZ_TEST(b.IsA<ezUInt64>());
    EZ_TEST(b.Get<ezUInt64>() == 23);

    EZ_TEST(b == ezVariant(23));
    EZ_TEST(b != ezVariant(11));
    EZ_TEST(b == ezVariant((ezUInt64) 23));
    EZ_TEST(b != ezVariant((ezUInt64) 11));

    EZ_TEST(b == 23);
    EZ_TEST(b != 24);
    EZ_TEST(b == (ezUInt64) 23);
    EZ_TEST(b != (ezUInt64) 24);

    b = (ezUInt64) 17;
    EZ_TEST(b == (ezUInt64) 17);

    b = ezVariant((ezUInt64) 19);
    EZ_TEST(b == (ezUInt64) 19);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "float")
  {
    ezVariant b(42.0f);
    EZ_TEST(b.IsValid());
    EZ_TEST(b.GetType() == ezVariant::Type::Float);
    EZ_TEST(b.IsA<float>());
    EZ_TEST(b.Get<float>() == 42.0f);

    EZ_TEST(b == ezVariant(42));
    EZ_TEST(b != ezVariant(11));
    EZ_TEST(b == ezVariant(42.0));
    EZ_TEST(b != ezVariant(11.0));
    EZ_TEST(b == ezVariant(42.0f));
    EZ_TEST(b != ezVariant(11.0f));

    EZ_TEST(b == 42);
    EZ_TEST(b != 41);
    EZ_TEST(b == 42.0);
    EZ_TEST(b != 41.0);
    EZ_TEST(b == 42.0f);
    EZ_TEST(b != 41.0f);

    b = 17.0f;
    EZ_TEST(b == 17.0f);

    b = ezVariant(19.0f);
    EZ_TEST(b == 19.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "double")
  {
    ezVariant b(42.0);
    EZ_TEST(b.IsValid());
    EZ_TEST(b.GetType() == ezVariant::Type::Double);
    EZ_TEST(b.IsA<double>());
    EZ_TEST(b.Get<double>() == 42.0);

    EZ_TEST(b == ezVariant(42));
    EZ_TEST(b != ezVariant(11));
    EZ_TEST(b == ezVariant(42.0));
    EZ_TEST(b != ezVariant(11.0));
    EZ_TEST(b == ezVariant(42.0f));
    EZ_TEST(b != ezVariant(11.0f));

    EZ_TEST(b == 42);
    EZ_TEST(b != 41);
    EZ_TEST(b == 42.0);
    EZ_TEST(b != 41.0);
    EZ_TEST(b == 42.0f);
    EZ_TEST(b != 41.0f);

    b = 17.0;
    EZ_TEST(b == 17.0);

    b = ezVariant(19.0);
    EZ_TEST(b == 19.0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVec2")
  {
    ezVariant v(ezVec2(1, 2));
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::Vector2);
    EZ_TEST(v.IsA<ezVec2>());
    EZ_TEST(v.Get<ezVec2>() == ezVec2(1, 2));

    EZ_TEST(v == ezVariant(ezVec2(1, 2)));
    EZ_TEST(v != ezVariant(ezVec2(1, 1)));

    EZ_TEST(v == ezVec2(1, 2));
    EZ_TEST(v != ezVec2(1, 4));

    v = ezVec2(5, 8);
    EZ_TEST(v == ezVec2(5, 8));

    v = ezVariant(ezVec2(7, 9));
    EZ_TEST(v == ezVec2(7, 9));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVec3")
  {
    ezVariant v(ezVec3(1, 2, 3));
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::Vector3);
    EZ_TEST(v.IsA<ezVec3>());
    EZ_TEST(v.Get<ezVec3>() == ezVec3(1, 2, 3));

    EZ_TEST(v == ezVariant(ezVec3(1, 2, 3)));
    EZ_TEST(v != ezVariant(ezVec3(1, 1, 3)));

    EZ_TEST(v == ezVec3(1, 2, 3));
    EZ_TEST(v != ezVec3(1, 4, 3));

    v = ezVec3(5, 8, 9);
    EZ_TEST(v == ezVec3(5, 8, 9));

    v = ezVariant(ezVec3(7, 9, 8));
    EZ_TEST(v == ezVec3(7, 9, 8));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVec4")
  {
    ezVariant v(ezVec4(1, 2, 3, 4));
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::Vector4);
    EZ_TEST(v.IsA<ezVec4>());
    EZ_TEST(v.Get<ezVec4>() == ezVec4(1, 2, 3, 4));

    EZ_TEST(v == ezVariant(ezVec4(1, 2, 3, 4)));
    EZ_TEST(v != ezVariant(ezVec4(1, 1, 3, 4)));

    EZ_TEST(v == ezVec4(1, 2, 3, 4));
    EZ_TEST(v != ezVec4(1, 4, 3, 4));

    v = ezVec4(5, 8, 9, 3);
    EZ_TEST(v == ezVec4(5, 8, 9, 3));

    v = ezVariant(ezVec4(7, 9, 8, 4));
    EZ_TEST(v == ezVec4(7, 9, 8, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezQuat")
  {
    ezVariant v(ezQuat(1, 2, 3, 4));
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::Quaternion);
    EZ_TEST(v.IsA<ezQuat>());
    EZ_TEST(v.Get<ezQuat>() == ezQuat(1, 2, 3, 4));

    EZ_TEST(v == ezQuat(1, 2, 3, 4));
    EZ_TEST(v != ezQuat(1, 2, 3, 5));

    EZ_TEST(v == ezQuat(1, 2, 3, 4));
    EZ_TEST(v != ezQuat(1, 4, 3, 4));

    v = ezQuat(5, 8, 9, 3);
    EZ_TEST(v == ezQuat(5, 8, 9, 3));

    v = ezVariant(ezQuat(7, 9, 8, 4));
    EZ_TEST(v == ezQuat(7, 9, 8, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezMat3")
  {
    ezVariant v(ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::Matrix3);
    EZ_TEST(v.IsA<ezMat3>());
    EZ_TEST(v.Get<ezMat3>() == ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));

    EZ_TEST(v == ezVariant(ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9)));
    EZ_TEST(v != ezVariant(ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 8)));

    EZ_TEST(v == ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    EZ_TEST(v != ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 8));

    v = ezMat3(5, 8, 9, 3, 1, 2, 3, 4, 5);
    EZ_TEST(v == ezMat3(5, 8, 9, 3, 1, 2, 3, 4, 5));

    v = ezVariant(ezMat3(5, 8, 9, 3, 1, 2, 3, 4, 4));
    EZ_TEST(v == ezMat3(5, 8, 9, 3, 1, 2, 3, 4, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezMat4")
  {
    ezVariant v(ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::Matrix4);
    EZ_TEST(v.IsA<ezMat4>());
    EZ_TEST(v.Get<ezMat4>() == ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    EZ_TEST(v == ezVariant(ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
    EZ_TEST(v != ezVariant(ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15)));

    EZ_TEST(v == ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    EZ_TEST(v != ezMat4(1, 2, 3, 4, 5, 6, 2, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    v = ezMat4(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8);
    EZ_TEST(v == ezMat4(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    v = ezVariant(ezMat4(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));
    EZ_TEST(v == ezMat4(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "const char*")
  {
    ezVariant v("This is a const char array");
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::String);
    EZ_TEST(v.IsA<const char*>());
    EZ_TEST(v.IsA<char*>());
    EZ_TEST(v.Get<ezString>() == ezString("This is a const char array"));

    EZ_TEST(v == ezVariant("This is a const char array"));
    EZ_TEST(v != ezVariant("This is something else"));

    EZ_TEST(v == ezString("This is a const char array"));
    EZ_TEST(v != ezString("This is another string"));

    v = "blurg!";
    EZ_TEST(v == ezString("blurg!"));

    v = ezVariant("blärg!");
    EZ_TEST(v == ezString("blärg!"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezString")
  {
    ezVariant v(ezString("This is an ezString"));
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::String);
    EZ_TEST(v.IsA<ezString>());
    EZ_TEST(v.Get<ezString>() == ezString("This is an ezString"));

    EZ_TEST(v == ezVariant(ezString("This is an ezString")));
    EZ_TEST(v != ezVariant(ezString("This is something else")));

    EZ_TEST(v == ezString("This is an ezString"));
    EZ_TEST(v != ezString("This is another ezString"));

    v = ezString("blurg!");
    EZ_TEST(v == ezString("blurg!"));

    v = ezVariant(ezString("blärg!"));
    EZ_TEST(v == ezString("blärg!"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTime")
  {
    ezVariant v(ezTime::Seconds(1337));
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::Time);
    EZ_TEST(v.IsA<ezTime>());
    EZ_TEST(v.Get<ezTime>() == ezTime::Seconds(1337));

    EZ_TEST(v == ezVariant(ezTime::Seconds(1337)));
    EZ_TEST(v != ezVariant(ezTime::Seconds(1336)));

    EZ_TEST(v == ezTime::Seconds(1337));
    EZ_TEST(v != ezTime::Seconds(1338));

    v = ezTime::Seconds(8472);
    EZ_TEST(v == ezTime::Seconds(8472));

    v = ezVariant(ezTime::Seconds(13));
    EZ_TEST(v == ezTime::Seconds(13));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVariantArray")
  {
    ezVariantArray a, a2;
    a.PushBack("This");
    a.PushBack("is a");
    a.PushBack("test");

    ezVariant va(a);
    EZ_TEST(va.IsValid());
    EZ_TEST(va.GetType() == ezVariant::Type::VariantArray);
    EZ_TEST(va.IsA<ezVariantArray>());

    const ezArrayPtr<ezVariant>& b = va.Get<ezVariantArray>();
    ezArrayPtr<ezVariant> b2 = va.Get<ezVariantArray>();

    EZ_TEST(a == b);
    EZ_TEST(a == b2);

    EZ_TEST(a != a2);

    EZ_TEST(va == a);
    EZ_TEST(va != a2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVariantDictionary")
  {
    ezVariantDictionary a, a2;
    a["my"] = true;
    a["luv"] = 4;
    a["pon"] = "ies";

    ezVariant va(a);
    EZ_TEST(va.IsValid());
    EZ_TEST(va.GetType() == ezVariant::Type::VariantDictionary);
    EZ_TEST(va.IsA<ezVariantDictionary>());

    const ezVariantDictionary& d1 = va.Get<ezVariantDictionary>();
    ezVariantDictionary d2 = va.Get<ezVariantDictionary>();

    EZ_TEST(a == d1);
    EZ_TEST(a == d2);
    EZ_TEST(d1 == d2);

    EZ_TEST(va == a);
    EZ_TEST(va != a2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "void*")
  {
    struct bla { bool pups; };
    bla blub, blub2;

    ezVariant v((void*) &blub);
    EZ_TEST(v.IsValid());
    EZ_TEST(v.GetType() == ezVariant::Type::VoidPointer);
    EZ_TEST(v.IsA<void*>());
    EZ_TEST(v.Get<void*>() == &blub);
    EZ_TEST(v.Get<void*>() != &blub2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (bool)")
  {
    ezVariant v(true);

    EZ_TEST(v.CanConvertTo<bool>());
    EZ_TEST(v.CanConvertTo<ezInt32>());

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezInt32)")
  {
    ezVariant v((ezInt32) 3);

    EZ_TEST(v.CanConvertTo<bool>());
    EZ_TEST(v.CanConvertTo<ezInt32>());

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezUInt32)")
  {
    ezVariant v((ezUInt32) 3);

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezInt64)")
  {
    ezVariant v((ezInt64) 3);

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezUInt64)")
  {
    ezVariant v((ezUInt64) 3);

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (float)")
  {
    ezVariant v((float) 3.0f);

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (double)")
  {
    ezVariant v((double) 3.0f);

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezVec2)")
  {
    ezVariant v(ezVec2(3.0f, 4.0f));

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezVec3)")
  {
    ezVariant v(ezVec3(3.0f, 4.0f, 6));

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezVec4)")
  {
    ezVariant v(ezVec4(3.0f, 4.0f, 3, 56));

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezQuat)")
  {
    ezVariant v(ezQuat(3.0f, 4.0f, 3, 56));

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezMat3)")
  {
    ezVariant v(ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezMat4)")
  {
    ezVariant v(ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6));

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezString)")
  {
    ezVariant v("ich hab keine Lust mehr");

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool)); /// \todo These conversions need to be thoroughly tested
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezTime)")
  {
    ezVariant v(ezTime::Seconds(0.0));

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false); /// \todo None of these ? (int / float)
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (VariantArray)")
  {
    ezVariantArray va;
    ezVariant v(va);

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (ezVariantDictionary)")
  {
    ezVariantDictionary va;
    ezVariant v(va);

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary));
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CanConvertTo (void*)")
  {
    ezVariant v((void*) 0);

    EZ_TEST(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Bool) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt32) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Int64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::UInt64) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Float) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Double) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::String) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST(v.CanConvertTo(ezVariant::Type::VoidPointer));
  }
}


