#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>


ezApplyNativePropertyChangesContext::ezApplyNativePropertyChangesContext(ezRttiConverterContext& ref_source, const ezAbstractObjectGraph& originalGraph)
  : m_NativeContext(ref_source)
  , m_OriginalGraph(originalGraph)
{
}

ezUuid ezApplyNativePropertyChangesContext::GenerateObjectGuid(const ezUuid& parentGuid, const ezAbstractProperty* pProp, ezVariant index, void* pObject) const
{
  if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
  {
    // If the object is already known by the native context (a pointer that existed before the native changes)
    // we can just return it. Any other pointer will get a new guid assigned.
    ezUuid guid = m_NativeContext.GetObjectGUID(pProp->GetSpecificType(), pObject);
    if (guid.IsValid())
      return guid;
  }
  else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
  {
    // In case of by-value classes we lookup the guid in the object manager graph by using
    // the index as the identify of the object. If the index is not valid (e.g. the array was expanded by native changes)
    // a new guid is assigned.
    if (const ezAbstractObjectNode* originalNode = m_OriginalGraph.GetNode(parentGuid))
    {
      if (const ezAbstractObjectNode::Property* originalProp = originalNode->FindProperty(pProp->GetPropertyName()))
      {
        switch (pProp->GetCategory())
        {
          case ezPropertyCategory::Member:
          {
            if (originalProp->m_Value.IsA<ezUuid>() && originalProp->m_Value.Get<ezUuid>().IsValid())
              return originalProp->m_Value.Get<ezUuid>();
          }
          break;
          case ezPropertyCategory::Array:
          {
            ezUInt32 uiIndex = index.Get<ezUInt32>();
            if (originalProp->m_Value.IsA<ezVariantArray>())
            {
              const ezVariantArray& values = originalProp->m_Value.Get<ezVariantArray>();
              if (uiIndex < values.GetCount())
              {
                const auto& originalElemValue = values[uiIndex];
                if (originalElemValue.IsA<ezUuid>() && originalElemValue.Get<ezUuid>().IsValid())
                  return originalElemValue.Get<ezUuid>();
              }
            }
          }
          break;
          case ezPropertyCategory::Map:
          {
            const ezString& sIndex = index.Get<ezString>();
            if (originalProp->m_Value.IsA<ezVariantDictionary>())
            {
              const ezVariantDictionary& values = originalProp->m_Value.Get<ezVariantDictionary>();
              if (values.Contains(sIndex))
              {
                const auto& originalElemValue = *values.GetValue(sIndex);
                if (originalElemValue.IsA<ezUuid>() && originalElemValue.Get<ezUuid>().IsValid())
                  return originalElemValue.Get<ezUuid>();
              }
            }
          }
          break;

          default:
            break;
        }
      }
    }
  }

  return ezUuid::MakeUuid();
}


