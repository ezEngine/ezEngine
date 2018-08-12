#pragma once

#include <EditorPluginAssets/Plugin.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <ModelImporter/Declarations.h>
#include <Foundation/Types/SharedPtr.h>

class ezMaterialAssetDocument;
class ezMeshResourceDescriptor;
class ezProgressRange;

namespace ezMeshImportUtils
{
  ezString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, const char* szTexturePath,
                                  ezModelImporter::SemanticHint::Enum hint);

  void ImportMaterial(ezMaterialAssetDocument* materialDocument, const ezModelImporter::Material* material,
                      const char* szImportSourceFolder, const char* szImportTargetFolder);

  void ImportMeshMaterials(const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                           ezHybridArray<ezMaterialResourceSlot, 8>& inout_MaterialSlots, const char* szImportSourceFolder,
                           const char* szImportTargetFolder);

  void ImportMeshAssetMaterials(const char* szAssetDocument, const char* szMeshFile, bool bUseSubFolderForImportedMaterials,
                                const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                                ezHybridArray<ezMaterialResourceSlot, 8>& inout_MaterialSlots);

  const ezString GetResourceSlotProperty(const ezHybridArray<ezMaterialResourceSlot, 8>& materialSlots, ezUInt32 uiSlot);

  void AddMeshToDescriptor(ezMeshResourceDescriptor& meshDescriptor, const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                           const ezHybridArray<ezMaterialResourceSlot, 8>& materialSlots);

  void UpdateMaterialSlots(const char* szDocumentPath, const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                           bool bImportMaterials, bool bUseSubFolderForImportedMaterials, const char* szMeshFile,
                           ezHybridArray<ezMaterialResourceSlot, 8>& inout_MaterialSlots);

  void PrepareMeshForImport(ezModelImporter::Mesh& mesh, bool bRecalculateNormals, ezProgressRange& range);

  ezStatus GenerateMeshBuffer(const ezModelImporter::Mesh& mesh, ezMeshResourceDescriptor& meshDescriptor, const ezMat3& mTransformation,
                              bool bInvertNormals, bool bSkinnedMesh);

  ezStatus TryImportMesh(ezSharedPtr<ezModelImporter::Scene>& out_pScene, ezModelImporter::Mesh*& out_pMesh, const char* szMeshFile,
                         const char* szSubMeshName, const ezMat3& mMeshTransform, bool bRecalculateNormals, bool bInvertNormals,
                         ezProgressRange& range, ezMeshResourceDescriptor& meshDescriptor, bool bSkinnedMesh);
}
