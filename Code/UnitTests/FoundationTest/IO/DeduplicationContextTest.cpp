#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  struct RefCountedVec3 : public ezRefCounted
  {
    RefCountedVec3() = default;
    RefCountedVec3(const ezVec3& v)
      : m_v(v)
    {
    }

    ezResult Serialize(ezStreamWriter& inout_stream) const
    {
      inout_stream << m_v;
      return EZ_SUCCESS;
    }

    ezResult Deserialize(ezStreamReader& inout_stream)
    {
      inout_stream >> m_v;
      return EZ_SUCCESS;
    }

    ezVec3 m_v;
  };

  struct ComplexComponent
  {
    ezTransform* m_pTransform = nullptr;
    ezVec3* m_pPosition = nullptr;
    ezSharedPtr<RefCountedVec3> m_pScale;
    ezUInt32 m_uiIndex = ezInvalidIndex;

    ezResult Serialize(ezStreamWriter& inout_stream) const
    {
      EZ_SUCCEED_OR_RETURN(ezDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pTransform));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pPosition));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pScale));

      inout_stream << m_uiIndex;
      return EZ_SUCCESS;
    }

    ezResult Deserialize(ezStreamReader& inout_stream)
    {
      EZ_SUCCEED_OR_RETURN(ezDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pTransform));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pPosition));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pScale));

      inout_stream >> m_uiIndex;
      return EZ_SUCCESS;
    }
  };

  struct ComplexObject
  {
    ezDynamicArray<ezUniquePtr<ezTransform>> m_Transforms;
    ezDynamicArray<ezVec3> m_Positions;
    ezDynamicArray<ezSharedPtr<RefCountedVec3>> m_Scales;

    ezDynamicArray<ComplexComponent> m_Components;

    ezMap<ezUInt32, ezTransform*> m_TransformMap;
    ezSet<ezVec3*> m_UniquePositions;

    ezResult Serialize(ezStreamWriter& inout_stream) const
    {
      EZ_SUCCEED_OR_RETURN(ezDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Transforms));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Positions));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Scales));
      EZ_SUCCEED_OR_RETURN(
        ezDeduplicationWriteContext::GetContext()->WriteMap(inout_stream, m_TransformMap, ezDeduplicationWriteContext::WriteMapMode::DedupValue));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationWriteContext::GetContext()->WriteSet(inout_stream, m_UniquePositions));
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Components));
      return EZ_SUCCESS;
    }

    ezResult Deserialize(ezStreamReader& inout_stream)
    {
      EZ_SUCCEED_OR_RETURN(ezDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Transforms));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Positions,
        nullptr));                                                                                                       // should not allocate anything
      EZ_SUCCEED_OR_RETURN(ezDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Scales));
      EZ_SUCCEED_OR_RETURN(ezDeduplicationReadContext::GetContext()->ReadMap(
        inout_stream, m_TransformMap, ezDeduplicationReadContext::ReadMapMode::DedupValue, nullptr, nullptr));           // should not allocate anything
      EZ_SUCCEED_OR_RETURN(ezDeduplicationReadContext::GetContext()->ReadSet(inout_stream, m_UniquePositions, nullptr)); // should not allocate anything
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Components));
      return EZ_SUCCESS;
    }
  };
} // namespace

EZ_CREATE_SIMPLE_TEST(IO, DeduplicationContext)
{
  ezDefaultMemoryStreamStorage streamStorage;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Writer")
  {
    ezMemoryStreamWriter writer(&streamStorage);

    ezDeduplicationWriteContext dedupWriteContext;

    ComplexObject obj;
    for (ezUInt32 i = 0; i < 20; ++i)
    {
      obj.m_Transforms.ExpandAndGetRef() = EZ_DEFAULT_NEW(ezTransform, ezVec3(static_cast<float>(i), 0, 0));
      obj.m_Positions.ExpandAndGetRef() = ezVec3(1, 2, static_cast<float>(i));
      obj.m_Scales.ExpandAndGetRef() = EZ_DEFAULT_NEW(RefCountedVec3, ezVec3(0, static_cast<float>(i), 0));
    }

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      auto& component = obj.m_Components.ExpandAndGetRef();
      component.m_uiIndex = i * 2;
      component.m_pTransform = obj.m_Transforms[component.m_uiIndex].Borrow();
      component.m_pPosition = &obj.m_Positions[component.m_uiIndex];
      component.m_pScale = obj.m_Scales[component.m_uiIndex];

      obj.m_TransformMap.Insert(i, obj.m_Transforms[i].Borrow());
      obj.m_UniquePositions.Insert(&obj.m_Positions[i]);
    }



    EZ_TEST_BOOL(obj.Serialize(writer).Succeeded());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reader")
  {
    ezMemoryStreamReader reader(&streamStorage);

    ezDeduplicationReadContext dedupReadContext;

    ComplexObject obj;
    EZ_TEST_BOOL(obj.Deserialize(reader).Succeeded());

    EZ_TEST_INT(obj.m_Transforms.GetCount(), 20);
    EZ_TEST_INT(obj.m_Positions.GetCount(), 20);
    EZ_TEST_INT(obj.m_Scales.GetCount(), 20);
    EZ_TEST_INT(obj.m_TransformMap.GetCount(), 10);
    EZ_TEST_INT(obj.m_UniquePositions.GetCount(), 10);
    EZ_TEST_INT(obj.m_Components.GetCount(), 10);

    for (ezUInt32 i = 0; i < obj.m_Components.GetCount(); ++i)
    {
      auto& component = obj.m_Components[i];

      EZ_TEST_BOOL(component.m_pTransform == obj.m_Transforms[component.m_uiIndex].Borrow());
      EZ_TEST_BOOL(component.m_pPosition == &obj.m_Positions[component.m_uiIndex]);
      EZ_TEST_BOOL(component.m_pScale == obj.m_Scales[component.m_uiIndex]);

      EZ_TEST_BOOL(component.m_pTransform->m_vPosition == ezVec3(static_cast<float>(i) * 2, 0, 0));
      EZ_TEST_BOOL(*component.m_pPosition == ezVec3(1, 2, static_cast<float>(i) * 2));
      EZ_TEST_BOOL(component.m_pScale->m_v == ezVec3(0, static_cast<float>(i) * 2, 0));
    }

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      if (EZ_TEST_BOOL(obj.m_TransformMap.GetValue(i) != nullptr))
      {
        EZ_TEST_BOOL(*obj.m_TransformMap.GetValue(i) == obj.m_Transforms[i].Borrow());
      }

      EZ_TEST_BOOL(obj.m_UniquePositions.Contains(&obj.m_Positions[i]));
    }
  }
}
