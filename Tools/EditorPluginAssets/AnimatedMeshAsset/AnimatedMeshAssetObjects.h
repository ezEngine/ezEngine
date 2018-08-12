#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <EditorPluginAssets/Util/AssetUtils.h>

struct ezPropertyMetaStateEvent;

class ezAnimatedMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetProperties, ezReflectedClass);

public:
  ezAnimatedMeshAssetProperties();

  ezString m_sMeshFile;
  // ezString m_sSubMeshName;
  float m_fUniformScaling;

  ezEnum<ezBasisAxis> m_ForwardDir;
  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;

  bool m_bRecalculateNormals;
  bool m_bInvertNormals;

  bool m_bImportMaterials;
  bool m_bUseSubFolderForImportedMaterials;
  ezHybridArray<ezMaterialResourceSlot, 8> m_Slots;

  const ezString GetResourceSlotProperty(ezUInt32 uiSlot) const;
};
