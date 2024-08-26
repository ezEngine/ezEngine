#include <EditorPluginScene/EditorPluginScenePCH.h>

#include "Foundation/Serialization/GraphPatch.h"
#include <Core/World/GameObject.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentSettingsBase, 1, ezRTTINoAllocator)
{
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPrefabDocumentSettings, 1, ezRTTIDefaultAllocator<ezPrefabDocumentSettings>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("ExposedProperties", m_ExposedProperties),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLayerDocumentSettings, 1, ezRTTIDefaultAllocator<ezLayerDocumentSettings>)
{
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentRoot, 2, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Settings", m_pSettings)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezSceneObjectManager::ezSceneObjectManager()
  : ezDocumentObjectManager(ezGetStaticRTTI<ezSceneDocumentRoot>())
{
}

void ezSceneObjectManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezGameObject>());

  ezRTTI::ForEachDerivedType<ezComponent>(
    [&](const ezRTTI* pRtti)
    { ref_types.PushBack(pRtti); },
    ezRTTI::ForEachOptions::ExcludeAbstract);
}

ezStatus ezSceneObjectManager::InternalCanAdd(
  const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const
{
  if (IsUnderRootProperty("Children", pParent, sParentProperty))
  {
    if (pParent == nullptr)
    {
      bool bIsDerived = pRtti->IsDerivedFrom<ezGameObject>();
      if (!bIsDerived)
      {
        return ezStatus("Only ezGameObject can be added to the root of the world!");
      }
    }
    else
    {
      // only prevent adding game objects (as children) to objects that already have a prefab component
      // do allow to attach components to objects with prefab components
      // if (pRtti->IsDerivedFrom<ezGameObject>())
      //{
      //  if (pParent->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      //  {
      //    auto children = pParent->GetChildren();
      //    for (auto pChild : children)
      //    {
      //      if (pChild->GetType()->IsDerivedFrom<ezPrefabReferenceComponent>())
      //        return ezStatus("Cannot add objects to a prefab node.");
      //    }
      //  }
      //}

      // in case prefab component should be the only component on a node
      // if (pRtti->IsDerivedFrom<ezPrefabReferenceComponent>())
      //{
      //  if (!pParent->GetChildren().IsEmpty())
      //    return ezStatus("Prefab components can only be added to empty nodes.");
      //}
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneObjectManager::InternalCanMove(
  const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, ezStringView sParentProperty, const ezVariant& index) const
{
  // code to disallow attaching nodes to a prefab node
  // if (pNewParent != nullptr)
  //{
  //  if (pNewParent->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
  //  {
  //    auto children = pNewParent->GetChildren();
  //    for (auto pChild : children)
  //    {
  //      if (pChild->GetType()->IsDerivedFrom<ezPrefabReferenceComponent>())
  //        return ezStatus("Cannot move objects into a prefab node.");
  //    }
  //  }
  //}

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneObjectManager::InternalCanSelect(const ezDocumentObject* pObject) const
{
  /*if (pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
  {
    return ezStatus(
      ezFmt("Object of type '{0}' is not a 'ezGameObject' and can't be selected.", pObject->GetTypeAccessor().GetType()->GetTypeName()));
  }*/
  return ezStatus(EZ_SUCCESS);
}

namespace
{
  /// Patch class
  class ezSceneDocumentSettings_1_2 : public ezGraphPatch
  {
  public:
    ezSceneDocumentSettings_1_2()
      : ezGraphPatch("ezSceneDocumentSettings", 2)
    {
    }
    virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
    {
      // Previously, ezSceneDocumentSettings only contained prefab settings. As these only apply to prefab documents, we switch the old version to prefab.
      ref_context.RenameClass("ezPrefabDocumentSettings", 1);
      ezVersionKey bases[] = {{"ezSceneDocumentSettingsBase", 1}, {"ezReflectedClass", 1}};
      ref_context.ChangeBaseClass(bases);
    }
  };
  ezSceneDocumentSettings_1_2 g_ezSceneDocumentSettings_1_2;
} // namespace
