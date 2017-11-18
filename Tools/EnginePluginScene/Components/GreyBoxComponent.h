#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

typedef ezComponentManager<class ezGreyBoxComponent, ezBlockStorageType::Compact> ezGreyBoxComponentManager;

struct EZ_ENGINEPLUGINSCENE_DLL ezGreyBoxShape
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Box,

    Default = Box
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_ENGINEPLUGINSCENE_DLL, ezGreyBoxShape)

class EZ_ENGINEPLUGINSCENE_DLL ezGreyBoxComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGreyBoxComponent, ezComponent, ezGreyBoxComponentManager);

public:
  ezGreyBoxComponent();
  ~ezGreyBoxComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  ezEnum<ezGreyBoxShape> m_Shape;
  float m_fSizeNegX = 0;
  float m_fSizePosX = 0;
  float m_fSizeNegY = 0;
  float m_fSizePosY = 0;
  float m_fSizeNegZ = 0;
  float m_fSizePosZ = 0;
};

class EZ_ENGINEPLUGINSCENE_DLL ezSceneExportModifier_ConvertGreyBoxComponents : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_ConvertGreyBoxComponents, ezSceneExportModifier);
public:

  virtual void ModifyWorld(ezWorld& world) override;
};

