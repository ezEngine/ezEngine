#pragma once

#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

struct ezPropertyMetaStateEvent;

class ezAnimatedMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetProperties, ezReflectedClass);

public:
  ezAnimatedMeshAssetProperties();
  ~ezAnimatedMeshAssetProperties();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezString m_sMeshFile;
  ezString m_sMeshIncludeTags;
  ezString m_sMeshExcludeTags;
  ezString m_sDefaultSkeleton;

  bool m_bRecalculateNormals = false;
  bool m_bRecalculateTrangents = true;
  bool m_bNormalizeWeights = false;
  bool m_bImportMaterials = true;

  ezEnum<ezMeshNormalPrecision> m_NormalPrecision;
  ezEnum<ezMeshTexCoordPrecision> m_TexCoordPrecision;
  ezEnum<ezMeshBoneWeigthPrecision> m_BoneWeightPrecision;
  ezEnum<ezMeshVertexColorConversion> m_VertexColorConversion;

  ezHybridArray<ezMaterialResourceSlot, 8> m_Slots;

  bool m_bSimplifyMesh = false;
  bool m_bAggressiveSimplification = false;
  ezUInt8 m_uiMeshSimplification = 50;
  ezUInt8 m_uiMaxSimplificationError = 5;
};
