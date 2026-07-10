#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCheckRules.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUnknownTagsAssetCheckRule, 1, ezRTTIDefaultAllocator<ezUnknownTagsAssetCheckRule>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRequiredPropertyAssetCheckRule, 1, ezRTTIDefaultAllocator<ezRequiredPropertyAssetCheckRule>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEmptyGameObjectAssetCheckRule, 1, ezRTTIDefaultAllocator<ezEmptyGameObjectAssetCheckRule>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStringView ezUnknownTagsAssetCheckRule::GetDescription() const
{
  return "Detects tags used on objects that are not registered in the project's tag configuration. "
         "Auto-fix removes these tags.";
}

void ezUnknownTagsAssetCheckRule::CheckProperty(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  // A tag-set property is a Set with the tag-set widget attribute.
  if (pProp->GetCategory() != ezPropertyCategory::Set)
    return;
  if (pProp->GetAttributeByType<ezTagSetWidgetAttribute>() == nullptr)
    return;

  ezObjectAccessorBase* pAcc = ref_ctx.GetObjectAccessor();

  ezHybridArray<ezVariant, 16> indices;
  ezHybridArray<ezVariant, 16> values;
  if (GetPropertyValues(pAcc, pObject, pProp, indices, values).Failed())
    return;

  // Iterate back-to-front so that removals keep the remaining indices valid.
  for (ezUInt32 i = values.GetCount(); i-- > 0;)
  {
    if (!values[i].CanConvertTo<ezString>())
      continue;

    const ezString sTag = values[i].ConvertTo<ezString>();
    if (ezToolsTagRegistry::IsTagKnown(sTag))
      continue;

    const ezString sObj = ezAssetCheckContext::GetObjectDisplayName(pObject);
    ezStringBuilder sMsg;

    if (ref_ctx.IsAutoFixAllowed())
    {
      if (pAcc->RemoveValue(pObject, pProp, indices[i]).Succeeded())
      {
        sMsg.SetFormat("Removed unknown tag '{}' from '{}' (property '{}').", sTag, sObj, pProp->GetPropertyName());
        ref_ctx.ReportIssue(ezAssetCheckSeverity::Warning, sMsg, pObject, /*bFixed*/ true);
      }
      else
      {
        sMsg.SetFormat("Failed to remove unknown tag '{}' from '{}' (property '{}').", sTag, sObj, pProp->GetPropertyName());
        ref_ctx.ReportIssue(ezAssetCheckSeverity::Error, sMsg, pObject, /*bFixed*/ false);
      }
    }
    else
    {
      sMsg.SetFormat("Unknown tag '{}' on '{}' (property '{}').", sTag, sObj, pProp->GetPropertyName());
      ref_ctx.ReportIssue(ezAssetCheckSeverity::Warning, sMsg, pObject, /*bFixed*/ false);
    }
  }
}

ezStringView ezRequiredPropertyAssetCheckRule::GetDescription() const
{
  return "Detects properties marked with ezRequiredAttribute that are empty, or, for game object / "
         "component reference properties, that reference an object which does not exist.";
}

namespace
{
  bool IsEmptyOrInvalidRequiredValue(ezObjectAccessorBase* pAcc, const ezAbstractProperty* pProp, const ezVariant& value)
  {
    if (!value.IsValid() || !value.CanConvertTo<ezString>())
      return true;

    const ezString sValue = value.ConvertTo<ezString>();
    if (sValue.IsEmpty())
      return true;

    // Game object / component reference properties store a stringified ezUuid; make sure it resolves.
    if (pProp->GetAttributeByType<ezGameObjectReferenceAttribute>() != nullptr)
    {
      if (!ezConversionUtils::IsStringUuid(sValue))
        return true;

      const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sValue);
      if (pAcc->GetObject(guid) == nullptr)
        return true;
    }

    return false;
  }

} // namespace

void ezRequiredPropertyAssetCheckRule::CheckProperty(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  if (pProp->GetAttributeByType<ezRequiredAttribute>() == nullptr)
    return;

  ezObjectAccessorBase* pAcc = ref_ctx.GetObjectAccessor();

  ezHybridArray<ezVariant, 16> indices;
  ezHybridArray<ezVariant, 16> values;
  if (GetPropertyValues(pAcc, pObject, pProp, indices, values).Failed())
    return;

  const ezString sObj = ezAssetCheckContext::GetObjectDisplayName(pObject);
  ezStringBuilder sMsg;

  // An empty container has no element to inspect but still fails the requirement.
  if (values.IsEmpty())
  {
    sMsg.SetFormat("Required property '{}' on '{}' is empty.", pProp->GetPropertyName(), sObj);
    ref_ctx.ReportIssue(ezAssetCheckSeverity::Error, sMsg, pObject);
    return;
  }

  for (const ezVariant& value : values)
  {
    if (!IsEmptyOrInvalidRequiredValue(pAcc, pProp, value))
      continue;

    sMsg.SetFormat("Required property '{}' on '{}' is empty.", pProp->GetPropertyName(), sObj);
    ref_ctx.ReportIssue(ezAssetCheckSeverity::Error, sMsg, pObject);
  }
}

ezStringView ezEmptyGameObjectAssetCheckRule::GetDescription() const
{
  return "Detects game objects that have no name, no components and no child objects, and thus have "
         "no effect on the scene. Auto-fix removes them.";
}

bool ezEmptyGameObjectAssetCheckRule::AppliesToDocumentType(ezStringView sDocumentTypeName) const
{
  return sDocumentTypeName == "Scene" || sDocumentTypeName == "Prefab" || sDocumentTypeName == "Layer";
}

void ezEmptyGameObjectAssetCheckRule::CheckDocument(ezAssetCheckContext& ref_ctx)
{
  m_EmptyObjects.Clear();

  // The default traversal (via CheckObject below) only collects candidates; objects are removed
  // afterwards so that removal doesn't invalidate a parent frame's ongoing child iteration.
  ezAssetCheckRule::CheckDocument(ref_ctx);

  ezObjectAccessorBase* pAcc = ref_ctx.GetObjectAccessor();
  ezStringBuilder sMsg;

  for (const ezDocumentObject* pObject : m_EmptyObjects)
  {
    const ezString sObj = ezAssetCheckContext::GetObjectDisplayName(pObject);

    if (ref_ctx.IsAutoFixAllowed())
    {
      if (pAcc->RemoveObject(pObject).Succeeded())
      {
        sMsg.SetFormat("Removed empty game object '{}' (no name, no components, no children).", sObj);
        ref_ctx.ReportIssue(ezAssetCheckSeverity::Warning, sMsg, pObject, /*bFixed*/ true);
      }
      else
      {
        sMsg.SetFormat("Failed to remove empty game object '{}'.", sObj);
        ref_ctx.ReportIssue(ezAssetCheckSeverity::Error, sMsg, pObject, /*bFixed*/ false);
      }
    }
    else
    {
      sMsg.SetFormat("Game object '{}' has no name, no components and no children.", sObj);
      ref_ctx.ReportIssue(ezAssetCheckSeverity::Warning, sMsg, pObject, /*bFixed*/ false);
    }
  }

  m_EmptyObjects.Clear();
}

void ezEmptyGameObjectAssetCheckRule::CheckObject(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject)
{
  if (!pObject->GetType()->IsDerivedFrom<ezGameObject>())
    return;

  bool bHasChildOrComponent = false;
  for (const ezDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentProperty() == "Children" || pChild->GetParentProperty() == "Components")
    {
      bHasChildOrComponent = true;
      break;
    }
  }

  if (bHasChildOrComponent)
    return;

  ezVariant name;
  if (ref_ctx.GetObjectAccessor()->GetValueByName(pObject, "Name", name).Succeeded() && name.CanConvertTo<ezString>() &&
      !name.ConvertTo<ezString>().IsEmpty())
    return;

  m_EmptyObjects.PushBack(pObject);
}
