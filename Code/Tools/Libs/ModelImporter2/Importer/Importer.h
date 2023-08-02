#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter2/ModelImporterDLL.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

class ezLogInterface;
class ezProgress;
class ezEditableSkeleton;
class ezMeshResourceDescriptor;
struct ezAnimationClipResourceDescriptor;

namespace ezModelImporter2
{
  struct ImportOptions
  {
    ezString m_sSourceFile;

    bool m_bImportSkinningData = false;
    bool m_bRecomputeNormals = false;
    bool m_bRecomputeTangents = false;
    bool m_bNormalizeWeights = false;
    ezMat3 m_RootTransform = ezMat3::MakeIdentity();

    ezMeshResourceDescriptor* m_pMeshOutput = nullptr;
    ezEnum<ezMeshNormalPrecision> m_MeshNormalsPrecision = ezMeshNormalPrecision::Default;
    ezEnum<ezMeshTexCoordPrecision> m_MeshTexCoordsPrecision = ezMeshTexCoordPrecision::Default;
    ezEnum<ezMeshBoneWeigthPrecision> m_MeshBoneWeightPrecision = ezMeshBoneWeigthPrecision::Default;

    ezEditableSkeleton* m_pSkeletonOutput = nullptr;

    bool m_bAdditiveAnimation = false;
    ezString m_sAnimationToImport; // empty = first in file; "name" = only anim with that name
    ezAnimationClipResourceDescriptor* m_pAnimationOutput = nullptr;
    ezUInt32 m_uiFirstAnimKeyframe = 0;
    ezUInt32 m_uiNumAnimKeyframes = 0;
  };

  enum class PropertySemantic : ezInt8
  {
    Unknown = 0,

    DiffuseColor,
    RoughnessValue,
    MetallicValue,
    EmissiveColor,
    TwosidedValue,
  };

  enum class TextureSemantic : ezInt8
  {
    Unknown = 0,

    DiffuseMap,
    DiffuseAlphaMap,
    OcclusionMap,
    RoughnessMap,
    MetallicMap,
    OrmMap,
    DisplacementMap,
    NormalMap,
    EmissiveMap,
  };

  struct EZ_MODELIMPORTER2_DLL OutputTexture
  {
    ezString m_sFilename;
    ezString m_sFileFormatExtension;
    ezConstByteArrayPtr m_RawData;

    void GenerateFileName(ezStringBuilder& out_sName) const;
  };

  struct EZ_MODELIMPORTER2_DLL OutputMaterial
  {
    ezString m_sName;

    ezInt32 m_iReferencedByMesh = -1;                     // if -1, no sub-mesh in the output actually references this
    ezMap<TextureSemantic, ezString> m_TextureReferences; // semantic -> path
    ezMap<PropertySemantic, ezVariant> m_Properties;      // semantic -> value
  };

  class EZ_MODELIMPORTER2_DLL Importer
  {
  public:
    Importer();
    virtual ~Importer();

    ezResult Import(const ImportOptions& options, ezLogInterface* pLogInterface = nullptr, ezProgress* pProgress = nullptr);
    const ImportOptions& GetImportOptions() const { return m_Options; }

    ezMap<ezString, OutputTexture> m_OutputTextures; // path -> additional data
    ezDeque<OutputMaterial> m_OutputMaterials;
    ezDynamicArray<ezString> m_OutputAnimationNames;

  protected:
    virtual ezResult DoImport() = 0;

    ImportOptions m_Options;
    ezProgress* m_pProgress = nullptr;
  };

} // namespace ezModelImporter2
