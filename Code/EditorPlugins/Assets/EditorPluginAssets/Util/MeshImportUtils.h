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

  EZ_EDITORPLUGINASSETS_DLL void SetMeshAssetMaterialSlots(ezDynamicArray<ezMaterialResourceSlot>& inout_materialSlots, const ezModelImporter2::Importer* pImporter);
  EZ_EDITORPLUGINASSETS_DLL void CopyMeshAssetMaterialSlotToResource(ezMeshResourceDescriptor& ref_desc, const ezArrayPtr<ezMaterialResourceSlot>& materialSlots);
  EZ_EDITORPLUGINASSETS_DLL void ImportMeshAssetMaterials(ezDynamicArray<ezMaterialResourceSlot>& inout_materialSlots, ezStringView sDocumentDirectory, const ezModelImporter2::Importer* pImporter);

  /// \brief Extracts external buffer file dependencies from a glTF file and adds them to the transform dependencies set.
  ///
  /// Parses the glTF JSON to find all external buffer files referenced in the "buffers" array.
  /// Skips embedded data URIs and only processes external file references.
  /// All referenced buffer files are converted to data directory relative paths before being added.
  ///
  /// \param sMeshFile Data directory relative path to the glTF file
  /// \param inout_dependencies Set to add the buffer file dependencies to
  EZ_EDITORPLUGINASSETS_DLL void AddGltfBufferDependencies(ezStringView sMeshFile, ezSet<ezString>& inout_dependencies);
} // namespace ezMeshImportUtils
