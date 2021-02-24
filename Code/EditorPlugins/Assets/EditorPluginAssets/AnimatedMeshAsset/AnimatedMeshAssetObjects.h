#pragma once

#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

class ezAnimatedMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetProperties, ezReflectedClass);

public:
  ezAnimatedMeshAssetProperties();
  ~ezAnimatedMeshAssetProperties();

  ezString m_sMeshFile;
  float m_fUniformScaling = 1.0f;

  ezString m_sDefaultSkeleton;

  //ezEnum<ezBasisAxis> m_RightDir = ezBasisAxis::PositiveY;
  //ezEnum<ezBasisAxis> m_UpDir = ezBasisAxis::PositiveZ;
  //bool m_bFlipForwardDir = false;

  bool m_bRecalculateNormals = false;
  bool m_bRecalculateTrangents = true;
  bool m_bImportMaterials = true;

  ezEnum<ezMeshNormalPrecision> m_NormalPrecision;
  ezEnum<ezMeshTexCoordPrecision> m_TexCoordPrecision;

  ezHybridArray<ezMaterialResourceSlot, 8> m_Slots;
};
