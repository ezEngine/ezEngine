#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct ezPropertyMetaStateEvent;

class ezAnimatedMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetProperties, ezReflectedClass);

public:
  ezAnimatedMeshAssetProperties();

  ezString m_sSkeletonFile;
  ezString m_sMeshFile;
  float m_fUniformScaling;

  ezEnum<ezBasisAxis> m_ForwardDir;
  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;

  bool m_bRecalculateNormals;
  bool m_bInvertNormals;

  ezEnum<ezMeshNormalPrecision> m_NormalPrecision;
  ezEnum<ezMeshTexCoordPrecision> m_TexCoordPrecision;

  bool m_bImportMaterials;
  bool m_bUseSubFolderForImportedMaterials;
  ezHybridArray<ezMaterialResourceSlot, 8> m_Slots;
};
