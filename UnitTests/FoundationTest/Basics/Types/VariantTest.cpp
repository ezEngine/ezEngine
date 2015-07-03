#include <PCH.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Reflection/Reflection.h>

class Blubb : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(Blubb);
public:
  float u;
  float v;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Blubb, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("u", u),
    EZ_MEMBER_PROPERTY("v", v)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

template <typename T>
inline void TestIntegerVariant(ezVariant::Type::Enum type)
{
  ezVariant b((T) 23);
  EZ_TEST_BOOL(b.IsValid());
  EZ_TEST_BOOL(b.GetType() == type);
  EZ_TEST_BOOL(b.IsA<T>());
  EZ_TEST_BOOL(b.Get<T>() == 23);

  EZ_TEST_BOOL(b == ezVariant(23));
  EZ_TEST_BOOL(b != ezVariant(11));
  EZ_TEST_BOOL(b == ezVariant((T) 23));
  EZ_TEST_BOOL(b != ezVariant((T) 11));

  EZ_TEST_BOOL(b == 23);
  EZ_TEST_BOOL(b != 24);
  EZ_TEST_BOOL(b == (T) 23);
  EZ_TEST_BOOL(b != (T) 24);

  b = (T) 17;
  EZ_TEST_BOOL(b == (T) 17);

  b = ezVariant((T) 19);
  EZ_TEST_BOOL(b == (T) 19);
}

inline void TestNumberCanConvertTo(const ezVariant& v)
{
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Invalid) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Bool));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int8));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt8));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int16));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt16));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int32));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt32));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int64));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt64));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Float));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Double));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Color) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector2) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector3) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector4) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::String));
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Time) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Uuid) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
  EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);

  EZ_TEST_BOOL(v.ConvertTo<bool>() == true);
  EZ_TEST_BOOL(v.ConvertTo<ezInt8>() == 3);
  EZ_TEST_BOOL(v.ConvertTo<ezUInt8>() == 3);
  EZ_TEST_BOOL(v.ConvertTo<ezInt16>() == 3);
  EZ_TEST_BOOL(v.ConvertTo<ezUInt16>() == 3);
  EZ_TEST_BOOL(v.ConvertTo<ezInt32>() == 3);
  EZ_TEST_BOOL(v.ConvertTo<ezUInt32>() == 3);
  EZ_TEST_BOOL(v.ConvertTo<ezInt64>() == 3);
  EZ_TEST_BOOL(v.ConvertTo<ezUInt64>() == 3);
  EZ_TEST_BOOL(v.ConvertTo<float>() == 3.0f);
  EZ_TEST_BOOL(v.ConvertTo<double>() == 3.0);
  EZ_TEST_BOOL(v.ConvertTo<ezString>() == "3");

  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Bool).Get<bool>() == true);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int8).Get<ezInt8>() == 3);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt8).Get<ezUInt8>() == 3);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int16).Get<ezInt16>() == 3);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt16).Get<ezUInt16>() == 3);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int32).Get<ezInt32>() == 3);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt32).Get<ezUInt32>() == 3);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int64).Get<ezInt64>() == 3);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt64).Get<ezUInt64>() == 3);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Float).Get<float>() == 3.0f);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Double).Get<double>() == 3.0);
  EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "3");
}

inline void TestCanOnlyConvertToID(const ezVariant& v, ezVariant::Type::Enum type)
{
  for (int iType = 0; iType <= ezVariant::Type::VoidPointer; ++iType)
  {
    if (iType == type)
    {
      EZ_TEST_BOOL(v.CanConvertTo(type));
    }
    else
    {
      EZ_TEST_BOOL(v.CanConvertTo((ezVariant::Type::Enum)iType) == false);
    }
  }
}

inline void TestCanOnlyConvertToStringAndID(const ezVariant& v, ezVariant::Type::Enum type)
{
  for (int iType = 0; iType <= ezVariant::Type::VoidPointer; ++iType)
  {
    if (iType == ezVariant::Type::String)
    {
      EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::String));
    }
    else if (iType == type)
    {
      EZ_TEST_BOOL(v.CanConvertTo(type));
    }
    else
    {
      EZ_TEST_BOOL(v.CanConvertTo((ezVariant::Type::Enum)iType) == false);
    }
  }
}

EZ_CREATE_SIMPLE_TEST(Basics, Variant)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invalid")
  {
    ezVariant b;
    EZ_TEST_BOOL(b.GetType() == ezVariant::Type::Invalid);
    EZ_TEST_BOOL(b == ezVariant());
    EZ_TEST_BOOL(b != ezVariant(0));
    EZ_TEST_BOOL(!b.IsValid());
    EZ_TEST_BOOL(!b[0].IsValid());
    EZ_TEST_BOOL(!b["x"].IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "bool")
  {
    ezVariant b(true);
    EZ_TEST_BOOL(b.IsValid());
    EZ_TEST_BOOL(b.GetType() == ezVariant::Type::Bool);
    EZ_TEST_BOOL(b.IsA<bool>());
    EZ_TEST_BOOL(b.Get<bool>() == true);

    EZ_TEST_BOOL(b == ezVariant(true));
    EZ_TEST_BOOL(b != ezVariant(false));

    EZ_TEST_BOOL(b == true);
    EZ_TEST_BOOL(b != false);

    b = false;
    EZ_TEST_BOOL(b == false);

    b = ezVariant(true);
    EZ_TEST_BOOL(b == true);
    EZ_TEST_BOOL(!b[0].IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezInt8")
  {
    TestIntegerVariant<ezInt8>(ezVariant::Type::Int8);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezUInt8")
  {
    TestIntegerVariant<ezUInt8>(ezVariant::Type::UInt8);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezInt16")
  {
    TestIntegerVariant<ezInt16>(ezVariant::Type::Int16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezUInt16")
  {
    TestIntegerVariant<ezUInt16>(ezVariant::Type::UInt16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezInt32")
  {
    TestIntegerVariant<ezInt32>(ezVariant::Type::Int32);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezUInt32")
  {
    TestIntegerVariant<ezUInt32>(ezVariant::Type::UInt32);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezInt64")
  {
    TestIntegerVariant<ezInt64>(ezVariant::Type::Int64);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezUInt64")
  {
    TestIntegerVariant<ezUInt64>(ezVariant::Type::UInt64);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "float")
  {
    ezVariant b(42.0f);
    EZ_TEST_BOOL(b.IsValid());
    EZ_TEST_BOOL(b.GetType() == ezVariant::Type::Float);
    EZ_TEST_BOOL(b.IsA<float>());
    EZ_TEST_BOOL(b.Get<float>() == 42.0f);

    EZ_TEST_BOOL(b == ezVariant(42));
    EZ_TEST_BOOL(b != ezVariant(11));
    EZ_TEST_BOOL(b == ezVariant(42.0));
    EZ_TEST_BOOL(b != ezVariant(11.0));
    EZ_TEST_BOOL(b == ezVariant(42.0f));
    EZ_TEST_BOOL(b != ezVariant(11.0f));

    EZ_TEST_BOOL(b == 42);
    EZ_TEST_BOOL(b != 41);
    EZ_TEST_BOOL(b == 42.0);
    EZ_TEST_BOOL(b != 41.0);
    EZ_TEST_BOOL(b == 42.0f);
    EZ_TEST_BOOL(b != 41.0f);

    b = 17.0f;
    EZ_TEST_BOOL(b == 17.0f);

    b = ezVariant(19.0f);
    EZ_TEST_BOOL(b == 19.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "double")
  {
    ezVariant b(42.0);
    EZ_TEST_BOOL(b.IsValid());
    EZ_TEST_BOOL(b.GetType() == ezVariant::Type::Double);
    EZ_TEST_BOOL(b.IsA<double>());
    EZ_TEST_BOOL(b.Get<double>() == 42.0);

    EZ_TEST_BOOL(b == ezVariant(42));
    EZ_TEST_BOOL(b != ezVariant(11));
    EZ_TEST_BOOL(b == ezVariant(42.0));
    EZ_TEST_BOOL(b != ezVariant(11.0));
    EZ_TEST_BOOL(b == ezVariant(42.0f));
    EZ_TEST_BOOL(b != ezVariant(11.0f));

    EZ_TEST_BOOL(b == 42);
    EZ_TEST_BOOL(b != 41);
    EZ_TEST_BOOL(b == 42.0);
    EZ_TEST_BOOL(b != 41.0);
    EZ_TEST_BOOL(b == 42.0f);
    EZ_TEST_BOOL(b != 41.0f);

    b = 17.0;
    EZ_TEST_BOOL(b == 17.0);

    b = ezVariant(19.0);
    EZ_TEST_BOOL(b == 19.0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezColor")
  {
    ezVariant v(ezColor(1, 2, 3, 1));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Color);
    EZ_TEST_BOOL(v.IsA<ezColor>());
    EZ_TEST_BOOL(v.Get<ezColor>() == ezColor(1, 2, 3, 1));

    EZ_TEST_BOOL(v == ezVariant(ezColor(1, 2, 3)));
    EZ_TEST_BOOL(v != ezVariant(ezColor(1, 1, 1)));

    EZ_TEST_BOOL(v == ezColor(1, 2, 3));
    EZ_TEST_BOOL(v != ezColor(1, 4, 3));

    v = ezColor(5, 8, 9);
    EZ_TEST_BOOL(v == ezColor(5, 8, 9));

    v = ezVariant(ezColor(7, 9, 4));
    EZ_TEST_BOOL(v == ezColor(7, 9, 4));
    EZ_TEST_BOOL(v[0] == 7);
    EZ_TEST_BOOL(v[1] == 9);
    EZ_TEST_BOOL(v[2] == 4);
    EZ_TEST_BOOL(v[3] == 1);
    EZ_TEST_BOOL(v[4] == ezVariant());
    EZ_TEST_BOOL(!v[4].IsValid());
    EZ_TEST_BOOL(v["r"] == 7);
    EZ_TEST_BOOL(v["g"] == 9);
    EZ_TEST_BOOL(v["b"] == 4);
    EZ_TEST_BOOL(v["a"] == 1);
    EZ_TEST_BOOL(v["x"] == ezVariant());
    EZ_TEST_BOOL(!v["x"].IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVec2")
  {
    ezVariant v(ezVec2(1, 2));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Vector2);
    EZ_TEST_BOOL(v.IsA<ezVec2>());
    EZ_TEST_BOOL(v.Get<ezVec2>() == ezVec2(1, 2));

    EZ_TEST_BOOL(v == ezVariant(ezVec2(1, 2)));
    EZ_TEST_BOOL(v != ezVariant(ezVec2(1, 1)));

    EZ_TEST_BOOL(v == ezVec2(1, 2));
    EZ_TEST_BOOL(v != ezVec2(1, 4));

    v = ezVec2(5, 8);
    EZ_TEST_BOOL(v == ezVec2(5, 8));

    v = ezVariant(ezVec2(7, 9));
    EZ_TEST_BOOL(v == ezVec2(7, 9));
    EZ_TEST_BOOL(v[0] == 7);
    EZ_TEST_BOOL(v[1] == 9);
    EZ_TEST_BOOL(v["x"] == 7);
    EZ_TEST_BOOL(v["y"] == 9);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVec3")
  {
    ezVariant v(ezVec3(1, 2, 3));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Vector3);
    EZ_TEST_BOOL(v.IsA<ezVec3>());
    EZ_TEST_BOOL(v.Get<ezVec3>() == ezVec3(1, 2, 3));

    EZ_TEST_BOOL(v == ezVariant(ezVec3(1, 2, 3)));
    EZ_TEST_BOOL(v != ezVariant(ezVec3(1, 1, 3)));

    EZ_TEST_BOOL(v == ezVec3(1, 2, 3));
    EZ_TEST_BOOL(v != ezVec3(1, 4, 3));

    v = ezVec3(5, 8, 9);
    EZ_TEST_BOOL(v == ezVec3(5, 8, 9));

    v = ezVariant(ezVec3(7, 9, 8));
    EZ_TEST_BOOL(v == ezVec3(7, 9, 8));
    EZ_TEST_BOOL(v[0] == 7);
    EZ_TEST_BOOL(v[1] == 9);
    EZ_TEST_BOOL(v[2] == 8);
    EZ_TEST_BOOL(v["x"] == 7);
    EZ_TEST_BOOL(v["y"] == 9);
    EZ_TEST_BOOL(v["z"] == 8);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVec4")
  {
    ezVariant v(ezVec4(1, 2, 3, 4));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Vector4);
    EZ_TEST_BOOL(v.IsA<ezVec4>());
    EZ_TEST_BOOL(v.Get<ezVec4>() == ezVec4(1, 2, 3, 4));

    EZ_TEST_BOOL(v == ezVariant(ezVec4(1, 2, 3, 4)));
    EZ_TEST_BOOL(v != ezVariant(ezVec4(1, 1, 3, 4)));

    EZ_TEST_BOOL(v == ezVec4(1, 2, 3, 4));
    EZ_TEST_BOOL(v != ezVec4(1, 4, 3, 4));

    v = ezVec4(5, 8, 9, 3);
    EZ_TEST_BOOL(v == ezVec4(5, 8, 9, 3));

    v = ezVariant(ezVec4(7, 9, 8, 4));
    EZ_TEST_BOOL(v == ezVec4(7, 9, 8, 4));
    EZ_TEST_BOOL(v[0] == 7);
    EZ_TEST_BOOL(v[1] == 9);
    EZ_TEST_BOOL(v[2] == 8);
    EZ_TEST_BOOL(v[3] == 4);
    EZ_TEST_BOOL(v["x"] == 7);
    EZ_TEST_BOOL(v["y"] == 9);
    EZ_TEST_BOOL(v["z"] == 8);
    EZ_TEST_BOOL(v["w"] == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezQuat")
  {
    ezVariant v(ezQuat(1, 2, 3, 4));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Quaternion);
    EZ_TEST_BOOL(v.IsA<ezQuat>());
    EZ_TEST_BOOL(v.Get<ezQuat>() == ezQuat(1, 2, 3, 4));

    EZ_TEST_BOOL(v == ezQuat(1, 2, 3, 4));
    EZ_TEST_BOOL(v != ezQuat(1, 2, 3, 5));

    EZ_TEST_BOOL(v == ezQuat(1, 2, 3, 4));
    EZ_TEST_BOOL(v != ezQuat(1, 4, 3, 4));

    v = ezQuat(5, 8, 9, 3);
    EZ_TEST_BOOL(v == ezQuat(5, 8, 9, 3));

    v = ezVariant(ezQuat(7, 9, 8, 4));
    EZ_TEST_BOOL(v == ezQuat(7, 9, 8, 4));
    EZ_TEST_BOOL(v[0][0] == 7);
    EZ_TEST_BOOL(v[0][1] == 9);
    EZ_TEST_BOOL(v[0][2] == 8);
    EZ_TEST_BOOL(v[1] == 4);
    EZ_TEST_BOOL(v["v"]["x"] == 7);
    EZ_TEST_BOOL(v["v"]["y"] == 9);
    EZ_TEST_BOOL(v["v"]["z"] == 8);
    EZ_TEST_BOOL(v["w"] == 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezMat3")
  {
    ezVariant v(ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Matrix3);
    EZ_TEST_BOOL(v.IsA<ezMat3>());
    EZ_TEST_BOOL(v.Get<ezMat3>() == ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));

    EZ_TEST_BOOL(v == ezVariant(ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9)));
    EZ_TEST_BOOL(v != ezVariant(ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 8)));

    EZ_TEST_BOOL(v == ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    EZ_TEST_BOOL(v != ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 8));

    v = ezMat3(5, 8, 9, 3, 1, 2, 3, 4, 5);
    EZ_TEST_BOOL(v == ezMat3(5, 8, 9, 3, 1, 2, 3, 4, 5));

    v = ezVariant(ezMat3(5, 8, 9, 3, 1, 2, 3, 4, 4));
    EZ_TEST_BOOL(v == ezMat3(5, 8, 9, 3, 1, 2, 3, 4, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezMat4")
  {
    ezVariant v(ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Matrix4);
    EZ_TEST_BOOL(v.IsA<ezMat4>());
    EZ_TEST_BOOL(v.Get<ezMat4>() == ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    EZ_TEST_BOOL(v == ezVariant(ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
    EZ_TEST_BOOL(v != ezVariant(ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15)));

    EZ_TEST_BOOL(v == ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    EZ_TEST_BOOL(v != ezMat4(1, 2, 3, 4, 5, 6, 2, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    v = ezMat4(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8);
    EZ_TEST_BOOL(v == ezMat4(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    v = ezVariant(ezMat4(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));
    EZ_TEST_BOOL(v == ezMat4(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "const char*")
  {
    ezVariant v("This is a const char array");
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::String);
    EZ_TEST_BOOL(v.IsA<const char*>());
    EZ_TEST_BOOL(v.IsA<char*>());
    EZ_TEST_BOOL(v.Get<ezString>() == ezString("This is a const char array"));

    EZ_TEST_BOOL(v == ezVariant("This is a const char array"));
    EZ_TEST_BOOL(v != ezVariant("This is something else"));

    EZ_TEST_BOOL(v == ezString("This is a const char array"));
    EZ_TEST_BOOL(v != ezString("This is another string"));

    v = "blurg!";
    EZ_TEST_BOOL(v == ezString("blurg!"));

    v = ezVariant("blärg!");
    EZ_TEST_BOOL(v == ezString("blärg!"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezString")
  {
    ezVariant v(ezString("This is an ezString"));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::String);
    EZ_TEST_BOOL(v.IsA<ezString>());
    EZ_TEST_BOOL(v.Get<ezString>() == ezString("This is an ezString"));

    EZ_TEST_BOOL(v == ezVariant(ezString("This is an ezString")));
    EZ_TEST_BOOL(v != ezVariant(ezString("This is something else")));

    EZ_TEST_BOOL(v == ezString("This is an ezString"));
    EZ_TEST_BOOL(v != ezString("This is another ezString"));

    v = ezString("blurg!");
    EZ_TEST_BOOL(v == ezString("blurg!"));

    v = ezVariant(ezString("blärg!"));
    EZ_TEST_BOOL(v == ezString("blärg!"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTime")
  {
    ezVariant v(ezTime::Seconds(1337));
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Time);
    EZ_TEST_BOOL(v.IsA<ezTime>());
    EZ_TEST_BOOL(v.Get<ezTime>() == ezTime::Seconds(1337));

    EZ_TEST_BOOL(v == ezVariant(ezTime::Seconds(1337)));
    EZ_TEST_BOOL(v != ezVariant(ezTime::Seconds(1336)));

    EZ_TEST_BOOL(v == ezTime::Seconds(1337));
    EZ_TEST_BOOL(v != ezTime::Seconds(1338));

    v = ezTime::Seconds(8472);
    EZ_TEST_BOOL(v == ezTime::Seconds(8472));

    v = ezVariant(ezTime::Seconds(13));
    EZ_TEST_BOOL(v == ezTime::Seconds(13));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezUuid")
  {
    ezUuid id;
    ezVariant v(id);
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::Uuid);
    EZ_TEST_BOOL(v.IsA<ezUuid>());
    EZ_TEST_BOOL(v.Get<ezUuid>() == ezUuid());

    ezUuid uuid;
    uuid.CreateNewUuid();
    EZ_TEST_BOOL(v != ezVariant(uuid));
    EZ_TEST_BOOL(ezVariant(uuid).Get<ezUuid>() == uuid);

    ezUuid uuid2;
    uuid2.CreateNewUuid();
    EZ_TEST_BOOL(ezVariant(uuid) != ezVariant(uuid2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVariantArray")
  {
    ezVariantArray a, a2;
    a.PushBack("This");
    a.PushBack("is a");
    a.PushBack("test");

    ezVariant va(a);
    EZ_TEST_BOOL(va.IsValid());
    EZ_TEST_BOOL(va.GetType() == ezVariant::Type::VariantArray);
    EZ_TEST_BOOL(va.IsA<ezVariantArray>());

    const ezArrayPtr<const ezVariant>& b = va.Get<ezVariantArray>();
    ezArrayPtr<const ezVariant> b2 = va.Get<ezVariantArray>();

    EZ_TEST_BOOL(a == b);
    EZ_TEST_BOOL(a == b2);

    EZ_TEST_BOOL(a != a2);

    EZ_TEST_BOOL(va == a);
    EZ_TEST_BOOL(va != a2);

    EZ_TEST_BOOL(va[0] == ezString("This"));
    EZ_TEST_BOOL(va[1] == ezString("is a"));
    EZ_TEST_BOOL(va[2] == ezString("test"));
    EZ_TEST_BOOL(va[4] == ezVariant());
    EZ_TEST_BOOL(!va[4].IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezVariantDictionary")
  {
    ezVariantDictionary a, a2;
    a["my"] = true;
    a["luv"] = 4;
    a["pon"] = "ies";

    ezVariant va(a);
    EZ_TEST_BOOL(va.IsValid());
    EZ_TEST_BOOL(va.GetType() == ezVariant::Type::VariantDictionary);
    EZ_TEST_BOOL(va.IsA<ezVariantDictionary>());

    const ezVariantDictionary& d1 = va.Get<ezVariantDictionary>();
    ezVariantDictionary d2 = va.Get<ezVariantDictionary>();

    EZ_TEST_BOOL(a == d1);
    EZ_TEST_BOOL(a == d2);
    EZ_TEST_BOOL(d1 == d2);

    EZ_TEST_BOOL(va == a);
    EZ_TEST_BOOL(va != a2);

    EZ_TEST_BOOL(va["my"] == true);
    EZ_TEST_BOOL(va["luv"] == 4);
    EZ_TEST_BOOL(va["pon"] == ezString("ies"));
    EZ_TEST_BOOL(va["x"] == ezVariant());
    EZ_TEST_BOOL(!va["x"].IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezReflectedClass*")
  {
    Blubb blubb;
    blubb.u = 1.0f;
    blubb.v = 2.0f;

    Blubb blubb2;

    ezVariant v(&blubb);
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::ReflectedPointer);
    EZ_TEST_BOOL(v.IsA<ezReflectedClass*>());
    EZ_TEST_BOOL(v.Get<ezReflectedClass*>() == &blubb);
    EZ_TEST_BOOL(v.Get<ezReflectedClass*>() != &blubb2);

    EZ_TEST_BOOL(v[0] == 1.0f);
    EZ_TEST_BOOL(v[1] == 2.0f);
    EZ_TEST_BOOL(v["u"] == 1.0f);
    EZ_TEST_BOOL(v["v"] == 2.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "void*")
  {
    struct bla { bool pups; };
    bla blub, blub2;

    ezVariant v(&blub);
    EZ_TEST_BOOL(v.IsValid());
    EZ_TEST_BOOL(v.GetType() == ezVariant::Type::VoidPointer);
    EZ_TEST_BOOL(v.IsA<void*>());
    EZ_TEST_BOOL(v.Get<void*>() == &blub);
    EZ_TEST_BOOL(v.Get<void*>() != &blub2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (bool)")
  {
    ezVariant v(true);

    EZ_TEST_BOOL(v.CanConvertTo<bool>());
    EZ_TEST_BOOL(v.CanConvertTo<ezInt32>());

    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int8));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt8));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int16));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt16));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Color) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);

    EZ_TEST_BOOL(v.ConvertTo<bool>() == true);
    EZ_TEST_BOOL(v.ConvertTo<ezInt8>() == 1);
    EZ_TEST_BOOL(v.ConvertTo<ezUInt8>() == 1);
    EZ_TEST_BOOL(v.ConvertTo<ezInt16>() == 1);
    EZ_TEST_BOOL(v.ConvertTo<ezUInt16>() == 1);
    EZ_TEST_BOOL(v.ConvertTo<ezInt32>() == 1);
    EZ_TEST_BOOL(v.ConvertTo<ezUInt32>() == 1);
    EZ_TEST_BOOL(v.ConvertTo<ezInt64>() == 1);
    EZ_TEST_BOOL(v.ConvertTo<ezUInt64>() == 1);
    EZ_TEST_BOOL(v.ConvertTo<float>() == 1.0f);
    EZ_TEST_BOOL(v.ConvertTo<double>() == 1.0);
    EZ_TEST_BOOL(v.ConvertTo<ezString>() == "true");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Bool).Get<bool>() == true);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int8).Get<ezInt8>() == 1);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt8).Get<ezUInt8>() == 1);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int16).Get<ezInt16>() == 1);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt16).Get<ezUInt16>() == 1);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int32).Get<ezInt32>() == 1);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt32).Get<ezUInt32>() == 1);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int64).Get<ezInt64>() == 1);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt64).Get<ezUInt64>() == 1);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Float).Get<float>() == 1.0f);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Double).Get<double>() == 1.0);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "true");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezInt8)")
  {
    ezVariant v((ezInt8) 3);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezUInt8)")
  {
    ezVariant v((ezUInt8) 3);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezInt16)")
  {
    ezVariant v((ezInt16) 3);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezUInt16)")
  {
    ezVariant v((ezUInt16) 3);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezInt32)")
  {
    ezVariant v((ezInt32) 3);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezUInt32)")
  {
    ezVariant v((ezUInt32) 3);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezInt64)")
  {
    ezVariant v((ezInt64) 3);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezUInt64)")
  {
    ezVariant v((ezUInt64) 3);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (float)")
  {
    ezVariant v((float) 3.0f);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (double)")
  {
    ezVariant v((double) 3.0f);
    TestNumberCanConvertTo(v);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (Color)")
  {
    ezColor c(3, 3, 4, 0);
    ezVariant v(c);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Color);

    EZ_TEST_BOOL(v.ConvertTo<ezColor>() == c);
    EZ_TEST_BOOL(v.ConvertTo<ezString>() == "{ r=3, g=3, b=4, a=0 }");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Color).Get<ezColor>() == c);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "{ r=3, g=3, b=4, a=0 }");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezVec2)")
  {
    ezVec2 vec(3.0f, 4.0f);
    ezVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Vector2);

    EZ_TEST_BOOL(v.ConvertTo<ezVec2>() == vec);
    EZ_TEST_BOOL(v.ConvertTo<ezString>() == "{ x=3, y=4 }");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Vector2).Get<ezVec2>() == vec);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "{ x=3, y=4 }");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezVec3)")
  {
    ezVec3 vec(3.0f, 4.0f, 6.0f);
    ezVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Vector3);

    EZ_TEST_BOOL(v.ConvertTo<ezVec3>() == vec);
    EZ_TEST_BOOL(v.ConvertTo<ezString>() == "{ x=3, y=4, z=6 }");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Vector3).Get<ezVec3>() == vec);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "{ x=3, y=4, z=6 }");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezVec4)")
  {
    ezVec4 vec(3.0f, 4.0f, 3, 56);
    ezVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Vector4);

    EZ_TEST_BOOL(v.ConvertTo<ezVec4>() == vec);
    EZ_TEST_BOOL(v.ConvertTo<ezString>() == "{ x=3, y=4, z=3, w=56 }");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Vector4).Get<ezVec4>() == vec);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "{ x=3, y=4, z=3, w=56 }");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezQuat)")
  {
    ezQuat q(3.0f, 4.0f, 3, 56);
    ezVariant v(q);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Quaternion);

    EZ_TEST_BOOL(v.ConvertTo<ezQuat>() == q);
    EZ_TEST_BOOL(v.ConvertTo<ezString>() == "{ x=3, y=4, z=3, w=56 }");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Quaternion).Get<ezQuat>() == q);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "{ x=3, y=4, z=3, w=56 }");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezMat3)")
  {
    ezMat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    ezVariant v(m);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Matrix3);

    EZ_TEST_BOOL(v.ConvertTo<ezMat3>() == m);
    EZ_TEST_BOOL(v.ConvertTo<ezString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Matrix3).Get<ezMat3>() == m);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezMat4)")
  {
    ezMat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6);
    ezVariant v(m);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Matrix4);

    EZ_TEST_BOOL(v.ConvertTo<ezMat4>() == m);
    EZ_TEST_BOOL(v.ConvertTo<ezString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Matrix4).Get<ezMat4>() == m);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, c1r2=5, c2r2=6, c3r2=7, c4r2=8, c1r3=9, c2r3=0, c3r3=1, c4r3=2, c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezString)")
  {
    ezVariant v("ich hab keine Lust mehr");

    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Invalid) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Bool));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int8));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt8));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int16));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt16));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int32));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt32));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Int64));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::UInt64));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Float));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Double));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Color) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector2) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector3) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Vector4) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Quaternion) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Matrix3) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Matrix4) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::String));
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::Time) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VariantArray) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VariantDictionary) == false);
    EZ_TEST_BOOL(v.CanConvertTo(ezVariant::Type::VoidPointer) == false);

    {
      ezResult ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == false);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);
      
      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<ezInt8>(&ConversionStatus) == 0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);

      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<ezUInt8>(&ConversionStatus) == 0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);
      
      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<ezInt16>(&ConversionStatus) == 0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);

      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<ezUInt16>(&ConversionStatus) == 0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);

      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<ezInt32>(&ConversionStatus) == 0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);

      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<ezUInt32>(&ConversionStatus) == 0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);

      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<ezInt64>(&ConversionStatus) == 0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);

      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<ezUInt64>(&ConversionStatus) == 0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);

      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 0.0f);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);

      ConversionStatus = EZ_SUCCESS;
      EZ_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 0.0);
      EZ_TEST_BOOL(ConversionStatus == EZ_FAILURE);
    }

    {
      v = "true";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == true);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Bool, &ConversionStatus).Get<bool>() == true);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "-128";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<ezInt8>(&ConversionStatus) == -128);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int8, &ConversionStatus).Get<ezInt8>() == -128);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "255";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<ezUInt8>(&ConversionStatus) == 255);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt8, &ConversionStatus).Get<ezUInt8>() == 255);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "-5643";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<ezInt16>(&ConversionStatus) == -5643);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int16, &ConversionStatus).Get<ezInt16>() == -5643);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "9001";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<ezUInt16>(&ConversionStatus) == 9001);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt16, &ConversionStatus).Get<ezUInt16>() == 9001);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "46";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<ezInt32>(&ConversionStatus) == 46);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int32, &ConversionStatus).Get<ezInt32>() == 46);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "356";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<ezUInt32>(&ConversionStatus) == 356);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt32, &ConversionStatus).Get<ezUInt32>() == 356);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "64";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<ezInt64>(&ConversionStatus) == 64);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Int64, &ConversionStatus).Get<ezInt64>() == 64);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "6464";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<ezUInt64>(&ConversionStatus) == 6464);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::UInt64, &ConversionStatus).Get<ezUInt64>() == 6464);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "0.07564f";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 0.07564f);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Float, &ConversionStatus).Get<float>() == 0.07564f);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }

    {
      v = "0.4453";
      ezResult ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 0.4453);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);

      ConversionStatus = EZ_FAILURE;
      EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Double, &ConversionStatus).Get<double>() == 0.4453);
      EZ_TEST_BOOL(ConversionStatus == EZ_SUCCESS);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezTime)")
  {
    ezTime t = ezTime::Seconds(123.0);
    ezVariant v(t);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Time);

    EZ_TEST_BOOL(v.ConvertTo<ezTime>() == t);
    //EZ_TEST_BOOL(v.ConvertTo<ezString>() == "");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Time).Get<ezTime>() == t);
    //EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezUuid)")
  {
    ezUuid uuid;
    uuid.CreateNewUuid();
    ezVariant v(uuid);

    TestCanOnlyConvertToStringAndID(v, ezVariant::Type::Uuid);

    EZ_TEST_BOOL(v.ConvertTo<ezUuid>() == uuid);
    //EZ_TEST_BOOL(v.ConvertTo<ezString>() == "");

    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::Uuid).Get<ezUuid>() == uuid);
    //EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::String).Get<ezString>() == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (VariantArray)")
  {
    ezVariantArray va;
    ezVariant v(va);

    TestCanOnlyConvertToID(v, ezVariant::Type::VariantArray);

    EZ_TEST_BOOL(v.ConvertTo<ezVariantArray>() == va);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::VariantArray).Get<ezVariantArray>() == va);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (ezVariantDictionary)")
  {
    ezVariantDictionary va;
    ezVariant v(va);

    TestCanOnlyConvertToID(v, ezVariant::Type::VariantDictionary);

    EZ_TEST_BOOL(v.ConvertTo<ezVariantDictionary>() == va);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::VariantDictionary).Get<ezVariantDictionary>() == va);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "(Can)ConvertTo (void*)")
  {
    ezVariant v((void*) 0);

    TestCanOnlyConvertToID(v, ezVariant::Type::VoidPointer);

    EZ_TEST_BOOL(v.ConvertTo<void*>() == nullptr);
    EZ_TEST_BOOL(v.ConvertTo(ezVariant::Type::VoidPointer).Get<void*>() == nullptr);
  }
}
