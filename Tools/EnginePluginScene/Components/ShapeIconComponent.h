#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

typedef ezComponentManager<class ezShapeIconComponent, ezBlockStorageType::Compact> ezShapeIconComponentManager;

/// \brief This is a dummy component that the editor creates on all 'empty' nodes for the sole purpose to render a shape icon and enable picking.
///
/// Though in the future one could potentially use them for other editor functionality, such as displaying the object name or some other useful text.
class EZ_ENGINEPLUGINSCENE_DLL ezShapeIconComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezShapeIconComponent, ezComponent, ezShapeIconComponentManager);

public:
  ezShapeIconComponent();
  ~ezShapeIconComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

};

class EZ_ENGINEPLUGINSCENE_DLL ezSceneExportModifier_RemoveShapeIconComponents : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_RemoveShapeIconComponents, ezSceneExportModifier);
public:

  virtual void ModifyWorld(ezWorld& world, const ezUuid& documentGuid) override;
};

