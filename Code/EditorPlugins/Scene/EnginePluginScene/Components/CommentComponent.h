#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

using ezCommentComponentManager = ezComponentManager<class ezCommentComponent, ezBlockStorageType::Compact>;

/// \brief This component is for adding notes to objects in a scene.
///
/// These comments are solely to explain things to other people that look at the scene or prefab structure.
/// They are not meant for use at runtime. Therefore, all instances of ezCommentComponent are automatically stripped from a scene during export.
class EZ_ENGINEPLUGINSCENE_DLL ezCommentComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCommentComponent, ezComponent, ezCommentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezCommentComponent

public:
  ezCommentComponent();
  ~ezCommentComponent();

  void SetComment(const char* szText);
  const char* GetComment() const;

private:
  ezHashedString m_sComment;
};

//////////////////////////////////////////////////////////////////////////

class EZ_ENGINEPLUGINSCENE_DLL ezSceneExportModifier_RemoveCommentComponents : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_RemoveCommentComponents, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};
