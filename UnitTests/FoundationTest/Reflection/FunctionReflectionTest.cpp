#include <PCH.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>
#include <Foundation/Reflection/ReflectionUtils.h>

struct FunctionTest
{
  int StandardTypeFunction(int v, const ezVec2 cv, ezVec3& rv, const ezVec4& crv, ezVec2U32* pv, const ezVec3U32* cpv)
  {
    EZ_TEST_BOOL(m_values[0] == v);
    EZ_TEST_BOOL(m_values[1] == cv);
    EZ_TEST_BOOL(m_values[2] == rv);
    EZ_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pv);
      EZ_TEST_BOOL(!cpv);
    }
    else
    {
      EZ_TEST_BOOL(m_values[4] == *pv);
      EZ_TEST_BOOL(m_values[5] == *cpv);
    }
    rv.Set(1, 2, 3);
    if (pv)
    {
      pv->Set(1, 2);
    }
    return 5;
  }

  const char* StringTypeFunction(const char* szString, ezString& sString, ezStringView sView)
  {
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!szString);
    }
    else
    {
      EZ_TEST_BOOL(m_values[0] == szString);
    }
    EZ_TEST_BOOL(m_values[1] == sString);
    EZ_TEST_BOOL(m_values[2] == sView);
    return "StringRet";
  }

  ezEnum<ezExampleEnum> EnumFunction(ezEnum<ezExampleEnum> e, ezEnum<ezExampleEnum>& re, const ezEnum<ezExampleEnum>& cre, ezEnum<ezExampleEnum>* pe, const ezEnum<ezExampleEnum>* cpe)
  {
    EZ_TEST_BOOL(m_values[0].Get<ezInt64>() == e);
    EZ_TEST_BOOL(m_values[1].Get<ezInt64>() == re);
    EZ_TEST_BOOL(m_values[2].Get<ezInt64>() == cre);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pe);
      EZ_TEST_BOOL(!cpe);
    }
    else
    {
      EZ_TEST_BOOL(m_values[3].Get<ezInt64>() == *pe);
      EZ_TEST_BOOL(m_values[4].Get<ezInt64>() == *cpe);
    }
    return ezExampleEnum::Value1;
  }

  ezBitflags<ezExampleBitflags> BitflagsFunction(ezBitflags<ezExampleBitflags> e, ezBitflags<ezExampleBitflags>& re, const ezBitflags<ezExampleBitflags>& cre, ezBitflags<ezExampleBitflags>* pe, const ezBitflags<ezExampleBitflags>* cpe)
  {
    EZ_TEST_BOOL(e == m_values[0].Get<ezInt64>());
    EZ_TEST_BOOL(re == m_values[1].Get<ezInt64>());
    EZ_TEST_BOOL(cre == m_values[2].Get<ezInt64>());
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pe);
      EZ_TEST_BOOL(!cpe);
    }
    else
    {
      EZ_TEST_BOOL(*pe == m_values[3].Get<ezInt64>());
      EZ_TEST_BOOL(*cpe == m_values[4].Get<ezInt64>());
    }
    return ezExampleBitflags::Value1 | ezExampleBitflags::Value2;
  }

  ezTestStruct3 StructFunction(ezTestStruct3 s, const ezTestStruct3 cs, ezTestStruct3& rs, const ezTestStruct3& crs, ezTestStruct3* ps, const ezTestStruct3* cps)
  {
    EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[0].Get<void*>()) == s);
    EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[1].Get<void*>()) == cs);
    EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[2].Get<void*>()) == rs);
    EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[3].Get<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!ps);
      EZ_TEST_BOOL(!cps);
    }
    else
    {
      EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[4].Get<void*>()) == *ps);
      EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[5].Get<void*>()) == *cps);
    }
    rs.m_fFloat1 = 999.0f;
    rs.m_UInt8 = 666;
    if (ps)
    {
      ps->m_fFloat1 = 666.0f;
      ps->m_UInt8 = 999;
    }
    ezTestStruct3 retS; retS.m_fFloat1 = 42; retS.m_UInt8 = 42;
    return retS;
  }

  ezTestClass1 ReflectedClassFunction(ezTestClass1 s, const ezTestClass1 cs, ezTestClass1& rs, const ezTestClass1& crs, ezTestClass1* ps, const ezTestClass1* cps)
  {
    EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[0].ConvertTo<void*>()) == s);
    EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[1].ConvertTo<void*>()) == cs);
    EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[2].ConvertTo<void*>()) == rs);
    EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[3].ConvertTo<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!ps);
      EZ_TEST_BOOL(!cps);
    }
    else
    {
      EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[4].ConvertTo<void*>()) == *ps);
      EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[5].ConvertTo<void*>()) == *cps);
    }
    rs.m_Color.SetRGB(1, 2, 3);
    rs.m_MyVector.Set(1, 2, 3);
    if (ps)
    {
      ps->m_Color.SetRGB(1, 2, 3);
      ps->m_MyVector.Set(1, 2, 3);
    }
    ezTestClass1 retS; retS.m_Color.SetRGB(42,42,42); retS.m_MyVector.Set(42, 42, 42);
    return retS;
  }

  ezVariant VariantFunction(ezVariant v, const ezVariant cv, ezVariant& rv, const ezVariant& crv, ezVariant* pv, const ezVariant* cpv)
  {
    EZ_TEST_BOOL(m_values[0] == v);
    EZ_TEST_BOOL(m_values[1] == cv);
    EZ_TEST_BOOL(m_values[2] == rv);
    EZ_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      // Can't have variant as nullptr as it must exist in the array and there is no further
      // way of distinguishing a between a ezVariant* and a ezVariant that is invalid.
      EZ_TEST_BOOL(!pv->IsValid());
      EZ_TEST_BOOL(!cpv->IsValid());
    }
    else
    {
      EZ_TEST_BOOL(m_values[4] == *pv);
      EZ_TEST_BOOL(m_values[5] == *cpv);
    }
    rv = ezVec3(1, 2, 3);
    if (pv)
    {
      *pv = ezVec2U32(1, 2);
    }
    return 5;
  }

  bool m_bPtrAreNull = false;
  ezDynamicArray<ezVariant> m_values;
};

typedef std::tuple< const ezRTTI*, ezBitflags<ezPropertyFlags> > ParamSig;

void VerifyFunctionSignature(const ezAbstractFunctionProperty* pFunc, ezArrayPtr<ParamSig> params, ParamSig ret)
{
  EZ_TEST_INT(params.GetCount(), pFunc->GetArgumentCount());
  for (ezUInt32 i = 0; i < ezMath::Min(params.GetCount(), pFunc->GetArgumentCount()); i++)
  {
    EZ_TEST_BOOL(pFunc->GetArgumentType(i) == std::get<0>(params[i]));
    EZ_TEST_BOOL(pFunc->GetArgumentFlags(i) == std::get<1>(params[i]));
  }
  EZ_TEST_BOOL(pFunc->GetReturnType() == std::get<0>(ret));
  EZ_TEST_BOOL(pFunc->GetReturnFlags() == std::get<1>(ret));
}

EZ_CREATE_SIMPLE_TEST(Reflection, Functions)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - StandardTypes")
  {
    ezFunctionProperty<decltype(&FunctionTest::StandardTypeFunction)> funccall("", &FunctionTest::StandardTypeFunction);
    ParamSig testSet[] = {
      { ezGetStaticRTTI<int>(), ezPropertyFlags::StandardType },
      { ezGetStaticRTTI<ezVec2>(), ezPropertyFlags::StandardType },
      { ezGetStaticRTTI<ezVec3>(), ezPropertyFlags::StandardType | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezVec4>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezVec2U32>(), ezPropertyFlags::StandardType | ezPropertyFlags::Pointer },
      { ezGetStaticRTTI<ezVec3U32>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Pointer },
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<int>(), ezPropertyFlags::StandardType));

    FunctionTest test;
    test.m_values.PushBack(1);
    test.m_values.PushBack(ezVec2(2));
    test.m_values.PushBack(ezVec3(3));
    test.m_values.PushBack(ezVec4(4));
    test.m_values.PushBack(ezVec2U32(5));
    test.m_values.PushBack(ezVec3U32(6));

    ezVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int32);
    EZ_TEST_BOOL(ret == 5);
    EZ_TEST_BOOL(test.m_values[2] == ezVec3(1, 2, 3));
    EZ_TEST_BOOL(test.m_values[4] == ezVec2U32(1, 2));

    test.m_bPtrAreNull = true;
    test.m_values[4] = ezVariant();
    test.m_values[5] = ezVariant();
    ret = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int32);
    EZ_TEST_BOOL(ret == 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - Strings")
  {
    ezFunctionProperty<decltype(&FunctionTest::StringTypeFunction)> funccall("", &FunctionTest::StringTypeFunction);
    ParamSig testSet[] = {
      { ezGetStaticRTTI<const char*>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const },
      { ezGetStaticRTTI<ezString>(), ezPropertyFlags::StandardType | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezStringView>(), ezPropertyFlags::StandardType },
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<const char*>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const));

    FunctionTest test;
    test.m_values.PushBack(ezString("String0"));
    test.m_values.PushBack(ezString("String1"));
    test.m_values.PushBack(ezStringView("String2"));

    ezVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::String);
    EZ_TEST_BOOL(ret == ezString("StringRet"));

    test.m_bPtrAreNull = true;
    test.m_values[0] = ezVariant();
    ret = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::String);
    EZ_TEST_BOOL(ret == ezString("StringRet"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - Enum")
  {
    ezFunctionProperty<decltype(&FunctionTest::EnumFunction)> funccall("", &FunctionTest::EnumFunction);
    ParamSig testSet[] = {
      { ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum },
      { ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum | ezPropertyFlags::Const | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum | ezPropertyFlags::Pointer },
      { ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum | ezPropertyFlags::Const | ezPropertyFlags::Pointer },
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum));

    FunctionTest test;
    test.m_values.PushBack((ezInt64)ezExampleEnum::Value1);
    test.m_values.PushBack((ezInt64)ezExampleEnum::Value2);
    test.m_values.PushBack((ezInt64)ezExampleEnum::Value3);
    test.m_values.PushBack((ezInt64)ezExampleEnum::Default);
    test.m_values.PushBack((ezInt64)ezExampleEnum::Value3);

    ezVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int64);
    EZ_TEST_BOOL(ret == (ezInt64)ezExampleEnum::Value1);

    test.m_bPtrAreNull = true;
    test.m_values[3] = ezVariant();
    test.m_values[4] = ezVariant();
    ret = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int64);
    EZ_TEST_BOOL(ret == (ezInt64)ezExampleEnum::Value1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - Bitflags")
  {
    ezFunctionProperty<decltype(&FunctionTest::BitflagsFunction)> funccall("", &FunctionTest::BitflagsFunction);
    ParamSig testSet[] = {
      { ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags },
      { ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags | ezPropertyFlags::Const | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags | ezPropertyFlags::Pointer },
      { ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags | ezPropertyFlags::Const | ezPropertyFlags::Pointer },
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags));

    FunctionTest test;
    test.m_values.PushBack((ezInt64)(0));
    test.m_values.PushBack((ezInt64)(ezExampleBitflags::Value2));
    test.m_values.PushBack((ezInt64)(ezExampleBitflags::Value3 | ezExampleBitflags::Value2).GetValue());
    test.m_values.PushBack((ezInt64)(ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3).GetValue());
    test.m_values.PushBack((ezInt64)(ezExampleBitflags::Value3));

    ezVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int64);
    EZ_TEST_BOOL(ret == (ezInt64)(ezExampleBitflags::Value1 | ezExampleBitflags::Value2).GetValue());

    test.m_bPtrAreNull = true;
    test.m_values[3] = ezVariant();
    test.m_values[4] = ezVariant();
    ret = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int64);
    EZ_TEST_BOOL(ret == (ezInt64)(ezExampleBitflags::Value1 | ezExampleBitflags::Value2).GetValue());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - Structs")
  {
    ezFunctionProperty<decltype(&FunctionTest::StructFunction)> funccall("", &FunctionTest::StructFunction);
    ParamSig testSet[] = {
      { ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class },
      { ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class },
      { ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer },
      { ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Pointer },
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class));

    FunctionTest test;
    ezTestStruct3 retS; retS.m_fFloat1 = 0; retS.m_UInt8 = 0;
    ezTestStruct3 value; value.m_fFloat1 = 0; value.m_UInt8 = 0;
    ezTestStruct3 rs; rs.m_fFloat1 = 42;
    ezTestStruct3 ps; ps.m_fFloat1 = 18;

    test.m_values.PushBack(&value);
    test.m_values.PushBack(&value);
    test.m_values.PushBack(&rs);
    test.m_values.PushBack(&value);
    test.m_values.PushBack(&ps);
    test.m_values.PushBack(&value);

    ezVariant ret = &retS;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_INT(retS.m_fFloat1, 42);
    EZ_TEST_FLOAT(retS.m_UInt8, 42, 0);

    EZ_TEST_INT(rs.m_fFloat1, 999);
    EZ_TEST_FLOAT(rs.m_UInt8, 666, 0);

    EZ_TEST_INT(ps.m_fFloat1, 666);
    EZ_TEST_FLOAT(ps.m_UInt8, 999, 0);

    test.m_bPtrAreNull = true;
    test.m_values[4] = ezVariant();
    test.m_values[5] = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - Reflected Classes")
  {
    ezFunctionProperty<decltype(&FunctionTest::ReflectedClassFunction)> funccall("", &FunctionTest::ReflectedClassFunction);
    ParamSig testSet[] = {
      { ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class },
      { ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class },
      { ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer },
      { ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Pointer },
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class));

    FunctionTest test;
    ezTestClass1 retS; retS.m_Color = ezColor::Chocolate;
    ezTestClass1 value; value.m_Color = ezColor::AliceBlue;
    ezTestClass1 rs; rs.m_Color = ezColor::Beige;
    ezTestClass1 ps; ps.m_Color = ezColor::DarkBlue;

    test.m_values.PushBack(&value);
    test.m_values.PushBack(&value);
    test.m_values.PushBack(&rs);
    test.m_values.PushBack(&value);
    test.m_values.PushBack(&ps);
    test.m_values.PushBack(&value);

    rs.m_Color.SetRGB(1, 2, 3);
    rs.m_MyVector.Set(1, 2, 3);


    ezVariant ret = &retS;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(retS.m_Color == ezColor(42, 42, 42));
    EZ_TEST_BOOL(retS.m_MyVector == ezVec3(42, 42, 42));

    EZ_TEST_BOOL(rs.m_Color == ezColor(1, 2, 3));
    EZ_TEST_BOOL(rs.m_MyVector == ezVec3(1, 2, 3));

    EZ_TEST_BOOL(ps.m_Color == ezColor(1, 2, 3));
    EZ_TEST_BOOL(ps.m_MyVector == ezVec3(1, 2, 3));

    test.m_bPtrAreNull = true;
    test.m_values[4] = ezVariant();
    test.m_values[5] = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - Variant")
  {
    ezFunctionProperty<decltype(&FunctionTest::VariantFunction)> funccall("", &FunctionTest::VariantFunction);
    ParamSig testSet[] = {
      { ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType },
      { ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType },
      { ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Reference },
      { ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType | ezPropertyFlags::Pointer },
      { ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Pointer },
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType));

    FunctionTest test;
    test.m_values.PushBack(1);
    test.m_values.PushBack(ezVec2(2));
    test.m_values.PushBack(ezVec3(3));
    test.m_values.PushBack(ezVec4(4));
    test.m_values.PushBack(ezVec2U32(5));
    test.m_values.PushBack(ezVec3U32(6));

    ezVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int32);
    EZ_TEST_BOOL(ret == 5);
    EZ_TEST_BOOL(test.m_values[2] == ezVec3(1, 2, 3));
    EZ_TEST_BOOL(test.m_values[4] == ezVec2U32(1, 2));

    test.m_bPtrAreNull = true;
    test.m_values[4] = ezVariant();
    test.m_values[5] = ezVariant();
    ret = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int32);
    EZ_TEST_BOOL(ret == 5);
  }
}
