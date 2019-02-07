#include <FoundationTestPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

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

  ezEnum<ezExampleEnum> EnumFunction(ezEnum<ezExampleEnum> e, ezEnum<ezExampleEnum>& re, const ezEnum<ezExampleEnum>& cre,
                                     ezEnum<ezExampleEnum>* pe, const ezEnum<ezExampleEnum>* cpe)
  {
    EZ_TEST_BOOL(m_values[0].Get<ezInt64>() == e.GetValue());
    EZ_TEST_BOOL(m_values[1].Get<ezInt64>() == re.GetValue());
    EZ_TEST_BOOL(m_values[2].Get<ezInt64>() == cre.GetValue());
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pe);
      EZ_TEST_BOOL(!cpe);
    }
    else
    {
      EZ_TEST_BOOL(m_values[3].Get<ezInt64>() == pe->GetValue());
      EZ_TEST_BOOL(m_values[4].Get<ezInt64>() == cpe->GetValue());
    }
    return ezExampleEnum::Value1;
  }

  ezBitflags<ezExampleBitflags> BitflagsFunction(ezBitflags<ezExampleBitflags> e, ezBitflags<ezExampleBitflags>& re,
                                                 const ezBitflags<ezExampleBitflags>& cre, ezBitflags<ezExampleBitflags>* pe,
                                                 const ezBitflags<ezExampleBitflags>* cpe)
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

  ezTestStruct3 StructFunction(ezTestStruct3 s, const ezTestStruct3 cs, ezTestStruct3& rs, const ezTestStruct3& crs, ezTestStruct3* ps,
                               const ezTestStruct3* cps)
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
    ezTestStruct3 retS;
    retS.m_fFloat1 = 42;
    retS.m_UInt8 = 42;
    return retS;
  }

  ezTestClass1 ReflectedClassFunction(ezTestClass1 s, const ezTestClass1 cs, ezTestClass1& rs, const ezTestClass1& crs, ezTestClass1* ps,
                                      const ezTestClass1* cps)
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
    ezTestClass1 retS;
    retS.m_Color.SetRGB(42, 42, 42);
    retS.m_MyVector.Set(42, 42, 42);
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

  static void StaticFunction(bool b, ezVariant v)
  {
    EZ_TEST_BOOL(b == true);
    EZ_TEST_BOOL(v == 4.0f);
  }

  static int StaticFunction2() { return 42; }

  bool m_bPtrAreNull = false;
  ezDynamicArray<ezVariant> m_values;
};

typedef std::tuple<const ezRTTI*, ezBitflags<ezPropertyFlags>> ParamSig;

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
        ParamSig(ezGetStaticRTTI<int>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<ezVec2>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<ezVec3>(), ezPropertyFlags::StandardType | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezVec4>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezVec2U32>(), ezPropertyFlags::StandardType | ezPropertyFlags::Pointer),
        ParamSig(ezGetStaticRTTI<ezVec3U32>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<int>(), ezPropertyFlags::StandardType));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

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
        ParamSig(ezGetStaticRTTI<const char*>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const),
        ParamSig(ezGetStaticRTTI<ezString>(), ezPropertyFlags::StandardType | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezStringView>(), ezPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet),
                            ParamSig(ezGetStaticRTTI<const char*>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(ezVariant(ezString("String0")));
    test.m_values.PushBack(ezVariant(ezString("String1")));
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
        ParamSig(ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum),
        ParamSig(ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum | ezPropertyFlags::Const | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum | ezPropertyFlags::Pointer),
        ParamSig(ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezExampleEnum>(), ezPropertyFlags::IsEnum));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

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
        ParamSig(ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags),
        ParamSig(ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags | ezPropertyFlags::Const | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags | ezPropertyFlags::Pointer),
        ParamSig(ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet),
                            ParamSig(ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

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
        ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class),
        ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class),
        ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer),
        ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

    FunctionTest test;
    ezTestStruct3 retS;
    retS.m_fFloat1 = 0;
    retS.m_UInt8 = 0;
    ezTestStruct3 value;
    value.m_fFloat1 = 0;
    value.m_UInt8 = 0;
    ezTestStruct3 rs;
    rs.m_fFloat1 = 42;
    ezTestStruct3 ps;
    ps.m_fFloat1 = 18;

    test.m_values.PushBack(ezVariant(&value));
    test.m_values.PushBack(ezVariant(&value));
    test.m_values.PushBack(ezVariant(&rs));
    test.m_values.PushBack(ezVariant(&value));
    test.m_values.PushBack(ezVariant(&ps));
    test.m_values.PushBack(ezVariant(&value));

    ezVariant ret(&retS);
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
        ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class),
        ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class),
        ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer),
        ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

    FunctionTest test;
    ezTestClass1 retS;
    retS.m_Color = ezColor::Chocolate;
    ezTestClass1 value;
    value.m_Color = ezColor::AliceBlue;
    ezTestClass1 rs;
    rs.m_Color = ezColor::Beige;
    ezTestClass1 ps;
    ps.m_Color = ezColor::DarkBlue;

    test.m_values.PushBack(ezVariant(&value));
    test.m_values.PushBack(ezVariant(&value));
    test.m_values.PushBack(ezVariant(&rs));
    test.m_values.PushBack(ezVariant(&value));
    test.m_values.PushBack(ezVariant(&ps));
    test.m_values.PushBack(ezVariant(&value));

    rs.m_Color.SetRGB(1, 2, 3);
    rs.m_MyVector.Set(1, 2, 3);


    ezVariant ret(&retS);
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
        ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType | ezPropertyFlags::Pointer),
        ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet),
                            ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Static Functions")
  {
    // Void return
    ezFunctionProperty<decltype(&FunctionTest::StaticFunction)> funccall("", &FunctionTest::StaticFunction);
    ParamSig testSet[] = {
        ParamSig(ezGetStaticRTTI<bool>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<void>(), ezPropertyFlags::Void));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::StaticMember);

    ezDynamicArray<ezVariant> values;
    values.PushBack(true);
    values.PushBack(4.0f);
    ezVariant ret;
    funccall.Execute(nullptr, values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Invalid);

    // Zero parameter
    ezFunctionProperty<decltype(&FunctionTest::StaticFunction2)> funccall2("", &FunctionTest::StaticFunction2);
    VerifyFunctionSignature(&funccall2, ezArrayPtr<ParamSig>(), ParamSig(ezGetStaticRTTI<int>(), ezPropertyFlags::StandardType));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::StaticMember);
    values.Clear();
    funccall2.Execute(nullptr, values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Int32);
    EZ_TEST_BOOL(ret == 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor Functions - StandardTypes")
  {
    ezConstructorFunctionProperty<ezVec4, float, float, float, float> funccall;
    ParamSig testSet[] = {
        ParamSig(ezGetStaticRTTI<float>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<float>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<float>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<float>(), ezPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet),
                            ParamSig(ezGetStaticRTTI<ezVec4>(), ezPropertyFlags::StandardType | ezPropertyFlags::Pointer));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Constructor);

    ezDynamicArray<ezVariant> values;
    values.PushBack(1.0f);
    values.PushBack(2.0f);
    values.PushBack(3.0f);
    values.PushBack(4.0f);
    ezVariant ret;
    funccall.Execute(nullptr, values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::Vector4);
    EZ_TEST_BOOL(ret == ezVec4(1.0f, 2.0f, 3.0f, 4.0f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor Functions - Struct")
  {
    ezConstructorFunctionProperty<ezTestStruct3, double, ezInt16> funccall;
    ParamSig testSet[] = {
        ParamSig(ezGetStaticRTTI<double>(), ezPropertyFlags::StandardType),
        ParamSig(ezGetStaticRTTI<ezInt16>(), ezPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet),
                            ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Constructor);

    ezDynamicArray<ezVariant> values;
    values.PushBack(59.0);
    values.PushBack((ezInt16)666);
    ezVariant ret;
    funccall.Execute(nullptr, values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::VoidPointer);
    ezTestStruct3* pRet = static_cast<ezTestStruct3*>(ret.ConvertTo<void*>());
    EZ_TEST_BOOL(pRet != nullptr);

    EZ_TEST_FLOAT(pRet->m_fFloat1, 59.0, 0);
    EZ_TEST_INT(pRet->m_UInt8, 666);
    EZ_TEST_INT(pRet->GetIntPublic(), 32);

    EZ_DEFAULT_DELETE(pRet);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor Functions - Reflected Classes")
  {
    // The function signature does not actually need to match the ctor 100% as long as implicit conversion is possible.
    ezConstructorFunctionProperty<ezTestClass1, const ezColor&, const ezTestStruct&> funccall;
    ParamSig testSet[] = {
        ParamSig(ezGetStaticRTTI<ezColor>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const | ezPropertyFlags::Reference),
        ParamSig(ezGetStaticRTTI<ezTestStruct>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Reference),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet),
                            ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Constructor);

    ezDynamicArray<ezVariant> values;
    ezTestStruct s;
    s.m_fFloat1 = 1.0f;
    s.m_UInt8 = 255;
    values.PushBack(ezColor::CornflowerBlue);
    values.PushBack(ezVariant(&s));
    ezVariant ret;
    funccall.Execute(nullptr, values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::ReflectedPointer);
    ezTestClass1* pRet = static_cast<ezTestClass1*>(ret.ConvertTo<void*>());
    EZ_TEST_BOOL(pRet != nullptr);

    EZ_TEST_BOOL(pRet->m_Color == ezColor::CornflowerBlue);
    EZ_TEST_BOOL(pRet->m_Struct == s);
    EZ_TEST_BOOL(pRet->m_MyVector == ezVec3(1, 2, 3));

    EZ_DEFAULT_DELETE(pRet);
  }
}
