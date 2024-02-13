#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

struct FunctionTest
{
  int StandardTypeFunction(int v, const ezVec2 vCv, ezVec3& ref_vRv, const ezVec4& vCrv, ezVec2U32* pPv, const ezVec3U32* pCpv)
  {
    EZ_TEST_BOOL(m_values[0] == v);
    EZ_TEST_BOOL(m_values[1] == vCv);
    EZ_TEST_BOOL(m_values[2] == ref_vRv);
    EZ_TEST_BOOL(m_values[3] == vCrv);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pPv);
      EZ_TEST_BOOL(!pCpv);
    }
    else
    {
      EZ_TEST_BOOL(m_values[4] == *pPv);
      EZ_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_vRv.Set(1, 2, 3);
    if (pPv)
    {
      pPv->Set(1, 2);
    }
    return 5;
  }

  ezVarianceTypeAngle CustomTypeFunction(ezVarianceTypeAngle v, const ezVarianceTypeAngle cv, ezVarianceTypeAngle& ref_rv, const ezVarianceTypeAngle& crv, ezVarianceTypeAngle* pPv, const ezVarianceTypeAngle* pCpv)
  {
    EZ_TEST_BOOL(m_values[0] == v);
    EZ_TEST_BOOL(m_values[1] == cv);
    EZ_TEST_BOOL(m_values[2] == ref_rv);
    EZ_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pPv);
      EZ_TEST_BOOL(!pCpv);
    }
    else
    {
      EZ_TEST_BOOL(m_values[4] == *pPv);
      EZ_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_rv = {2.0f, ezAngle::MakeFromDegree(200.0f)};
    if (pPv)
    {
      *pPv = {4.0f, ezAngle::MakeFromDegree(400.0f)};
    }
    return {0.6f, ezAngle::MakeFromDegree(60.0f)};
  }

  ezVarianceTypeAngle CustomTypeFunction2(ezVarianceTypeAngle v, const ezVarianceTypeAngle cv, ezVarianceTypeAngle& ref_rv, const ezVarianceTypeAngle& crv, ezVarianceTypeAngle* pPv, const ezVarianceTypeAngle* pCpv)
  {
    EZ_TEST_BOOL(*m_values[0].Get<ezVarianceTypeAngle*>() == v);
    EZ_TEST_BOOL(*m_values[1].Get<ezVarianceTypeAngle*>() == cv);
    EZ_TEST_BOOL(*m_values[2].Get<ezVarianceTypeAngle*>() == ref_rv);
    EZ_TEST_BOOL(*m_values[3].Get<ezVarianceTypeAngle*>() == crv);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pPv);
      EZ_TEST_BOOL(!pCpv);
    }
    else
    {
      EZ_TEST_BOOL(*m_values[4].Get<ezVarianceTypeAngle*>() == *pPv);
      EZ_TEST_BOOL(*m_values[5].Get<ezVarianceTypeAngle*>() == *pCpv);
    }
    ref_rv = {2.0f, ezAngle::MakeFromDegree(200.0f)};
    if (pPv)
    {
      *pPv = {4.0f, ezAngle::MakeFromDegree(400.0f)};
    }
    return {0.6f, ezAngle::MakeFromDegree(60.0f)};
  }

  const char* StringTypeFunction(const char* szString, ezString& ref_sString, ezStringView sView)
  {
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!szString);
    }
    else
    {
      EZ_TEST_BOOL(m_values[0] == szString);
    }
    EZ_TEST_BOOL(m_values[1] == ref_sString);
    EZ_TEST_BOOL(m_values[2] == sView);
    return "StringRet";
  }

  ezEnum<ezExampleEnum> EnumFunction(
    ezEnum<ezExampleEnum> e, ezEnum<ezExampleEnum>& ref_re, const ezEnum<ezExampleEnum>& cre, ezEnum<ezExampleEnum>* pPe, const ezEnum<ezExampleEnum>* pCpe)
  {
    EZ_TEST_BOOL(m_values[0].Get<ezInt64>() == e.GetValue());
    EZ_TEST_BOOL(m_values[1].Get<ezInt64>() == ref_re.GetValue());
    EZ_TEST_BOOL(m_values[2].Get<ezInt64>() == cre.GetValue());
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pPe);
      EZ_TEST_BOOL(!pCpe);
    }
    else
    {
      EZ_TEST_BOOL(m_values[3].Get<ezInt64>() == pPe->GetValue());
      EZ_TEST_BOOL(m_values[4].Get<ezInt64>() == pCpe->GetValue());
    }
    return ezExampleEnum::Value1;
  }

  ezBitflags<ezExampleBitflags> BitflagsFunction(ezBitflags<ezExampleBitflags> e, ezBitflags<ezExampleBitflags>& ref_re,
    const ezBitflags<ezExampleBitflags>& cre, ezBitflags<ezExampleBitflags>* pPe, const ezBitflags<ezExampleBitflags>* pCpe)
  {
    EZ_TEST_BOOL(e == m_values[0].Get<ezInt64>());
    EZ_TEST_BOOL(ref_re == m_values[1].Get<ezInt64>());
    EZ_TEST_BOOL(cre == m_values[2].Get<ezInt64>());
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pPe);
      EZ_TEST_BOOL(!pCpe);
    }
    else
    {
      EZ_TEST_BOOL(*pPe == m_values[3].Get<ezInt64>());
      EZ_TEST_BOOL(*pCpe == m_values[4].Get<ezInt64>());
    }
    return ezExampleBitflags::Value1 | ezExampleBitflags::Value2;
  }

  ezTestStruct3 StructFunction(
    ezTestStruct3 s, const ezTestStruct3 cs, ezTestStruct3& ref_rs, const ezTestStruct3& crs, ezTestStruct3* pPs, const ezTestStruct3* pCps)
  {
    EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[0].Get<void*>()) == s);
    EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[1].Get<void*>()) == cs);
    EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[2].Get<void*>()) == ref_rs);
    EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[3].Get<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pPs);
      EZ_TEST_BOOL(!pCps);
    }
    else
    {
      EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[4].Get<void*>()) == *pPs);
      EZ_TEST_BOOL(*static_cast<ezTestStruct3*>(m_values[5].Get<void*>()) == *pCps);
    }
    ref_rs.m_fFloat1 = 999.0f;
    ref_rs.m_UInt8 = 666;
    if (pPs)
    {
      pPs->m_fFloat1 = 666.0f;
      pPs->m_UInt8 = 999;
    }
    ezTestStruct3 retS;
    retS.m_fFloat1 = 42;
    retS.m_UInt8 = 42;
    return retS;
  }

  ezTestClass1 ReflectedClassFunction(
    ezTestClass1 s, const ezTestClass1 cs, ezTestClass1& ref_rs, const ezTestClass1& crs, ezTestClass1* pPs, const ezTestClass1* pCps)
  {
    EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[0].ConvertTo<void*>()) == s);
    EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[1].ConvertTo<void*>()) == cs);
    EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[2].ConvertTo<void*>()) == ref_rs);
    EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[3].ConvertTo<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pPs);
      EZ_TEST_BOOL(!pCps);
    }
    else
    {
      EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[4].ConvertTo<void*>()) == *pPs);
      EZ_TEST_BOOL(*static_cast<ezTestClass1*>(m_values[5].ConvertTo<void*>()) == *pCps);
    }
    ref_rs.m_Color.SetRGB(1, 2, 3);
    ref_rs.m_MyVector.Set(1, 2, 3);
    if (pPs)
    {
      pPs->m_Color.SetRGB(1, 2, 3);
      pPs->m_MyVector.Set(1, 2, 3);
    }
    ezTestClass1 retS;
    retS.m_Color.SetRGB(42, 42, 42);
    retS.m_MyVector.Set(42, 42, 42);
    return retS;
  }

  ezVariant VariantFunction(ezVariant v, const ezVariant cv, ezVariant& ref_rv, const ezVariant& crv, ezVariant* pPv, const ezVariant* pCpv)
  {
    EZ_TEST_BOOL(m_values[0] == v);
    EZ_TEST_BOOL(m_values[1] == cv);
    EZ_TEST_BOOL(m_values[2] == ref_rv);
    EZ_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      // Can't have variant as nullptr as it must exist in the array and there is no further
      // way of distinguishing a between a ezVariant* and a ezVariant that is invalid.
      EZ_TEST_BOOL(!pPv->IsValid());
      EZ_TEST_BOOL(!pCpv->IsValid());
    }
    else
    {
      EZ_TEST_BOOL(m_values[4] == *pPv);
      EZ_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_rv = ezVec3(1, 2, 3);
    if (pPv)
    {
      *pPv = ezVec2U32(1, 2);
    }
    return 5;
  }

  ezVariantArray VariantArrayFunction(ezVariantArray a, const ezVariantArray ca, ezVariantArray& ref_a, const ezVariantArray& cra, ezVariantArray* pA, const ezVariantArray* pCa)
  {
    EZ_TEST_BOOL(m_values[0].Get<ezVariantArray>() == a);
    EZ_TEST_BOOL(m_values[1].Get<ezVariantArray>() == ca);
    EZ_TEST_BOOL(m_values[2].Get<ezVariantArray>() == ref_a);
    EZ_TEST_BOOL(m_values[3].Get<ezVariantArray>() == cra);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pA);
      EZ_TEST_BOOL(!pCa);
    }
    else
    {
      EZ_TEST_BOOL(m_values[4] == *pA);
      EZ_TEST_BOOL(m_values[5] == *pCa);
    }
    ref_a.Clear();
    ref_a.PushBack(1.0f);
    ref_a.PushBack("Test");
    if (pA)
    {
      pA->Clear();
      pA->PushBack(2.0f);
      pA->PushBack("Test2");
    }

    ezVariantArray ret;
    ret.PushBack(3.0f);
    ret.PushBack("RetTest");
    return ret;
  }

  ezVariantDictionary VariantDictionaryFunction(ezVariantDictionary a, const ezVariantDictionary ca, ezVariantDictionary& ref_a, const ezVariantDictionary& cra, ezVariantDictionary* pA, const ezVariantDictionary* pCa)
  {
    EZ_TEST_BOOL(m_values[0].Get<ezVariantDictionary>() == a);
    EZ_TEST_BOOL(m_values[1].Get<ezVariantDictionary>() == ca);
    EZ_TEST_BOOL(m_values[2].Get<ezVariantDictionary>() == ref_a);
    EZ_TEST_BOOL(m_values[3].Get<ezVariantDictionary>() == cra);
    if (m_bPtrAreNull)
    {
      EZ_TEST_BOOL(!pA);
      EZ_TEST_BOOL(!pCa);
    }
    else
    {
      EZ_TEST_BOOL(m_values[4] == *pA);
      EZ_TEST_BOOL(m_values[5] == *pCa);
    }
    ref_a.Clear();
    ref_a.Insert("f", 1.0f);
    ref_a.Insert("s", "Test");
    if (pA)
    {
      pA->Clear();
      pA->Insert("f", 2.0f);
      pA->Insert("s", "Test2");
    }

    ezVariantDictionary ret;
    ret.Insert("f", 3.0f);
    ret.Insert("s", "RetTest");
    return ret;
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

using ParamSig = std::tuple<const ezRTTI*, ezBitflags<ezPropertyFlags>>;

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - CustomType")
  {
    ezFunctionProperty<decltype(&FunctionTest::CustomTypeFunction)> funccall("", &FunctionTest::CustomTypeFunction);
    ParamSig testSet[] = {
      ParamSig(ezGetStaticRTTI<ezVarianceTypeAngle>(), ezPropertyFlags::Class),
      ParamSig(ezGetStaticRTTI<ezVarianceTypeAngle>(), ezPropertyFlags::Class),
      ParamSig(ezGetStaticRTTI<ezVarianceTypeAngle>(), ezPropertyFlags::Class | ezPropertyFlags::Reference),
      ParamSig(ezGetStaticRTTI<ezVarianceTypeAngle>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Reference),
      ParamSig(ezGetStaticRTTI<ezVarianceTypeAngle>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer),
      ParamSig(ezGetStaticRTTI<ezVarianceTypeAngle>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezVarianceTypeAngle>(), ezPropertyFlags::Class));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

    {
      FunctionTest test;
      test.m_values.PushBack(ezVarianceTypeAngle{0.0f, ezAngle::MakeFromDegree(0.0f)});
      test.m_values.PushBack(ezVarianceTypeAngle{0.1f, ezAngle::MakeFromDegree(10.0f)});
      test.m_values.PushBack(ezVarianceTypeAngle{0.2f, ezAngle::MakeFromDegree(20.0f)});
      test.m_values.PushBack(ezVarianceTypeAngle{0.3f, ezAngle::MakeFromDegree(30.0f)});
      test.m_values.PushBack(ezVarianceTypeAngle{0.4f, ezAngle::MakeFromDegree(40.0f)});
      test.m_values.PushBack(ezVarianceTypeAngle{0.5f, ezAngle::MakeFromDegree(50.0f)});

      ezVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      EZ_TEST_BOOL(ret.GetType() == ezVariantType::TypedObject);
      EZ_TEST_BOOL(ret == ezVariant(ezVarianceTypeAngle{0.6f, ezAngle::MakeFromDegree(60.0f)}));
      EZ_TEST_BOOL(test.m_values[2] == ezVariant(ezVarianceTypeAngle{2.0f, ezAngle::MakeFromDegree(200.0f)}));
      EZ_TEST_BOOL(test.m_values[4] == ezVariant(ezVarianceTypeAngle{4.0f, ezAngle::MakeFromDegree(400.0f)}));

      test.m_bPtrAreNull = true;
      test.m_values[4] = ezVariant();
      test.m_values[5] = ezVariant();
      ret = ezVariant();
      funccall.Execute(&test, test.m_values, ret);
      EZ_TEST_BOOL(ret.GetType() == ezVariantType::TypedObject);
      EZ_TEST_BOOL(ret == ezVariant(ezVarianceTypeAngle{0.6f, ezAngle::MakeFromDegree(60.0f)}));
    }

    {
      ezFunctionProperty<decltype(&FunctionTest::CustomTypeFunction2)> funccall2("", &FunctionTest::CustomTypeFunction2);

      FunctionTest test;
      ezVarianceTypeAngle v0{0.0f, ezAngle::MakeFromDegree(0.0f)};
      ezVarianceTypeAngle v1{0.1f, ezAngle::MakeFromDegree(10.0f)};
      ezVarianceTypeAngle v2{0.2f, ezAngle::MakeFromDegree(20.0f)};
      ezVarianceTypeAngle v3{0.3f, ezAngle::MakeFromDegree(30.0f)};
      ezVarianceTypeAngle v4{0.4f, ezAngle::MakeFromDegree(40.0f)};
      ezVarianceTypeAngle v5{0.5f, ezAngle::MakeFromDegree(50.0f)};
      test.m_values.PushBack(&v0);
      test.m_values.PushBack(&v1);
      test.m_values.PushBack(&v2);
      test.m_values.PushBack(&v3);
      test.m_values.PushBack(&v4);
      test.m_values.PushBack(&v5);

      ezVariant ret;
      funccall2.Execute(&test, test.m_values, ret);
      EZ_TEST_BOOL(ret.GetType() == ezVariantType::TypedObject);
      EZ_TEST_BOOL(ret == ezVariant(ezVarianceTypeAngle{0.6f, ezAngle::MakeFromDegree(60.0f)}));
      EZ_TEST_BOOL((*test.m_values[2].Get<ezVarianceTypeAngle*>() == ezVarianceTypeAngle{2.0f, ezAngle::MakeFromDegree(200.0f)}));
      EZ_TEST_BOOL((*test.m_values[4].Get<ezVarianceTypeAngle*>() == ezVarianceTypeAngle{4.0f, ezAngle::MakeFromDegree(400.0f)}));

      test.m_bPtrAreNull = true;
      test.m_values[4] = ezVariant();
      test.m_values[5] = ezVariant();
      ret = ezVariant();
      funccall2.Execute(&test, test.m_values, ret);
      EZ_TEST_BOOL(ret.GetType() == ezVariantType::TypedObject);
      EZ_TEST_BOOL(ret == ezVariant(ezVarianceTypeAngle{0.6f, ezAngle::MakeFromDegree(60.0f)}));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - Strings")
  {
    ezFunctionProperty<decltype(&FunctionTest::StringTypeFunction)> funccall("", &FunctionTest::StringTypeFunction);
    ParamSig testSet[] = {
      ParamSig(ezGetStaticRTTI<const char*>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const),
      ParamSig(ezGetStaticRTTI<ezString>(), ezPropertyFlags::StandardType | ezPropertyFlags::Reference),
      ParamSig(ezGetStaticRTTI<ezStringView>(), ezPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<const char*>(), ezPropertyFlags::StandardType | ezPropertyFlags::Const));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(ezVariant(ezString("String0")));
    test.m_values.PushBack(ezVariant(ezString("String1")));
    test.m_values.PushBack(ezVariant(ezStringView("String2"), false));

    {
      // Exact types
      ezVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      EZ_TEST_BOOL(ret.GetType() == ezVariantType::String);
      EZ_TEST_BOOL(ret == ezString("StringRet"));
    }

    {
      // Using ezString instead of ezStringView
      test.m_values[2] = ezString("String2");
      ezVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      EZ_TEST_BOOL(ret.GetType() == ezVariantType::String);
      EZ_TEST_BOOL(ret == ezString("StringRet"));
      test.m_values[2] = ezVariant(ezStringView("String2"), false);
    }

    {
      // Using nullptr instead of const char*
      test.m_bPtrAreNull = true;
      test.m_values[0] = ezVariant();
      ezVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      EZ_TEST_BOOL(ret.GetType() == ezVariantType::String);
      EZ_TEST_BOOL(ret == ezString("StringRet"));
    }
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
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezExampleBitflags>(), ezPropertyFlags::Bitflags));
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

    // ezVariantAdapter<ezTestStruct3 const*> aa(ezVariant(&value));
    // auto bla = ezIsStandardType<ezTestStruct3 const*>::value;

    ezVariant ret(&retS);
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_FLOAT(retS.m_fFloat1, 42, 0);
    EZ_TEST_INT(retS.m_UInt8, 42);

    EZ_TEST_FLOAT(rs.m_fFloat1, 999, 0);
    EZ_TEST_INT(rs.m_UInt8, 666);

    EZ_TEST_DOUBLE(ps.m_fFloat1, 666, 0);
    EZ_TEST_INT(ps.m_UInt8, 999);

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
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezVariant>(), ezPropertyFlags::StandardType));
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - VariantArray")
  {
    ezFunctionProperty<decltype(&FunctionTest::VariantArrayFunction)> funccall("", &FunctionTest::VariantArrayFunction);
    ParamSig testSet[] = {
      ParamSig(ezGetStaticRTTI<ezVariantArray>(), ezPropertyFlags::Class),
      ParamSig(ezGetStaticRTTI<ezVariantArray>(), ezPropertyFlags::Class),
      ParamSig(ezGetStaticRTTI<ezVariantArray>(), ezPropertyFlags::Class | ezPropertyFlags::Reference),
      ParamSig(ezGetStaticRTTI<ezVariantArray>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Reference),
      ParamSig(ezGetStaticRTTI<ezVariantArray>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer),
      ParamSig(ezGetStaticRTTI<ezVariantArray>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezVariantArray>(), ezPropertyFlags::Class));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

    ezVariantArray testA;
    testA.PushBack(ezVec3(3));
    testA.PushBack(ezTime::MakeFromHours(22));
    testA.PushBack("Hello");

    FunctionTest test;
    for (ezUInt32 i = 0; i < 6; ++i)
    {
      test.m_values.PushBack(testA);
      testA.PushBack(i);
    }

    ezVariantArray expectedOutRef;
    expectedOutRef.PushBack(1.0f);
    expectedOutRef.PushBack("Test");

    ezVariantArray expectedOutPtr;
    expectedOutPtr.PushBack(2.0f);
    expectedOutPtr.PushBack("Test2");

    ezVariantArray expectedRet;
    expectedRet.PushBack(3.0f);
    expectedRet.PushBack("RetTest");

    ezVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::VariantArray);
    EZ_TEST_BOOL(ret.Get<ezVariantArray>() == expectedRet);
    EZ_TEST_BOOL(test.m_values[2] == expectedOutRef);
    EZ_TEST_BOOL(test.m_values[4] == expectedOutPtr);

    test.m_bPtrAreNull = true;
    test.m_values[4] = ezVariant();
    test.m_values[5] = ezVariant();
    ret = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::VariantArray);
    EZ_TEST_BOOL(ret.Get<ezVariantArray>() == expectedRet);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Functions - VariantDictionary")
  {
    ezFunctionProperty<decltype(&FunctionTest::VariantDictionaryFunction)> funccall("", &FunctionTest::VariantDictionaryFunction);
    ParamSig testSet[] = {
      ParamSig(ezGetStaticRTTI<ezVariantDictionary>(), ezPropertyFlags::Class),
      ParamSig(ezGetStaticRTTI<ezVariantDictionary>(), ezPropertyFlags::Class),
      ParamSig(ezGetStaticRTTI<ezVariantDictionary>(), ezPropertyFlags::Class | ezPropertyFlags::Reference),
      ParamSig(ezGetStaticRTTI<ezVariantDictionary>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Reference),
      ParamSig(ezGetStaticRTTI<ezVariantDictionary>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer),
      ParamSig(ezGetStaticRTTI<ezVariantDictionary>(), ezPropertyFlags::Class | ezPropertyFlags::Const | ezPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezVariantDictionary>(), ezPropertyFlags::Class));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Member);

    ezVariantDictionary testA;
    testA.Insert("v", ezVec3(3));
    testA.Insert("t", ezTime::MakeFromHours(22));
    testA.Insert("s", "Hello");

    ezStringBuilder tmp;
    FunctionTest test;
    for (ezUInt32 i = 0; i < 6; ++i)
    {
      test.m_values.PushBack(testA);
      testA.Insert(ezConversionUtils::ToString(i, tmp), i);
    }

    ezVariantDictionary expectedOutRef;
    expectedOutRef.Insert("f", 1.0f);
    expectedOutRef.Insert("s", "Test");

    ezVariantDictionary expectedOutPtr;
    expectedOutPtr.Insert("f", 2.0f);
    expectedOutPtr.Insert("s", "Test2");

    ezVariantDictionary expectedRet;
    expectedRet.Insert("f", 3.0f);
    expectedRet.Insert("s", "RetTest");

    ezVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::VariantDictionary);
    EZ_TEST_BOOL(ret.Get<ezVariantDictionary>() == expectedRet);
    EZ_TEST_BOOL(test.m_values[2] == expectedOutRef);
    EZ_TEST_BOOL(test.m_values[4] == expectedOutPtr);

    test.m_bPtrAreNull = true;
    test.m_values[4] = ezVariant();
    test.m_values[5] = ezVariant();
    ret = ezVariant();
    funccall.Execute(&test, test.m_values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::VariantDictionary);
    EZ_TEST_BOOL(ret.Get<ezVariantDictionary>() == expectedRet);
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
    VerifyFunctionSignature(
      &funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezVec4>(), ezPropertyFlags::StandardType | ezPropertyFlags::Pointer));
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
    VerifyFunctionSignature(
      &funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezTestStruct3>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Constructor);

    ezDynamicArray<ezVariant> values;
    values.PushBack(59.0);
    values.PushBack((ezInt16)666);
    ezVariant ret;
    funccall.Execute(nullptr, values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::TypedPointer);
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
    VerifyFunctionSignature(
      &funccall, ezArrayPtr<ParamSig>(testSet), ParamSig(ezGetStaticRTTI<ezTestClass1>(), ezPropertyFlags::Class | ezPropertyFlags::Pointer));
    EZ_TEST_BOOL(funccall.GetFunctionType() == ezFunctionType::Constructor);

    ezDynamicArray<ezVariant> values;
    ezTestStruct s;
    s.m_fFloat1 = 1.0f;
    s.m_UInt8 = 255;
    values.PushBack(ezColor::CornflowerBlue);
    values.PushBack(ezVariant(&s));
    ezVariant ret;
    funccall.Execute(nullptr, values, ret);
    EZ_TEST_BOOL(ret.GetType() == ezVariantType::TypedPointer);
    ezTestClass1* pRet = static_cast<ezTestClass1*>(ret.ConvertTo<void*>());
    EZ_TEST_BOOL(pRet != nullptr);

    EZ_TEST_BOOL(pRet->m_Color == ezColor::CornflowerBlue);
    EZ_TEST_BOOL(pRet->m_Struct == s);
    EZ_TEST_BOOL(pRet->m_MyVector == ezVec3(1, 2, 3));

    EZ_DEFAULT_DELETE(pRet);
  }
}
