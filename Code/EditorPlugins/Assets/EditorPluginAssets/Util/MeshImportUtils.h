#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

class ezMeshResourceDescriptor;

namespace ezModelImporter2
{
  class Importer;
  enum class TextureSemantic : ezInt8;
} // namespace ezModelImporter2

namespace ezMeshImportUtils
{
  EZ_EDITORPLUGINASSETS_DLL ezString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, ezStringView sTexturePath, ezModelImporter2::TextureSemantic hint, bool bTextureClamp, const ezModelImporter2::Importer* pImporter);

  EZ_EDITORPLUGINASSETS_DLL void SetMeshAssetMaterialSlots(ezHybridArray<ezMaterialResourceSlot, 8>& inout_materialSlots, const ezModelImporter2::Importer* pImporter);
  EZ_EDITORPLUGINASSETS_DLL void CopyMeshAssetMaterialSlotToResource(ezMeshResourceDescriptor& ref_desc, const ezHybridArray<ezMaterialResourceSlot, 8>& materialSlots);
  EZ_EDITORPLUGINASSETS_DLL void ImportMeshAssetMaterials(ezHybridArray<ezMaterialResourceSlot, 8>& inout_materialSlots, ezStringView sDocumentDirectory, const ezModelImporter2::Importer* pImporter);
} // namespace ezMeshImportUtils
