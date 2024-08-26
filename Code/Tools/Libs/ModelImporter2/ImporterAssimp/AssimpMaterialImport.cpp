#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>
#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/texture.h>

namespace ezModelImporter2
{
  static const void MakeValidMaterialName(ezString& ref_sTarget, const char* szSource, ezUInt32 uiMatIdx, ezSet<ezString>& ref_knownMaterialNames)
  {
    ezStringBuilder tmp;
    ezPathUtils::MakeValidFilename(szSource, '_', tmp);

    if (ref_knownMaterialNames.Contains(tmp))
    {
      if (!tmp.IsEmpty())
        tmp.Prepend("-");

      tmp.PrependFormat("Mat-{}", uiMatIdx);
    }

    ref_sTarget = tmp;
    ref_knownMaterialNames.Insert(ref_sTarget);
  }

  template <typename assimpType>
  static void TryReadAssimpProperty(ezMap<PropertySemantic, ezVariant>& inout_properties, PropertySemantic targetSemantic, const aiMaterial& assimpMaterial, const char* szKey, ezUInt32 uiType, ezUInt32 uiIdx, bool bInvert = false)
  {
    assimpType value;
    if (assimpMaterial.Get(szKey, uiType, uiIdx, value) == AI_SUCCESS)
    {
      inout_properties[targetSemantic] = ConvertAssimpType(value, bInvert);
    }
  }

  void TryReadAssimpTextures(ezMap<TextureSemantic, ezString>& out_textures, aiTextureType type, TextureSemantic targetSemantic, const aiMaterial& assimpMaterial)
  {
    // there could be multiple textures of this type, but we can only handle one
    aiString path;
    if (assimpMaterial.GetTexture(type, 0, &path) == AI_SUCCESS)
    {
      out_textures[targetSemantic] = path.C_Str();
    }
  }

  ezResult ImporterAssimp::ImportMaterials()
  {
    if (!m_pScene->HasMaterials())
      return EZ_SUCCESS;

    ezSet<ezString> knownMaterialNames;
    knownMaterialNames.Insert("");

    for (ezUInt32 matIdx = 0; matIdx < m_pScene->mNumMaterials; ++matIdx)
    {
      aiMaterial* pMat = m_pScene->mMaterials[matIdx];

      auto& outMaterial = m_OutputMaterials.ExpandAndGetRef();
      MakeValidMaterialName(outMaterial.m_sName, pMat->GetName().C_Str(), matIdx, knownMaterialNames);

      auto& mp = outMaterial.m_Properties;
      auto& tr = outMaterial.m_TextureReferences;

      TryReadAssimpProperty<aiColor3D>(mp, PropertySemantic::DiffuseColor, *pMat, AI_MATKEY_COLOR_DIFFUSE);
      TryReadAssimpProperty<float>(mp, PropertySemantic::RoughnessValue, *pMat, AI_MATKEY_SHININESS);
      TryReadAssimpProperty<float>(mp, PropertySemantic::MetallicValue, *pMat, AI_MATKEY_SHININESS_STRENGTH);
      TryReadAssimpProperty<aiColor3D>(mp, PropertySemantic::EmissiveColor, *pMat, AI_MATKEY_COLOR_EMISSIVE);
      TryReadAssimpProperty<int>(mp, PropertySemantic::TwosidedValue, *pMat, AI_MATKEY_TWOSIDED);
      // TryReadAssimpProperty<aiColor3D>(mp, PropertySemantic::MetallicColor, *pMat, AI_MATKEY_COLOR_SPECULAR);
      // TryReadAssimpProperty<aiColor3D>(mp, PropertySemantic::AmbientColor, *pMat, AI_MATKEY_COLOR_AMBIENT);
      // TryReadAssimpProperty<aiColor3D>(mp, PropertySemantic::OpacityColor, *pMat, AI_MATKEY_COLOR_TRANSPARENT, true);
      // TryReadAssimpProperty<float>(mp, PropertySemantic::OpacityValue, *pMat, AI_MATKEY_OPACITY);

      TryReadAssimpTextures(tr, aiTextureType_DIFFUSE, TextureSemantic::DiffuseMap, *pMat);
      TryReadAssimpTextures(tr, aiTextureType_BASE_COLOR, TextureSemantic::DiffuseMap, *pMat);          // override aiTextureType_DIFFUSE
      TryReadAssimpTextures(tr, aiTextureType_SHININESS, TextureSemantic::RoughnessMap, *pMat);
      TryReadAssimpTextures(tr, aiTextureType_DIFFUSE_ROUGHNESS, TextureSemantic::RoughnessMap, *pMat); // override aiTextureType_SHININESS
      TryReadAssimpTextures(tr, aiTextureType_SPECULAR, TextureSemantic::MetallicMap, *pMat);
      TryReadAssimpTextures(tr, aiTextureType_METALNESS, TextureSemantic::MetallicMap, *pMat);          // override aiTextureType_SPECULAR
      TryReadAssimpTextures(tr, aiTextureType_AMBIENT, TextureSemantic::OcclusionMap, *pMat);
      TryReadAssimpTextures(tr, aiTextureType_AMBIENT_OCCLUSION, TextureSemantic::OcclusionMap, *pMat); // override aiTextureType_AMBIENT
      TryReadAssimpTextures(tr, aiTextureType_DISPLACEMENT, TextureSemantic::DisplacementMap, *pMat);
      TryReadAssimpTextures(tr, aiTextureType_NORMALS, TextureSemantic::NormalMap, *pMat);
      TryReadAssimpTextures(tr, aiTextureType_EMISSIVE, TextureSemantic::EmissiveMap, *pMat);
      TryReadAssimpTextures(tr, aiTextureType_EMISSION_COLOR, TextureSemantic::EmissiveMap, *pMat); // override aiTextureType_EMISSIVE
      TryReadAssimpTextures(tr, aiTextureType_OPACITY, TextureSemantic::DiffuseAlphaMap, *pMat);
      // TryReadAssimpTextures(tr, aiTextureType_REFLECTION, TextureSemantic::ReflectionMap, *pMat); // From Assimp documentation "Contains the color of a perfect mirror reflection."
    }

    return EZ_SUCCESS;
  }
} // namespace ezModelImporter2
