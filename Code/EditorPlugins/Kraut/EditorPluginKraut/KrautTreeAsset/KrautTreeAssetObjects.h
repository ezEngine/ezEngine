#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct ezKrautAssetMaterial
{
  ezString m_sLabel;
  ezString m_sMaterial;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautAssetMaterial);

class ezKrautTreeAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetProperties, ezReflectedClass);

public:
  ezKrautTreeAssetProperties();
  ~ezKrautTreeAssetProperties();

  ezString m_sKrautFile;
  float m_fUniformScaling = 1.0f;
  float m_fLodDistanceScale = 1.0f;
  float m_fStaticColliderRadius = 0.4f;
  float m_fTreeStiffness = 10.0f;
  ezString m_sSurface;

  ezHybridArray<ezKrautAssetMaterial, 8> m_Materials;

  ezUInt16 m_uiRandomSeedForDisplay = 0;

  ezHybridArray<ezUInt16, 16> m_GoodRandomSeeds;
};
