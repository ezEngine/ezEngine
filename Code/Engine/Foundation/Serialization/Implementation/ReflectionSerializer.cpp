#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

////////////////////////////////////////////////////////////////////////
// ezReflectionSerializer public static functions
////////////////////////////////////////////////////////////////////////

void ezReflectionSerializer::WriteObjectToDDL(ezStreamWriter& inout_stream, const ezRTTI* pRtti, const void* pObject, bool bCompactMmode /*= true*/, ezOpenDdlWriter::TypeStringMode typeMode /*= ezOpenDdlWriter::TypeStringMode::Shortest*/)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter conv(&graph, &context, false, true);

  context.RegisterObject(ezUuid::MakeUuid(), pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  ezAbstractGraphDdlSerializer::Write(inout_stream, &graph, nullptr, bCompactMmode, typeMode);
}

void ezReflectionSerializer::WriteObjectToDDL(ezOpenDdlWriter& ref_ddl, const ezRTTI* pRtti, const void* pObject, ezUuid guid /*= ezUuid()*/)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter conv(&graph, &context, false, true);

  if (!guid.IsValid())
  {
    guid = ezUuid::MakeUuid();
  }

  context.RegisterObject(guid, pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  ezAbstractGraphDdlSerializer::Write(ref_ddl, &graph, nullptr);
}

void ezReflectionSerializer::WriteObjectToBinary(ezStreamWriter& inout_stream, const ezRTTI* pRtti, const void* pObject)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter conv(&graph, &context, false, true);

  context.RegisterObject(ezUuid::MakeUuid(), pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  ezAbstractGraphBinarySerializer::Write(inout_stream, &graph);
}

void* ezReflectionSerializer::ReadObjectFromDDL(ezStreamReader& inout_stream, const ezRTTI*& ref_pRtti)
{
  ezOpenDdlReader reader;
  if (reader.ParseDocument(inout_stream, 0, ezLog::GetThreadLocalLogSystem()).Failed())
  {
    ezLog::Error("Failed to parse DDL graph");
    return nullptr;
  }

  return ReadObjectFromDDL(reader.GetRootElement(), ref_pRtti);
}

void* ezReflectionSerializer::ReadObjectFromDDL(const ezOpenDdlReaderElement* pRootElement, const ezRTTI*& ref_pRtti)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphDdlSerializer::Read(pRootElement, &graph).IgnoreResult();

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  ref_pRtti = ezRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), ref_pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, ref_pRtti, pTarget);

  return pTarget;
}

void* ezReflectionSerializer::ReadObjectFromBinary(ezStreamReader& inout_stream, const ezRTTI*& ref_pRtti)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphBinarySerializer::Read(inout_stream, &graph);

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  ref_pRtti = ezRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), ref_pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, ref_pRtti, pTarget);

  return pTarget;
}

void ezReflectionSerializer::ReadObjectPropertiesFromDDL(ezStreamReader& inout_stream, const ezRTTI& rtti, void* pObject)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphDdlSerializer::Read(inout_stream, &graph).IgnoreResult();

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  if (pRootNode == nullptr)
    return;

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}

void ezReflectionSerializer::ReadObjectPropertiesFromBinary(ezStreamReader& inout_stream, const ezRTTI& rtti, void* pObject)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphBinarySerializer::Read(inout_stream, &graph);

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}


namespace
{
  static void CloneProperty(const void* pObject, void* pClone, const ezAbstractProperty* pProp)
  {
    if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
      return;

    const ezRTTI* pPropType = pProp->GetSpecificType();

    const bool bIsValueType = ezReflectionUtils::IsValueType(pProp);

    ezVariant vTemp;
    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
      {
        auto pSpecific = static_cast<const ezAbstractMemberProperty*>(pProp);

        if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          vTemp = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);

          void* pRefrencedObject = vTemp.ConvertTo<void*>();
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner) && pRefrencedObject)
          {
            pRefrencedObject = ezReflectionSerializer::Clone(pRefrencedObject, pPropType);
            vTemp = ezVariant(pRefrencedObject, pPropType);
          }

          ezVariant vOldValue = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pClone);
          ezReflectionUtils::SetMemberPropertyValue(pSpecific, pClone, vTemp);
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
            ezReflectionUtils::DeleteObject(vOldValue.ConvertTo<void*>(), pProp);
        }
        else
        {
          if (bIsValueType || pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
          {
            vTemp = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
            ezReflectionUtils::SetMemberPropertyValue(pSpecific, pClone, vTemp);
          }
          else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
          {
            void* pSubObject = pSpecific->GetPropertyPointer(pObject);
            // Do we have direct access to the property?
            if (pSubObject != nullptr)
            {
              void* pSubClone = pSpecific->GetPropertyPointer(pClone);
              ezReflectionSerializer::Clone(pSubObject, pSubClone, pPropType);
            }
            // If the property is behind an accessor, we need to retrieve it first.
            else if (pPropType->GetAllocator()->CanAllocate())
            {
              pSubObject = pPropType->GetAllocator()->Allocate<void>();
              pSpecific->GetValuePtr(pObject, pSubObject);
              pSpecific->SetValuePtr(pClone, pSubObject);
              pPropType->GetAllocator()->Deallocate(pSubObject);
            }
          }
        }
      }
      break;
      case ezPropertyCategory::Array:
      {
        auto pSpecific = static_cast<const ezAbstractArrayProperty*>(pProp);
        // Delete old values
        if (pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
        {
          const ezInt32 iCloneCount = (ezInt32)pSpecific->GetCount(pClone);
          for (ezInt32 i = iCloneCount - 1; i >= 0; --i)
          {
            void* pOldSubClone = nullptr;
            pSpecific->GetValue(pClone, i, &pOldSubClone);
            pSpecific->Remove(pClone, i);
            if (pOldSubClone)
              ezReflectionUtils::DeleteObject(pOldSubClone, pProp);
          }
        }

        const ezUInt32 uiCount = pSpecific->GetCount(pObject);
        pSpecific->SetCount(pClone, uiCount);
        if (pSpecific->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          for (ezUInt32 i = 0; i < uiCount; ++i)
          {
            vTemp = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
            void* pRefrencedObject = vTemp.ConvertTo<void*>();
            if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner) && pRefrencedObject)
            {
              pRefrencedObject = ezReflectionSerializer::Clone(pRefrencedObject, pPropType);
              vTemp = ezVariant(pRefrencedObject, pPropType);
            }
            ezReflectionUtils::SetArrayPropertyValue(pSpecific, pClone, i, vTemp);
          }
        }
        else
        {
          if (bIsValueType)
          {
            for (ezUInt32 i = 0; i < uiCount; ++i)
            {
              vTemp = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
              ezReflectionUtils::SetArrayPropertyValue(pSpecific, pClone, i, vTemp);
            }
          }
          else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
          {
            void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

            for (ezUInt32 i = 0; i < uiCount; ++i)
            {
              pSpecific->GetValue(pObject, i, pSubObject);
              pSpecific->SetValue(pClone, i, pSubObject);
            }

            pPropType->GetAllocator()->Deallocate(pSubObject);
          }
        }
      }
      break;
      case ezPropertyCategory::Set:
      {
        auto pSpecific = static_cast<const ezAbstractSetProperty*>(pProp);

        // Delete old values
        if (pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
        {
          ezHybridArray<ezVariant, 16> keys;
          pSpecific->GetValues(pClone, keys);
          pSpecific->Clear(pClone);
          for (ezVariant& value : keys)
          {
            void* pOldClone = value.ConvertTo<void*>();
            if (pOldClone)
              ezReflectionUtils::DeleteObject(pOldClone, pProp);
          }
        }
        pSpecific->Clear(pClone);

        ezHybridArray<ezVariant, 16> values;
        pSpecific->GetValues(pObject, values);


        if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          for (ezUInt32 i = 0; i < values.GetCount(); ++i)
          {
            void* pRefrencedObject = values[i].ConvertTo<void*>();
            if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner) && pRefrencedObject)
            {
              pRefrencedObject = ezReflectionSerializer::Clone(pRefrencedObject, pPropType);
            }
            vTemp = ezVariant(pRefrencedObject, pPropType);
            ezReflectionUtils::InsertSetPropertyValue(pSpecific, pClone, vTemp);
          }
        }
        else if (bIsValueType)
        {
          for (ezUInt32 i = 0; i < values.GetCount(); ++i)
          {
            ezReflectionUtils::InsertSetPropertyValue(pSpecific, pClone, values[i]);
          }
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        auto pSpecific = static_cast<const ezAbstractMapProperty*>(pProp);

        // Delete old values
        if (pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
        {
          ezHybridArray<ezString, 16> keys;
          pSpecific->GetKeys(pClone, keys);
          for (const ezString& sKey : keys)
          {
            ezVariant value = ezReflectionUtils::GetMapPropertyValue(pSpecific, pClone, sKey);
            void* pOldClone = value.ConvertTo<void*>();
            pSpecific->Remove(pClone, sKey);
            if (pOldClone)
              ezReflectionUtils::DeleteObject(pOldClone, pProp);
          }
        }
        pSpecific->Clear(pClone);

        ezHybridArray<ezString, 16> keys;
        pSpecific->GetKeys(pObject, keys);

        for (ezUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          if (bIsValueType ||
              (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner)))
          {
            ezVariant value = ezReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
            ezReflectionUtils::SetMapPropertyValue(pSpecific, pClone, keys[i], value);
          }
          else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
          {
            if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
            {
              void* pValue = nullptr;
              pSpecific->GetValue(pObject, keys[i], &pValue);
              pValue = ezReflectionSerializer::Clone(pValue, pPropType);
              pSpecific->Insert(pClone, keys[i], &pValue);
            }
            else
            {
              if (pPropType->GetAllocator()->CanAllocate())
              {
                void* pValue = pPropType->GetAllocator()->Allocate<void>();
                EZ_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(pValue););
                EZ_VERIFY(pSpecific->GetValue(pObject, keys[i], pValue), "Previously retrieved key does not exist.");
                pSpecific->Insert(pClone, keys[i], pValue);
              }
              else
              {
                ezLog::Error("The property '{0}' can not be cloned as the type '{1}' cannot be allocated.", pProp->GetPropertyName(), pPropType->GetTypeName());
              }
            }
          }
        }
      }
      break;
      default:
        break;
    }
  }

  static void CloneProperties(const void* pObject, void* pClone, const ezRTTI* pType)
  {
    if (pType->GetParentType())
      CloneProperties(pObject, pClone, pType->GetParentType());

    for (auto* pProp : pType->GetProperties())
    {
      CloneProperty(pObject, pClone, pProp);
    }
  }
} // namespace

void* ezReflectionSerializer::Clone(const void* pObject, const ezRTTI* pType)
{
  if (!pObject)
    return nullptr;

  EZ_ASSERT_DEV(pType != nullptr, "invalid type.");
  if (pType->IsDerivedFrom<ezReflectedClass>())
  {
    const ezReflectedClass* pRefObject = static_cast<const ezReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
  }

  EZ_ASSERT_DEV(pType->GetAllocator()->CanAllocate(), "The type '{0}' can't be cloned!", pType->GetTypeName());
  void* pClone = pType->GetAllocator()->Allocate<void>();
  CloneProperties(pObject, pClone, pType);
  return pClone;
}


void ezReflectionSerializer::Clone(const void* pObject, void* pClone, const ezRTTI* pType)
{
  EZ_ASSERT_DEV(pObject && pClone && pType, "invalid type.");
  if (pType->IsDerivedFrom<ezReflectedClass>())
  {
    const ezReflectedClass* pRefObject = static_cast<const ezReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
    EZ_ASSERT_DEV(pType == static_cast<ezReflectedClass*>(pClone)->GetDynamicRTTI(), "Object '{0}' and clone '{1}' have mismatching types!", pType->GetTypeName(), static_cast<ezReflectedClass*>(pClone)->GetDynamicRTTI()->GetTypeName());
  }

  CloneProperties(pObject, pClone, pType);
}


