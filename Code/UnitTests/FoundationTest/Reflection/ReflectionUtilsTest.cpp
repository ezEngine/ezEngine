#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

template <typename T>
static void SetComponentTest(ezVec2Template<T> vVector, T value)
{
  ezVariant var = vVector;
  ezReflectionUtils::SetComponent(var, 0, value);
  EZ_TEST_BOOL(var.Get<ezVec2Template<T>>().x == value);
  ezReflectionUtils::SetComponent(var, 1, value);
  EZ_TEST_BOOL(var.Get<ezVec2Template<T>>().y == value);
}

template <typename T>
static void SetComponentTest(ezVec3Template<T> vVector, T value)
{
  ezVariant var = vVector;
  ezReflectionUtils::SetComponent(var, 0, value);
  EZ_TEST_BOOL(var.Get<ezVec3Template<T>>().x == value);
  ezReflectionUtils::SetComponent(var, 1, value);
  EZ_TEST_BOOL(var.Get<ezVec3Template<T>>().y == value);
  ezReflectionUtils::SetComponent(var, 2, value);
  EZ_TEST_BOOL(var.Get<ezVec3Template<T>>().z == value);
}

template <typename T>
static void SetComponentTest(ezVec4Template<T> vVector, T value)
{
  ezVariant var = vVector;
  ezReflectionUtils::SetComponent(var, 0, value);
  EZ_TEST_BOOL(var.Get<ezVec4Template<T>>().x == value);
  ezReflectionUtils::SetComponent(var, 1, value);
  EZ_TEST_BOOL(var.Get<ezVec4Template<T>>().y == value);
  ezReflectionUtils::SetComponent(var, 2, value);
  EZ_TEST_BOOL(var.Get<ezVec4Template<T>>().z == value);
  ezReflectionUtils::SetComponent(var, 3, value);
  EZ_TEST_BOOL(var.Get<ezVec4Template<T>>().w == value);
}

template <class T>
static void ClampValueTest(T tooSmall, T tooBig, T min, T max)
{
  ezClampValueAttribute minClamp(min, {});
  ezClampValueAttribute maxClamp({}, max);
  ezClampValueAttribute bothClamp(min, max);

  ezVariant value = tooSmall;
  EZ_TEST_BOOL(ezReflectionUtils::ClampValue(value, &minClamp).Succeeded());
  EZ_TEST_BOOL(value == min);

  value = tooSmall;
  EZ_TEST_BOOL(ezReflectionUtils::ClampValue(value, &bothClamp).Succeeded());
  EZ_TEST_BOOL(value == min);

  value = tooBig;
  EZ_TEST_BOOL(ezReflectionUtils::ClampValue(value, &maxClamp).Succeeded());
  EZ_TEST_BOOL(value == max);

  value = tooBig;
  EZ_TEST_BOOL(ezReflectionUtils::ClampValue(value, &bothClamp).Succeeded());
  EZ_TEST_BOOL(value == max);
}


EZ_CREATE_SIMPLE_TEST(Reflection, Utils)
{
  ezDefaultMemoryStreamStorage StreamStorage;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "WriteObjectToDDL")
  {
    ezMemoryStreamWriter FileOut(&StreamStorage);

    ezTestClass2 c2;
    c2.SetCharPtr("Hallo");
    c2.SetString("World");
    c2.SetStringView("!!!");
    c2.m_MyVector.Set(14, 16, 18);
    c2.m_Struct.m_fFloat1 = 128;
    c2.m_Struct.m_UInt8 = 234;
    c2.m_Struct.m_Angle = ezAngle::MakeFromDegree(360);
    c2.m_Struct.m_vVec3I = ezVec3I32(9, 8, 7);
    c2.m_Struct.m_DataBuffer.Clear();
    c2.m_Color = ezColor(0.1f, 0.2f, 0.3f);
    c2.m_Time = ezTime::MakeFromSeconds(91.0f);
    c2.m_enumClass = ezExampleEnum::Value3;
    c2.m_bitflagsClass = ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3;
    c2.m_array.PushBack(5.0f);
    c2.m_array.PushBack(10.0f);
    c2.m_Variant = ezVec3(1.0f, 2.0f, 3.0f);

    ezReflectionSerializer::WriteObjectToDDL(FileOut, c2.GetDynamicRTTI(), &c2, false, ezOpenDdlWriter::TypeStringMode::Compliant);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromDDL")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    ezTestClass2 c2;

    ezReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *c2.GetDynamicRTTI(), &c2);

    EZ_TEST_STRING(c2.GetCharPtr(), "Hallo");
    EZ_TEST_STRING(c2.GetString(), "World");
    EZ_TEST_STRING(c2.GetStringView(), "!!!");
    EZ_TEST_VEC3(c2.m_MyVector, ezVec3(3, 4, 5), 0.0f);
    EZ_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);
    EZ_TEST_BOOL(c2.m_Struct.m_Angle == ezAngle::MakeFromDegree(360));
    EZ_TEST_BOOL(c2.m_Struct.m_vVec3I == ezVec3I32(9, 8, 7));
    EZ_TEST_BOOL(c2.m_Struct.m_DataBuffer == ezDataBuffer());
    EZ_TEST_BOOL(c2.m_enumClass == ezExampleEnum::Value3);
    EZ_TEST_BOOL(c2.m_bitflagsClass == (ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3));
    EZ_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      EZ_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      EZ_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    EZ_TEST_VEC3(c2.m_Variant.Get<ezVec3>(), ezVec3(1.0f, 2.0f, 3.0f), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromDDL (different type)")
  {
    // here we restore the same properties into a different type of object which has properties that are named the same
    // but may have slightly different types (but which are compatible)

    ezMemoryStreamReader FileIn(&StreamStorage);

    ezTestClass2b c2;

    ezReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *c2.GetDynamicRTTI(), &c2);

    EZ_TEST_STRING(c2.GetText(), "Tut"); // not restored, different property name
    EZ_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectFromDDL")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    const ezRTTI* pRtti;
    void* pObject = ezReflectionSerializer::ReadObjectFromDDL(FileIn, pRtti);

    ezTestClass2& c2 = *((ezTestClass2*)pObject);

    EZ_TEST_STRING(c2.GetCharPtr(), "Hallo");
    EZ_TEST_STRING(c2.GetString(), "World");
    EZ_TEST_STRING(c2.GetStringView(), "!!!");
    EZ_TEST_VEC3(c2.m_MyVector, ezVec3(3, 4, 5), 0.0f);
    EZ_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);
    EZ_TEST_BOOL(c2.m_Struct.m_Angle == ezAngle::MakeFromDegree(360));
    EZ_TEST_BOOL(c2.m_Struct.m_vVec3I == ezVec3I32(9, 8, 7));
    EZ_TEST_BOOL(c2.m_Struct.m_DataBuffer == ezDataBuffer());
    EZ_TEST_BOOL(c2.m_enumClass == ezExampleEnum::Value3);
    EZ_TEST_BOOL(c2.m_bitflagsClass == (ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3));
    EZ_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      EZ_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      EZ_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    EZ_TEST_VEC3(c2.m_Variant.Get<ezVec3>(), ezVec3(1.0f, 2.0f, 3.0f), 0.0f);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  ezFileSystem::ClearAllDataDirectories();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetComponent")
  {
    SetComponentTest(ezVec2(0.0f, 0.1f), -0.5f);
    SetComponentTest(ezVec3(0.0f, 0.1f, 0.2f), -0.5f);
    SetComponentTest(ezVec4(0.0f, 0.1f, 0.2f, 0.3f), -0.5f);
    SetComponentTest(ezVec2I32(0, 1), -4);
    SetComponentTest(ezVec3I32(0, 1, 2), -4);
    SetComponentTest(ezVec4I32(0, 1, 2, 3), -4);
    SetComponentTest(ezVec2U32(0, 1), 4u);
    SetComponentTest(ezVec3U32(0, 1, 2), 4u);
    SetComponentTest(ezVec4U32(0, 1, 2, 3), 4u);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClampValue")
  {
    ClampValueTest<float>(-1, 1000, 2, 4);
    ClampValueTest<double>(-1, 1000, 2, 4);
    ClampValueTest<ezInt32>(-1, 1000, 2, 4);
    ClampValueTest<ezUInt64>(1, 1000, 2, 4);
    ClampValueTest<ezTime>(ezTime::MakeFromMilliseconds(1), ezTime::MakeFromMilliseconds(1000), ezTime::MakeFromMilliseconds(2), ezTime::MakeFromMilliseconds(4));
    ClampValueTest<ezAngle>(ezAngle::MakeFromDegree(1), ezAngle::MakeFromDegree(1000), ezAngle::MakeFromDegree(2), ezAngle::MakeFromDegree(4));
    ClampValueTest<ezVec3>(ezVec3(1), ezVec3(1000), ezVec3(2), ezVec3(4));
    ClampValueTest<ezVec4I32>(ezVec4I32(1), ezVec4I32(1000), ezVec4I32(2), ezVec4I32(4));
    ClampValueTest<ezVec4U32>(ezVec4U32(1), ezVec4U32(1000), ezVec4U32(2), ezVec4U32(4));

    ezVarianceTypeFloat vf = {1.0f, 2.0f};
    ezVariant variance = vf;
    EZ_TEST_BOOL(ezReflectionUtils::ClampValue(variance, nullptr).Succeeded());

    ezVarianceTypeFloat clamp = {2.0f, 3.0f};
    ezClampValueAttribute minClamp(clamp, {});
    EZ_TEST_BOOL(ezReflectionUtils::ClampValue(variance, &minClamp).Failed());
  }
}
