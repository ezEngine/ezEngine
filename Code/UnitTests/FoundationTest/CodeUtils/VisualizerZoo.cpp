#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/MathExpression.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/VarianceTypes.h>

#include <string>

#ifdef __clang__
#  pragma clang optimize off
#endif

class TestRefCounted : public ezRefCounted
{
public:
  ezUInt32 m_uiDummyMember = 0x42u;
};

// declare bitflags using macro magic
EZ_DECLARE_FLAGS(ezUInt32, TestFlags, Bit1, Bit2, Bit3, Bit4);
EZ_DEFINE_AS_POD_TYPE(TestFlags::Enum);

struct TestFlagsManual
{
  using StorageType = ezUInt32;

  enum Enum
  {
    Bit1 = EZ_BIT(0),
    Bit2 = EZ_BIT(1),
    Bit3 = EZ_BIT(2),
    Bit4 = EZ_BIT(3),
    MultiBits = Bit1 | Bit3,
    Default = 0
  };

  struct Bits
  {
    StorageType Bit1 : 1;
    StorageType Bit2 : 1;
    StorageType Bit3 : 1;
    StorageType Bit4 : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(TestFlagsManual);

class ReflectedTest : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ReflectedTest, ezReflectedClass);

public:
  float u;
  float v;
};

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ReflectedTest, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("u", u),
    EZ_MEMBER_PROPERTY("v", v),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


EZ_CREATE_SIMPLE_TEST(CodeUtils, VisualizerZoo)
{
  struct StuffStruct
  {
    int a;
    float b;
    ezString c;
  };
  EZ_TEST_BOOL(true);
  // Strings
  {
    ezString stringEmpty;
    ezString string = u8"こんにちは 世界";
    ezString* stringPtr = &string;
    ezString stringArray[4] = {"AAA", "BBB", "CCC", "DDD"};
    ezStringBuilder stringBuilder = "Test";
    ezStringView stringViewEmpty;
    ezStringView stringView = string.GetSubString(0, 5);
    ezStringIterator stringIteratorEmpty;
    ezStringIterator stringIterator = stringView.GetIteratorFront();
    stringIterator++;
    ezStringReverseIterator stringReverseIteratorEmpty;
    ezStringReverseIterator stringReverseIterator = stringView.GetIteratorBack();
    stringReverseIterator++;

    ezHashedString hashedStringEmpty;
    ezHashedString hashedString = ezMakeHashedString("Test");
    EZ_TEST_BOOL(true);
  }

  // Containers
  {
    ezDynamicArray<ezString> dynamicArray;
    dynamicArray.PushBack("Item1");
    dynamicArray.PushBack("Item2");

    ezHybridArray<StuffStruct, 4> hybridArray;
    hybridArray.PushBack({1, 2.0f, "Item3"});
    hybridArray.PushBack({2, 3.0f, "Item4"});

    ezHybridArray<StuffStruct, 1> hybridArray2;
    hybridArray2.PushBack({1, 2.0f, "Item3"});
    hybridArray2.PushBack({2, 3.0f, "Item4"});
    hybridArray2.PushBack({3, 4.0f, "Item5"});

    ezSmallArray<ezString, 66> smallArray;
    smallArray.PushBack("SmallItem1");
    smallArray.PushBack("SmallItem2");

    ezSmallArray<ezString, 2> smallArray2;
    smallArray2.PushBack("SmallItem1");
    smallArray2.PushBack("SmallItem2");
    smallArray2.PushBack("SmallItem3");
    smallArray2.PushBack("SmallItem4");

    ezStaticArray<float, 2> staticArray;
    staticArray.SetCount(2);
    staticArray[0] = 1.0f;
    staticArray[1] = 2.0f;

    ezHashTable<int, StuffStruct> hashTable;
    hashTable.Insert(1, StuffStruct{3, 4.0f, "HashItem1"});
    hashTable.Insert(99, StuffStruct{3, 4.0f, "HashItem1"});

    ezHashSet<ezString> hashSet;
    hashSet.Insert("HashSetItem1");
    hashSet.Insert("HashSetItem2");

    ezList<ezString> list;
    list.PushBack("ListItem1");
    list.PushBack("ListItem2");

    ezDeque<ezString> deque;
    deque.PushBack("DequeItem1");
    deque.PushBack("DequeItem2");
    deque.PushFront("DequeItem0");

    ezMap<int, ezString> map;
    map.Insert(1, "MapItem1");
    map.Insert(2, "MapItem2");

    ezSet<ezStringView> set;
    set.Insert("SetItem1");
    set.Insert("SetItem2");

    ezStaticRingBuffer<StuffStruct, 4> staticRingBuffer;
    staticRingBuffer.PushBack({5, 6.0f, "StaticRingItem1"});
    staticRingBuffer.PushBack({7, 8.0f, "StaticRingItem2"});
    staticRingBuffer.PushBack({9, 10.0f, "StaticRingItem3"});
    staticRingBuffer.PushBack({11, 12.0f, "StaticRingItem4"});
    staticRingBuffer.PopFront();
    staticRingBuffer.PushBack({13, 14.0f, "StaticRingItem5"});

    ezArrayPtr<ezString> arrayPtr = ezMakeArrayPtr(dynamicArray.GetData(), dynamicArray.GetCount());
    ezArrayPtr<StuffStruct> hybridArrayPtr = ezMakeArrayPtr(hybridArray.GetData(), hybridArray.GetCount());
    EZ_TEST_BOOL(true);
  }

  // ezVariant
  {
    ezVariant variantInvalid; // Default constructor creates Invalid type
    ezVariant variantBool = ezVariant(true);
    ezVariant variantInt8 = ezVariant(static_cast<ezInt8>(42));
    ezVariant variantUInt8 = ezVariant(static_cast<ezUInt8>(42));
    ezVariant variantInt16 = ezVariant(static_cast<ezInt16>(42));
    ezVariant variantUInt16 = ezVariant(static_cast<ezUInt16>(42));
    ezVariant variantInt32 = ezVariant(static_cast<ezInt32>(42));
    ezVariant variantUInt32 = ezVariant(static_cast<ezUInt32>(42));
    ezVariant variantInt64 = ezVariant(static_cast<ezInt64>(42));
    ezVariant variantUInt64 = ezVariant(static_cast<ezUInt64>(42));
    ezVariant variantFloat = ezVariant(42.0f);
    ezVariant variantDouble = ezVariant(42.0);
    ezVariant variantColor = ezVariant(ezColor(1.0f, 0.5f, 0.25f, 1.0f));
    ezVariant variantVector2 = ezVariant(ezVec2(1.0f, 2.0f));
    ezVariant variantVector3 = ezVariant(ezVec3(1.0f, 2.0f, 3.0f));
    ezVariant variantVector4 = ezVariant(ezVec4(1.0f, 2.0f, 3.0f, 4.0f));
    ezVariant variantVector2I = ezVariant(ezVec2I32(1, 2));
    ezVariant variantVector3I = ezVariant(ezVec3I32(1, 2, 3));
    ezVariant variantVector4I = ezVariant(ezVec4I32(1, 2, 3, 4));
    ezVariant variantVector2U = ezVariant(ezVec2U32(1, 2));
    ezVariant variantVector3U = ezVariant(ezVec3U32(1, 2, 3));
    ezVariant variantVector4U = ezVariant(ezVec4U32(1, 2, 3, 4));
    ezVariant variantQuaternion = ezVariant(ezQuat::MakeIdentity());
    ezVariant variantMatrix3 = ezVariant(ezMat3::MakeIdentity());
    ezVariant variantMatrix4 = ezVariant(ezMat4::MakeIdentity());
    ezVariant variantTransform = ezVariant(ezTransform::MakeIdentity());
    ezVariant variantString = ezVariant("SampleString");
    ezVariant variantStringView = ezVariant(ezStringView("SampleStringView"));

    ezDataBuffer dataBuffer;
    dataBuffer.PushBack(1);
    dataBuffer.PushBack(2);
    dataBuffer.PushBack(3);
    ezVariant variantDataBuffer = ezVariant(dataBuffer);

    ezVariant variantTime = ezVariant(ezTime::Seconds(42.0));
    ezVariant variantUuid = ezVariant(ezUuid::MakeStableUuidFromInt(42));
    ezVariant variantAngle = ezVariant(ezAngle::MakeFromDegree(45.0f));
    ezVariant variantColorGamma = ezVariant(ezColorGammaUB(128, 192, 255, 255));
    ezVariant variantHashedString = ezVariant(ezMakeHashedString("HashedSample"));
    ezVariant variantTempHashedString = ezVariant(ezTempHashedString("TempHashedSample"));

    // Extended types
    ezVariantArray varArray;
    varArray.PushBack(ezVariant("Item1"));
    varArray.PushBack(ezVariant(42));
    ezVariant variantVariantArray = ezVariant(varArray);

    ezVariantDictionary varDict;
    varDict.Insert("Key1", ezVariant("Value1"));
    varDict.Insert("Key2", ezVariant(42));
    ezVariant variantVariantDictionary = ezVariant(varDict);

    ReflectedTest test;
    ezVariant variantTypedPointer(&test);
    ezTypedPointer ptr = {nullptr, ezGetStaticRTTI<ReflectedTest>()};
    ezVariant variantTypedPointerNull = ptr;

    ezVarianceTypeAngle value2(ezAngle::MakeFromRadian(1.57079637f), 0.2f);
    ezVariant variantTypedObject(value2);
    EZ_TEST_BOOL(true);
  }

  // Enum
  {
    ezEnum<ezVariantType> enumTest = ezVariantType::Angle;
    ezEnum<ezVariantType> enumArray[3] = {ezVariantType::Int32, ezVariantType::String, ezVariantType::Color};
    ezHybridArray<ezEnum<ezVariantType>, 3> hybridEnumArray;
    hybridEnumArray.PushBack(ezVariantType::Int32);
    hybridEnumArray.PushBack(ezVariantType::String);
    EZ_TEST_BOOL(true);
  }

  // Bitflags
  {
    TestFlags::Enum rawBitflags = TestFlags::Bit1;
    TestFlags::Enum rawBitflags2 = (TestFlags::Enum)(TestFlags::Bit1 | TestFlags::Bit2).GetValue();

    ezBitflags<TestFlagsManual> bitflagsEmpty;
    ezBitflags<TestFlagsManual> bitflags;
    bitflags.Add(TestFlagsManual::Bit1);
    bitflags.Add(TestFlagsManual::Bit3);

    ezBitflags<TestFlags> bitflagsArray[2];
    bitflagsArray[0].Add(TestFlags::Bit3);
    bitflagsArray[1].Add(TestFlags::Bit4);

    ezHybridArray<ezBitflags<TestFlags>, 2> hybridBitflagsArray;
    hybridBitflagsArray.PushBack(TestFlags::Bit1 | TestFlags::Bit2);
    hybridBitflagsArray.PushBack(TestFlags::Bit3 | TestFlags::Bit4);
    EZ_TEST_BOOL(true);
  }

  // Math
  {
    ezVec2 vec2(1.0f, 2.0f);
    ezVec3 vec3(1.0f, 2.0f, 3.0f);
    ezVec4 vec4(1.0f, 2.0f, 3.0f, 4.0f);
    ezVec2I32 vec2I(1, 2);
    ezVec3I32 vec3I(1, 2, 3);
    ezVec4I32 vec4I(1, 2, 3, 4);
    ezVec2U32 vec2U(1, 2);
    ezVec3U32 vec3U(1, 2, 3);
    ezVec4U32 vec4U(1, 2, 3, 4);
    ezQuat quat = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(90.0f));
    ezMat3 mat3 = ezMat3::MakeRotationZ(ezAngle::MakeFromDegree(45.0f));
    ezMat4 mat4 = ezMat4::MakeRotationY(ezAngle::MakeFromDegree(30.0f));
    ezTransform transform(ezVec3(1.0f, 2.0f, 3.0f), quat, ezVec3(1.0f, 1.0f, 1.0f));
    ezAngle angle = ezAngle::MakeFromDegree(60.0f);
    ezPlane plane = ezPlane::MakeFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(0, 0, 0));

    ezColor color(1.0f, 0.5f, 0.25f, 1.0f);
    ezColorGammaUB colorGamma(128, 192, 255, 255);
    ezColorLinearUB colorLinear(128, 192, 255, 255);
    ezTime time = ezTime::Seconds(42.0);

    ezUuid uuid = ezConversionUtils::ConvertStringToUuid("{ 01234567-89AB-CDEF-0123-456789ABCDEF }");
    ezStringBuilder uuidString;
    ezConversionUtils::ToString(uuid, uuidString);
    EZ_TEST_BOOL(true);
  }

  // UniquePtr
  {
    ezUniquePtr<ReflectedTest> uniquePtr = EZ_DEFAULT_NEW(ReflectedTest);
    uniquePtr->u = 1.0f;
    uniquePtr->v = 2.0f;

    ezSharedPtr<TestRefCounted> sharedPtr = EZ_DEFAULT_NEW(TestRefCounted);
    sharedPtr->m_uiDummyMember = 0x42u;

    TestRefCounted testRef;
    ezScopedRefPointer<TestRefCounted> scopedRefPointer(&testRef);
    EZ_TEST_BOOL(true);
  }

  // Mutex
  {
    ezMutex mutex;
    EZ_LOCK(mutex);
    EZ_TEST_BOOL(true);
  }
}
