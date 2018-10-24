#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

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
  ezString m_sSurface;
};
