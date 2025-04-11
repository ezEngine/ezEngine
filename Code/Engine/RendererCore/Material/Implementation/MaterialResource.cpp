#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Material/MaterialResource.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Material/MaterialManager.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <Texture/Image/Formats/DdsFileFormat.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

namespace
{
  template <typename Source, typename Target>
  void CopyMaterialDesc(const Source& source, Target& ref_target)
  {
    ref_target.Clear();
    ref_target.Reserve(source.GetCount());
    for (const auto& entry : source)
    {
      ref_target.PushBack({entry.Key(), entry.Value()});
    }
  }

  template <typename Property>
  struct SetNameHelper
  {
    EZ_ALWAYS_INLINE void SetName(Property& ref_prop, const char* szName) { ref_prop.m_Name.Assign(szName); }
    EZ_ALWAYS_INLINE void SetName(Property& ref_prop, ezHashedString sName) { ref_prop.m_Name = sName; }
  };

  template <typename Value, typename Property, typename Name>
  Value GetProperty(ezDynamicArray<Property>& ref_properties, Name sName)
  {
    for (ezUInt32 i = 0; i < ref_properties.GetCount(); ++i)
    {
      if (ref_properties[i].m_Name == sName)
      {
        return ref_properties[i].m_Value;
      }
    }
    return {};
  }

  template <typename Property, typename Name, typename Value, typename NameLookup>
  bool SetProperty(ezDynamicArray<Property>& ref_properties, const Name& sName, const Value& value, const NameLookup& sNameLookup)
  {
    SetNameHelper<Property> setNameHelper;
    ezUInt32 uiIndex = ezInvalidIndex;
    for (ezUInt32 i = 0; i < ref_properties.GetCount(); ++i)
    {
      if (ref_properties[i].m_Name == sNameLookup)
      {
        uiIndex = i;
        break;
      }
    }

    if (value.IsValid())
    {
      if (uiIndex != ezInvalidIndex)
      {
        if (ref_properties[uiIndex].m_Value == value)
        {
          return false;
        }

        ref_properties[uiIndex].m_Value = value;
      }
      else
      {
        auto& param = ref_properties.ExpandAndGetRef();
        setNameHelper.SetName(param, sName);
        param.m_Value = value;
      }
    }
    else
    {
      if (uiIndex == ezInvalidIndex)
      {
        return false;
      }

      ref_properties.RemoveAtAndSwap(uiIndex);
    }
    return true;
  }
} // namespace

void ezMaterialResourceDescriptor::Clear()
{
  m_hBaseMaterial.Invalidate();
  m_sSurface.Clear();
  m_hShader.Invalidate();
  m_PermutationVars.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
  m_RenderDataCategory = ezInvalidRenderDataCategory;
}

bool ezMaterialResourceDescriptor::operator==(const ezMaterialResourceDescriptor& other) const
{
  return m_hBaseMaterial == other.m_hBaseMaterial &&
         m_hShader == other.m_hShader &&
         m_PermutationVars == other.m_PermutationVars &&
         m_Parameters == other.m_Parameters &&
         m_Texture2DBindings == other.m_Texture2DBindings &&
         m_TextureCubeBindings == other.m_TextureCubeBindings &&
         m_RenderDataCategory == other.m_RenderDataCategory;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialResource, 1, ezRTTIDefaultAllocator<ezMaterialResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezMaterialResource);
// clang-format on



ezMaterialResource::ezMaterialResource()
  : ezResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
}

ezMaterialResource::~ezMaterialResource()
{
  ezMaterialManager::MaterialRemoved(this);
}

ezHashedString ezMaterialResource::GetPermutationValue(const ezTempHashedString& sName)
{
  for (ezUInt32 i = 0; i < m_mDesc.m_PermutationVars.GetCount(); ++i)
  {
    if (m_mDesc.m_PermutationVars[i].m_sName == sName)
    {
      return m_mDesc.m_PermutationVars[i].m_sValue;
      break;
    }
  }
  return {};
}

ezHashedString ezMaterialResource::GetSurface() const
{
  return m_mDesc.m_sSurface;
}

void ezMaterialResource::SetParameter(const ezHashedString& sName, const ezVariant& value)
{
  if (SetProperty(m_mDesc.m_Parameters, sName, value, sName))
  {
    SetModified(DirtyFlags::Parameter);
  }
}

void ezMaterialResource::SetParameter(const char* szName, const ezVariant& value)
{
  ezTempHashedString sName(szName);
  if (SetProperty(m_mDesc.m_Parameters, szName, value, sName))
  {
    SetModified(DirtyFlags::Parameter);
  }
}

ezVariant ezMaterialResource::GetParameter(const ezTempHashedString& sName)
{
  return GetProperty<ezVariant>(m_mDesc.m_Parameters, sName);
}

void ezMaterialResource::SetTexture2DBinding(const ezHashedString& sName, const ezTexture2DResourceHandle& value)
{
  if (SetProperty(m_mDesc.m_Texture2DBindings, sName, value, sName))
  {
    SetModified(DirtyFlags::Texture2D);
  }
}

void ezMaterialResource::SetTexture2DBinding(const char* szName, const ezTexture2DResourceHandle& value)
{
  ezTempHashedString sName(szName);
  if (SetProperty(m_mDesc.m_Texture2DBindings, szName, value, sName))
  {
    SetModified(DirtyFlags::Texture2D);
  }
}

ezTexture2DResourceHandle ezMaterialResource::GetTexture2DBinding(const ezTempHashedString& sName)
{
  return GetProperty<ezTexture2DResourceHandle>(m_mDesc.m_Texture2DBindings, sName);
}

void ezMaterialResource::SetTextureCubeBinding(const ezHashedString& sName, const ezTextureCubeResourceHandle& value)
{
  if (SetProperty(m_mDesc.m_TextureCubeBindings, sName, value, sName))
  {
    SetModified(DirtyFlags::TextureCube);
  }
}

void ezMaterialResource::SetTextureCubeBinding(const char* szName, const ezTextureCubeResourceHandle& value)
{
  ezTempHashedString sName(szName);
  if (SetProperty(m_mDesc.m_TextureCubeBindings, szName, value, sName))
  {
    SetModified(DirtyFlags::TextureCube);
  }
}

ezTextureCubeResourceHandle ezMaterialResource::GetTextureCubeBinding(const ezTempHashedString& sName)
{
  return GetProperty<ezTextureCubeResourceHandle>(m_mDesc.m_TextureCubeBindings, sName);
}

ezRenderData::Category ezMaterialResource::GetRenderDataCategory()
{
  return m_mDesc.m_RenderDataCategory;
}

void ezMaterialResource::PreserveCurrentDesc()
{
  m_mOriginalDesc = m_mDesc;
}

void ezMaterialResource::ResetResource()
{
  if (m_mDesc != m_mOriginalDesc)
  {
    m_mDesc = m_mOriginalDesc;

    SetModified(DirtyFlags::ResourceReset);
  }
}

const char* ezMaterialResource::GetDefaultMaterialFileName(DefaultMaterialType materialType)
{
  switch (materialType)
  {
    case DefaultMaterialType::Fullbright:
      return "Base/Materials/BaseMaterials/Fullbright.ezMaterialAsset";
    case DefaultMaterialType::FullbrightAlphaTest:
      return "Base/Materials/BaseMaterials/FullbrightAlphaTest.ezMaterialAsset";
    case DefaultMaterialType::Lit:
      return "Base/Materials/BaseMaterials/Lit.ezMaterialAsset";
    case DefaultMaterialType::LitAlphaTest:
      return "Base/Materials/BaseMaterials/LitAlphaTest.ezMaterialAsset";
    case DefaultMaterialType::Sky:
      return "Base/Materials/BaseMaterials/Sky.ezMaterialAsset";
    case DefaultMaterialType::MissingMaterial:
      return "Base/Materials/Common/MissingMaterial.ezMaterialAsset";
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

ezResourceLoadDesc ezMaterialResource::UnloadData(Unload WhatToUnload)
{
  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezMaterialResource::UpdateContent(ezStreamReader* pOuterStream)
{
  // Setting all dirty flags here outside of SetModified prevents the setters being used from calling into the ezMaterialManager before the resource is fully loaded.
  m_DirtyFlags.SetValue(DirtyFlags::ResourceCreation);
  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  if (pOuterStream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  ezStringBuilder sAbsFilePath;
  (*pOuterStream) >> sAbsFilePath;

  ezUInt8 uiVersion = 0;
  if (sAbsFilePath.HasExtension("ezBinMaterial"))
  {
    ezStringBuilder sTemp, sTemp2;

    ezAssetFileHeader AssetHash;
    AssetHash.Read(*pOuterStream).IgnoreResult();

    (*pOuterStream) >> uiVersion;
    EZ_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 8, "Unknown ezBinMaterial version {0}", uiVersion);

    ezUInt8 uiCompressionMode = 0;
    if (uiVersion >= 6)
    {
      *pOuterStream >> uiCompressionMode;
    }

    ezStreamReader* pInnerStream = pOuterStream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    ezCompressedStreamReaderZstd decompressorZstd;
#endif

    switch (uiCompressionMode)
    {
      case 0:
        break;

      case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        decompressorZstd.SetInputStream(pOuterStream);
        pInnerStream = &decompressorZstd;
        break;
#else
        ezLog::Error("Material resource is compressed with zstandard, but support for this compressor is not compiled in.");
        res.m_State = ezResourceState::LoadedResourceMissing;
        return res;
#endif

      default:
        ezLog::Error("Material resource is compressed with an unknown algorithm.");
        res.m_State = ezResourceState::LoadedResourceMissing;
        return res;
    }

    ezStreamReader& s = *pInnerStream;

    // Base material
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>(sTemp);
    }

    // Surface
    {
      s >> sTemp;
      m_mDesc.m_sSurface.Assign(sTemp.GetView());
    }

    // Shader
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hShader = ezResourceManager::LoadResource<ezShaderResource>(sTemp);
    }

    // Permutation Variables
    {
      ezUInt16 uiPermVars;
      s >> uiPermVars;

      m_mDesc.m_PermutationVars.Reserve(uiPermVars);

      for (ezUInt16 i = 0; i < uiPermVars; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          AddPermutationVar(sTemp, sTemp2);
        }
      }
    }

    // 2D Textures
    {
      ezUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_Texture2DBindings.Reserve(uiTextures);

      for (ezUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          ezMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = ezResourceManager::LoadResource<ezTexture2DResource>(sTemp2);
        }
      }
    }

    // Cube Textures
    {
      ezUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_TextureCubeBindings.Reserve(uiTextures);

      for (ezUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          ezMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = ezResourceManager::LoadResource<ezTextureCubeResource>(sTemp2);
        }
      }
    }

    // Shader constants
    {
      ezUInt16 uiConstants = 0;
      s >> uiConstants;

      m_mDesc.m_Parameters.Reserve(uiConstants);

      ezVariant vTemp;

      for (ezUInt16 i = 0; i < uiConstants; ++i)
      {
        s >> sTemp;
        s >> vTemp;

        if (!sTemp.IsEmpty() && vTemp.IsValid())
        {
          ezMaterialResourceDescriptor::Parameter& tc = m_mDesc.m_Parameters.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = vTemp;
        }
      }
    }

    // Render data category
    if (uiVersion >= 7)
    {
      ezStringBuilder sRenderDataCategoryName;
      s >> sRenderDataCategoryName;

      ezTempHashedString sCategoryNameHashed(sRenderDataCategoryName.GetView());
      if (sCategoryNameHashed != ezTempHashedString("<Invalid>"))
      {
        m_mDesc.m_RenderDataCategory = ezRenderData::FindCategory(sCategoryNameHashed);
        if (m_mDesc.m_RenderDataCategory == ezInvalidRenderDataCategory)
        {
          ezLog::Error("Material '{}' uses an invalid render data category '{}'", GetResourceIdOrDescription(), sRenderDataCategoryName);
        }
      }
    }

    if (uiVersion >= 5)
    {
      ezStreamReader& s = *pInnerStream;

      ezStringBuilder sResourceName;
      s >> sResourceName;

      ezTextureResourceLoader::LoadedData embedded;

      while (!sResourceName.IsEmpty())
      {
        ezUInt32 dataSize = 0;
        s >> dataSize;

        ezTextureResourceLoader::LoadTexFile(s, embedded).IgnoreResult();
        embedded.m_bIsFallback = true;

        ezDefaultMemoryStreamStorage storage;
        ezMemoryStreamWriter loadStreamWriter(&storage);
        ezTextureResourceLoader::WriteTextureLoadStream(loadStreamWriter, embedded);

        ezMemoryStreamReader loadStreamReader(&storage);

        ezTexture2DResourceHandle hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sResourceName);
        ezResourceManager::SetResourceLowResData(hTexture, &loadStreamReader);

        s >> sResourceName;
      }
    }
  }
  else if (sAbsFilePath.HasExtension("ezMaterial"))
  {
    ezOpenDdlReader reader;

    if (reader.ParseDocument(*pOuterStream, 0, ezLog::GetThreadLocalLogSystem()).Failed())
    {
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
    }

    const ezOpenDdlReaderElement* pRoot = reader.GetRootElement();

    // Read the base material
    if (const ezOpenDdlReaderElement* pBase = pRoot->FindChildOfType(ezOpenDdlPrimitiveType::String, "BaseMaterial"))
    {
      m_mDesc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>(pBase->GetPrimitivesString()[0]);
    }

    // Read the shader
    if (const ezOpenDdlReaderElement* pShader = pRoot->FindChildOfType(ezOpenDdlPrimitiveType::String, "Shader"))
    {
      m_mDesc.m_hShader = ezResourceManager::LoadResource<ezShaderResource>(pShader->GetPrimitivesString()[0]);
    }

    // Read the render data category
    if (const ezOpenDdlReaderElement* pRenderDataCategory = pRoot->FindChildOfType(ezOpenDdlPrimitiveType::String, "RenderDataCategory"))
    {
      m_mDesc.m_RenderDataCategory = ezRenderData::FindCategory(ezTempHashedString(pRenderDataCategory->GetPrimitivesString()[0]));
    }

    for (const ezOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
    {
      // Read the shader permutation variables
      if (pChild->IsCustomType("Permutation"))
      {
        const ezOpenDdlReaderElement* pName = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Variable");
        const ezOpenDdlReaderElement* pValue = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          AddPermutationVar(pName->GetPrimitivesString()[0], pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the shader constants
      if (pChild->IsCustomType("Constant"))
      {
        const ezOpenDdlReaderElement* pName = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Variable");
        const ezOpenDdlReaderElement* pValue = pChild->FindChild("Value");

        ezVariant value;
        if (pName && pValue && ezOpenDdlUtils::ConvertToVariant(pValue, value).Succeeded())
        {
          ezMaterialResourceDescriptor::Parameter& sc = m_mDesc.m_Parameters.ExpandAndGetRef();
          sc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          sc.m_Value = value;
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("Texture2D"))
      {
        const ezOpenDdlReaderElement* pName = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Variable");
        const ezOpenDdlReaderElement* pValue = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          ezMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = ezResourceManager::LoadResource<ezTexture2DResource>(pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("TextureCube"))
      {
        const ezOpenDdlReaderElement* pName = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Variable");
        const ezOpenDdlReaderElement* pValue = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          ezMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = ezResourceManager::LoadResource<ezTextureCubeResource>(pValue->GetPrimitivesString()[0]);
        }
      }
    }
  }
  else
  {
    ezLog::Error("Unknown material file type: '{}'", sAbsFilePath);
  }

  // With version 8, all materials are flattened at asset transform time, removing the need to flatten the base material hierarchy.
  if (uiVersion < 8)
  {
    // Flatten works on the original desc of the base material hierarchy and stores the end result in m_mDesc.
    m_mOriginalDesc = m_mDesc;
    FlattenOriginalDescHierarchy();
  }

  // There is no guarantee that a material defines a render data category, so we always have to compute the fallbacks.
  ComputeRenderDataCategory();

  // After loading, base material info is removed as everything is flattened into this material.
  m_mDesc.m_hBaseMaterial.Invalidate();
  EZ_ASSERT_DEBUG(m_mDesc.m_RenderDataCategory != ezInvalidRenderDataCategory, "FlattenHierarchy should have set a category and newer versions should have it serialized.");

  m_mOriginalDesc = m_mDesc;

  // We add the material right away instead of during extraction / begin rendering to make sure the materialId can be used right away.
  ezMaterialManager::MaterialAdded(this);
  return res;
}

void ezMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU =
    sizeof(ezMaterialResource) + (ezUInt32)(m_mDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mDesc.m_Parameters.GetHeapMemoryUsage() + m_mDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mDesc.m_TextureCubeBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mOriginalDesc.m_Parameters.GetHeapMemoryUsage() + m_mOriginalDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_TextureCubeBindings.GetHeapMemoryUsage());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezMaterialResource, ezMaterialResourceDescriptor)
{
  m_mOriginalDesc = descriptor;

  ezResourceLoadDesc res;
  res.m_State = ezResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_DirtyFlags = DirtyFlags::ResourceCreation;
  FlattenOriginalDescHierarchy();
  ComputeRenderDataCategory();

  // After creation, base material info is removed as everything is flattened into this material.
  m_mDesc.m_hBaseMaterial.Invalidate();
  EZ_ASSERT_DEBUG(m_mDesc.m_RenderDataCategory != ezInvalidRenderDataCategory, "FlattenHierarchy should have set a category");
  m_mOriginalDesc = m_mDesc;

  // We add the material right away instead of during extraction / begin rendering to make sure the materialId can be used right away.
  ezMaterialManager::MaterialAdded(this);
  return res;
}

void ezMaterialResource::AddPermutationVar(ezStringView sName, ezStringView sValue)
{
  ezHashedString sNameHashed;
  sNameHashed.Assign(sName);
  ezHashedString sValueHashed;
  sValueHashed.Assign(sValue);

  if (ezShaderManager::IsPermutationValueAllowed(sNameHashed, sValueHashed))
  {
    ezPermutationVar& pv = m_mDesc.m_PermutationVars.ExpandAndGetRef();
    pv.m_sName = sNameHashed;
    pv.m_sValue = sValueHashed;
  }
  SetModified(DirtyFlags::PermutationVar);
}

void ezMaterialResource::SetModified(ezMaterialResource::DirtyFlags::Enum flag)
{
  bool bAlreadyModified = m_DirtyFlags.IsAnyFlagSet();
  m_DirtyFlags |= flag;
  if (!bAlreadyModified)
  {
    ezMaterialManager::MaterialModified(GetResourceHandle());
  }
  m_ModifiedEvent.Broadcast(this);
}


void ezMaterialResource::FlattenOriginalDescHierarchy()
{
  ezHybridArray<ezMaterialResource*, 16> materialHierarchy;
  ezMaterialResource* pCurrentMaterial = this;

  while (true)
  {
    materialHierarchy.PushBack(pCurrentMaterial);

    const ezMaterialResourceHandle& hBaseMaterial = pCurrentMaterial->m_mOriginalDesc.m_hBaseMaterial;
    if (!hBaseMaterial.IsValid())
      break;

    // Ensure that the base material is loaded at this point.
    // For loaded materials this will always be the case but is still necessary for runtime created materials.
    pCurrentMaterial = ezResourceManager::BeginAcquireResource(hBaseMaterial, ezResourceAcquireMode::BlockTillLoaded);
  }

  EZ_SCOPE_EXIT(for (ezUInt32 i = materialHierarchy.GetCount(); i-- > 1;) {
    ezMaterialResource* pMaterial = materialHierarchy[i];
    ezResourceManager::EndAcquireResource(pMaterial);

    materialHierarchy[i] = nullptr;
  });

  struct FlattenedMaterial
  {
    ezShaderResourceHandle m_hShader;
    ezHashedString m_sSurface;
    ezHashTable<ezHashedString, ezHashedString> m_PermutationVars;
    ezHashTable<ezHashedString, ezVariant> m_Parameters;
    ezHashTable<ezHashedString, ezTexture2DResourceHandle> m_Texture2DBindings;
    ezHashTable<ezHashedString, ezTextureCubeResourceHandle> m_TextureCubeBindings;
    ezRenderData::Category m_RenderDataCategory;
  } flattenedMaterial;

  // set state of parent material first
  for (ezUInt32 i = materialHierarchy.GetCount(); i-- > 0;)
  {
    ezMaterialResource* pMaterial = materialHierarchy[i];
    const ezMaterialResourceDescriptor& desc = pMaterial->m_mOriginalDesc;

    if (desc.m_hShader.IsValid())
      flattenedMaterial.m_hShader = desc.m_hShader;

    if (!desc.m_sSurface.IsEmpty())
      flattenedMaterial.m_sSurface = desc.m_sSurface;

    for (const auto& permutationVar : desc.m_PermutationVars)
    {
      flattenedMaterial.m_PermutationVars.Insert(permutationVar.m_sName, permutationVar.m_sValue);
    }

    for (const auto& param : desc.m_Parameters)
    {
      flattenedMaterial.m_Parameters.Insert(param.m_Name, param.m_Value);
    }

    for (const auto& textureBinding : desc.m_Texture2DBindings)
    {
      flattenedMaterial.m_Texture2DBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    for (const auto& textureBinding : desc.m_TextureCubeBindings)
    {
      flattenedMaterial.m_TextureCubeBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    if (desc.m_RenderDataCategory != ezInvalidRenderDataCategory)
    {
      flattenedMaterial.m_RenderDataCategory = desc.m_RenderDataCategory;
    }
  }

  m_mDesc.m_hBaseMaterial.Invalidate();
  m_mDesc.m_sSurface = flattenedMaterial.m_sSurface;
  m_mDesc.m_hShader = flattenedMaterial.m_hShader;
  m_mDesc.m_RenderDataCategory = flattenedMaterial.m_RenderDataCategory;
  CopyMaterialDesc(flattenedMaterial.m_PermutationVars, m_mDesc.m_PermutationVars);
  CopyMaterialDesc(flattenedMaterial.m_Parameters, m_mDesc.m_Parameters);
  CopyMaterialDesc(flattenedMaterial.m_Texture2DBindings, m_mDesc.m_Texture2DBindings);
  CopyMaterialDesc(flattenedMaterial.m_TextureCubeBindings, m_mDesc.m_TextureCubeBindings);
}

void ezMaterialResource::ComputeRenderDataCategory()
{
  if (m_mDesc.m_RenderDataCategory.IsValid())
    return;

  ezHashedString sBlendModeValue = GetPermutationValue("BLEND_MODE");
  if (sBlendModeValue.IsEmpty() || sBlendModeValue == ezTempHashedString("BLEND_MODE_OPAQUE"))
  {
    m_mDesc.m_RenderDataCategory = ezDefaultRenderDataCategories::LitOpaque;
  }
  else if (sBlendModeValue == ezTempHashedString("BLEND_MODE_MASKED") || sBlendModeValue == ezTempHashedString("BLEND_MODE_DITHERED"))
  {
    m_mDesc.m_RenderDataCategory = ezDefaultRenderDataCategories::LitMasked;
  }
  else
  {
    m_mDesc.m_RenderDataCategory = ezDefaultRenderDataCategories::LitTransparent;
  }
}

const ezMaterialResourceDescriptor& ezMaterialResource::GetCurrentDesc() const
{
  return m_mDesc;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);
