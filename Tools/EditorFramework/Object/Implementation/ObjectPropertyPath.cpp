#include <PCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezStatus ezObjectPropertyPath::CreatePath(const ezObjectPropertyPathContext& context, const ezPropertyReference& prop,
                                          ezStringBuilder& sObjectSearchSequence, ezStringBuilder& sComponentType,
                                          ezStringBuilder& sPropertyPath)
{
  EZ_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  const ezRTTI* pObjType = ezGetStaticRTTI<ezGameObject>();
  const ezRTTI* pCompType = ezGetStaticRTTI<ezComponent>();

  const ezAbstractProperty* pName = pObjType->FindPropertyByName("Name");
  const ezDocumentObject* pObject = context.m_pAccessor->GetObjectManager()->GetObject(prop.m_Object);
  if (!pObject || !prop.m_pProperty)
    return ezStatus(EZ_FAILURE);

  {
    // Build property part of the path from the next parent node / component.
    pObject = FindParentNodeComponent(pObject);
    if (!pObject)
      return ezStatus("No parent node or component found.");
    ezObjectPropertyPathContext context2 = context;
    context2.m_pContextObject = pObject;
    ezStatus res = CreatePropertyPath(context2, prop, sPropertyPath);
    if (res.Failed())
      return res;
  }

  {
    // Component part
    sComponentType.Clear();
    if (pObject->GetType()->IsDerivedFrom(ezGetStaticRTTI<ezComponent>()))
    {
      sComponentType = pObject->GetType()->GetTypeName();
      pObject = pObject->GetParent();
    }
  }

  // Node path
  while (pObject != context.m_pContextObject)
  {
    if (pObject == nullptr)
    {
      sObjectSearchSequence.Clear();
      sComponentType.Clear();
      sPropertyPath.Clear();
      return ezStatus("Property is not under the given context object, no path exists.");
    }

    if (pObject->GetType() == ezGetStaticRTTI<ezGameObject>())
    {
      ezString sName = context.m_pAccessor->Get<ezString>(pObject, pName);
      if (!sName.IsEmpty())
      {
        if (!sObjectSearchSequence.IsEmpty())
          sObjectSearchSequence.Prepend("/");
        sObjectSearchSequence.Prepend(sName);
      }
    }
    else
    {
      sObjectSearchSequence.Clear();
      sComponentType.Clear();
      sPropertyPath.Clear();
      return ezStatus(
          ezFmt("Only ezGameObject objects should be found in the hierarchy, found '{0}' instead.", pObject->GetType()->GetTypeName()));
    }

    pObject = pObject->GetParent();
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezObjectPropertyPath::CreatePropertyPath(const ezObjectPropertyPathContext& context, const ezPropertyReference& prop,
                                                  ezStringBuilder& out_sPropertyPath)
{
  EZ_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  const ezDocumentObject* pObject = context.m_pAccessor->GetObjectManager()->GetObject(prop.m_Object);
  if (!pObject || !prop.m_pProperty)
    return ezStatus(EZ_FAILURE);

  out_sPropertyPath.Clear();
  ezStatus res = PrependProperty(pObject, prop.m_pProperty, prop.m_Index, out_sPropertyPath);
  if (res.Failed())
    return res;

  while (pObject != context.m_pContextObject)
  {
    ezStatus res = PrependProperty(pObject->GetParent(), pObject->GetParentPropertyType(), pObject->GetPropertyIndex(), out_sPropertyPath);
    if (res.Failed())
      return res;

    pObject = pObject->GetParent();
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezObjectPropertyPath::ResolvePath(const ezObjectPropertyPathContext& context, ezHybridArray<ezPropertyReference, 1>& keys,
                                           const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath)
{
  EZ_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  keys.Clear();
  const ezDocumentObject* pContext = context.m_pContextObject;
  ezDocumentObjectVisitor visitor(context.m_pAccessor->GetObjectManager(), "Children", context.m_sRootProperty);
  ezHybridArray<const ezDocumentObject*, 8> input;
  input.PushBack(pContext);
  ezHybridArray<const ezDocumentObject*, 8> output;

  // Find objects that match the search path
  ezStringBuilder sObjectSearchSequence = szObjectSearchSequence;
  ezHybridArray<ezStringView, 4> names;
  sObjectSearchSequence.Split(false, names, "/");
  for (const ezStringView& sName : names)
  {
    for (const ezDocumentObject* pObj : input)
    {
      visitor.Visit(pContext, false, [&output, &sName](const ezDocumentObject* pObject) -> bool {
        const auto& sObjectName = pObject->GetTypeAccessor().GetValue("Name").Get<ezString>();
        if (sObjectName == sName)
        {
          output.PushBack(pObject);
          return false;
        }
        return true;
      });
    }
    input.Clear();
    input.Swap(output);
  }

  // Test found objects for component
  for (const ezDocumentObject* pObject : input)
  {
    // Could also be the root object in which case we found nothing.
    if (pObject->GetType() == ezGetStaticRTTI<ezGameObject>())
    {
      if (ezStringUtils::IsNullOrEmpty(szComponentType))
      {
        // We are animating the game object directly
        output.PushBack(pObject);
      }
      else
      {
        const ezInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
        for (ezInt32 i = 0; i < iComponents; i++)
        {
          ezVariant value = pObject->GetTypeAccessor().GetValue("Components", i);
          auto pChild = context.m_pAccessor->GetObjectManager()->GetObject(value.Get<ezUuid>());
          if (ezStringUtils::IsEqual(szComponentType, pChild->GetType()->GetTypeName()))
          {
            output.PushBack(pChild);
            continue; //#TODO: break on found component?
          }
        }
      }
    }
  }
  input.Clear();
  input.Swap(output);

  // Test found objects / components for property
  for (const ezDocumentObject* pObject : input)
  {
    ezObjectPropertyPathContext context2 = context;
    context2.m_pContextObject = pObject;
    ezPropertyReference key;
    ezStatus res = ResolvePropertyPath(context2, szPropertyPath, key);
    if (res.Succeeded())
    {
      keys.PushBack(key);
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezObjectPropertyPath::ResolvePropertyPath(const ezObjectPropertyPathContext& context, const char* szPropertyPath,
                                                   ezPropertyReference& out_key)
{
  EZ_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && szPropertyPath != nullptr, "All context fields must be valid.");
  const ezDocumentObject* pObject = context.m_pContextObject;
  ezStringBuilder sPath = szPropertyPath;
  ezHybridArray<ezStringView, 3> parts;
  sPath.Split(false, parts, "/");
  for (ezUInt32 i = 0; i < parts.GetCount(); i++)
  {
    ezStringBuilder sPart = parts[i];
    ezHybridArray<ezStringBuilder, 2> parts2;
    sPart.Split(false, parts2, "[", "]");
    if (parts2.GetCount() == 0 || parts2.GetCount() > 2)
    {
      return ezStatus(ezFmt("Malformed property path part: {0}", sPart));
    }
    ezAbstractProperty* pProperty = pObject->GetType()->FindPropertyByName(parts2[0]);
    if (!pProperty)
      return ezStatus(ezFmt("Property not found: {0}", parts2[0]));
    ezVariant index;
    if (parts2.GetCount() == 2)
    {
      ezInt32 iIndex = 0;
      if (ezConversionUtils::StringToInt(parts2[1], iIndex).Succeeded())
      {
        index = iIndex; // Array index
      }
      else
      {
        index = parts2[1].GetData(); // Map index
      }
    }

    ezVariant value;
    ezStatus res = context.m_pAccessor->GetValue(pObject, pProperty, value, index);
    if (res.Failed())
      return res;

    if (i == parts.GetCount() - 1)
    {
      out_key.m_Object = pObject->GetGuid();
      out_key.m_pProperty = pProperty;
      out_key.m_Index = index;
      return ezStatus(EZ_SUCCESS);
    }
    else
    {
      if (value.IsA<ezUuid>())
      {
        ezUuid id = value.Get<ezUuid>();
        pObject = context.m_pAccessor->GetObjectManager()->GetObject(id);
      }
      else
      {
        return ezStatus(ezFmt("Property '{0}' of type '{1}' is not an object and can't be traversed further.", pProperty->GetPropertyName(),
                              pProperty->GetSpecificType()->GetTypeName()));
      }
    }
  }
  return ezStatus(EZ_FAILURE);
}

ezStatus ezObjectPropertyPath::PrependProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index,
                                               ezStringBuilder& out_sPropertyPath)
{
  switch (pProperty->GetCategory())
  {
    case ezPropertyCategory::Enum::Member:
    {
      if (!out_sPropertyPath.IsEmpty())
        out_sPropertyPath.Prepend("/");
      out_sPropertyPath.Prepend(pProperty->GetPropertyName());
      return ezStatus(EZ_SUCCESS);
    }
    case ezPropertyCategory::Enum::Array:
    case ezPropertyCategory::Enum::Map:
    {
      if (!out_sPropertyPath.IsEmpty())
        out_sPropertyPath.Prepend("/");
      out_sPropertyPath.PrependFormat("{0}[{1}]", pProperty->GetPropertyName(), index);
      return ezStatus(EZ_SUCCESS);
    }
    default:
      return ezStatus(ezFmt("The property '{0}' of category '{1}' which is not supported in property paths", pProperty->GetPropertyName(),
                            pProperty->GetCategory()));
  }
}

const ezDocumentObject* ezObjectPropertyPath::FindParentNodeComponent(const ezDocumentObject* pObject)
{
  const ezRTTI* pObjType = ezGetStaticRTTI<ezGameObject>();
  const ezRTTI* pCompType = ezGetStaticRTTI<ezComponent>();
  const ezDocumentObject* pObj = pObject;
  while (pObj != nullptr)
  {
    if (pObj->GetType() == pObjType)
    {
      return pObj;
    }
    else if (pObj->GetType()->IsDerivedFrom(pCompType))
    {
      return pObj;
    }
    pObj = pObj->GetParent();
  }
  return nullptr;
}
