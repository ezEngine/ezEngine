#include <RendererCore/PCH.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>

void ezMaterialResourceDescriptor::Clear()
{
  m_hBaseMaterial.Invalidate();
  m_hShader.Invalidate();
  m_PermutationVars.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
}

bool ezMaterialResourceDescriptor::operator==(const ezMaterialResourceDescriptor& other) const
{
  return m_hBaseMaterial == other.m_hBaseMaterial &&
    m_hShader == other.m_hShader &&
    m_PermutationVars == other.m_PermutationVars &&
    m_Parameters == other.m_Parameters &&
    m_Texture2DBindings == other.m_Texture2DBindings &&
    m_TextureCubeBindings == other.m_TextureCubeBindings;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialResource, 1, ezRTTIDefaultAllocator<ezMaterialResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

/// \todo Should be done on any thread, but currently crashes when deleting custom loaders in the resource manager
ezMaterialResource::ezMaterialResource() : ezResource<ezMaterialResource, ezMaterialResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
  m_iLastUpdated = 0;
  m_iLastConstantsUpdated = 0;

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezMaterialResource::OnResourceEvent, this));
}

ezMaterialResource::~ezMaterialResource()
{
  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezMaterialResource::OnResourceEvent, this));
}

ezHashedString ezMaterialResource::GetPermutationValue(const ezTempHashedString& sName)
{
  UpdateCaches();

  EZ_LOCK(m_CacheMutex);

  ezHashedString sResult;
  m_CachedPermutationVars.TryGetValue(sName, sResult);

  return sResult;
}

void ezMaterialResource::SetParameter(const ezHashedString& sName, const ezVariant& value)
{
  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_Desc.m_Parameters.GetCount(); ++i)
  {
    if (m_Desc.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != ezInvalidIndex)
    {
      if (m_Desc.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_Desc.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param = m_Desc.m_Parameters.ExpandAndGetRef();
      param.m_Name = sName;
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == ezInvalidIndex)
    {
      return;
    }

    m_Desc.m_Parameters.RemoveAtSwap(uiIndex);
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void ezMaterialResource::SetParameter(const char* szName, const ezVariant& value)
{
  ezTempHashedString sName(szName);

  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_Desc.m_Parameters.GetCount(); ++i)
  {
    if (m_Desc.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != ezInvalidIndex)
    {
      if (m_Desc.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_Desc.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param = m_Desc.m_Parameters.ExpandAndGetRef();
      param.m_Name.Assign(szName);
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == ezInvalidIndex)
    {
      return;
    }

    m_Desc.m_Parameters.RemoveAtSwap(uiIndex);
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

ezVariant ezMaterialResource::GetParameter(const ezTempHashedString& sName)
{
  EZ_LOCK(m_CacheMutex);

  UpdateCaches();

  ezVariant value;
  m_CachedParameters.TryGetValue(sName, value);

  return value;
}

void ezMaterialResource::SetTexture2DBinding(const ezHashedString& sName, const ezTexture2DResourceHandle& value)
{
  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_Desc.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_Desc.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_Desc.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_Texture2DBindings.RemoveAtSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void ezMaterialResource::SetTexture2DBinding(const char* szName, const ezTexture2DResourceHandle& value)
{
  ezTempHashedString sName(szName);

  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_Desc.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_Desc.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_Desc.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_Texture2DBindings.RemoveAtSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

ezTexture2DResourceHandle ezMaterialResource::GetTexture2DBinding(const ezTempHashedString& sName)
{
  EZ_LOCK(m_CacheMutex);

  UpdateCaches();

  // Use pointer to prevent ref counting
  ezTexture2DResourceHandle* pBinding;
  if (m_CachedTexture2DBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return ezTexture2DResourceHandle();
}


void ezMaterialResource::SetTextureCubeBinding(const ezHashedString& sName, const ezTextureCubeResourceHandle& value)
{
  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_Desc.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_Desc.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_Desc.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_TextureCubeBindings.RemoveAtSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void ezMaterialResource::SetTextureCubeBinding(const char* szName, const ezTextureCubeResourceHandle& value)
{
  ezTempHashedString sName(szName);

  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_Desc.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_Desc.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_Desc.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_TextureCubeBindings.RemoveAtSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

ezTextureCubeResourceHandle ezMaterialResource::GetTextureCubeBinding(const ezTempHashedString& sName)
{
  EZ_LOCK(m_CacheMutex);

  UpdateCaches();

  // Use pointer to prevent ref counting
  ezTextureCubeResourceHandle* pBinding;
  if (m_CachedTextureCubeBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return ezTextureCubeResourceHandle();
}

void ezMaterialResource::PreserveCurrentDesc()
{
  m_OriginalDesc = m_Desc;
}

void ezMaterialResource::ResetResource()
{
  if (m_Desc != m_OriginalDesc)
  {
    m_Desc = m_OriginalDesc;

    m_iLastModified.Increment();
    m_iLastConstantsModified.Increment();

    m_ModifiedEvent.Broadcast(this);
  }
}

ezResourceLoadDesc ezMaterialResource::UnloadData(Unload WhatToUnload)
{
  if (m_Desc.m_hBaseMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pBaseMaterial(m_Desc.m_hBaseMaterial, ezResourceAcquireMode::PointerOnly);
    pBaseMaterial->m_ModifiedEvent.RemoveEventHandler(ezMakeDelegate(&ezMaterialResource::OnBaseMaterialModified, this));
  }

  m_Desc.Clear();
  m_OriginalDesc.Clear();

  if (!m_hConstantBufferStorage.IsInvalidated())
  {
    ezRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
    m_hConstantBufferStorage.Invalidate();
  }

  m_hCachedShader.Invalidate();
  m_CachedPermutationVars.Clear();
  m_CachedParameters.Clear();
  m_CachedTexture2DBindings.Clear();
  m_CachedTextureCubeBindings.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezMaterialResource::UpdateContent(ezStreamReader* Stream)
{
  m_Desc.Clear();
  m_OriginalDesc.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("ezMaterialBin"))
  {
    ezStringBuilder sTemp, sTemp2;

    ezAssetFileHeader AssetHash;
    AssetHash.Read(*Stream);

    ezUInt8 uiVersion = 0;
    (*Stream) >> uiVersion;
    EZ_ASSERT_DEV(uiVersion <= 3, "Unknown ezMaterialBin version {0}", uiVersion);

    // Base material
    {
      (*Stream) >> sTemp;

      if (!sTemp.IsEmpty())
        m_Desc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>(sTemp);
    }

    // Shader
    {
      (*Stream) >> sTemp;

      if (!sTemp.IsEmpty())
        m_Desc.m_hShader = ezResourceManager::LoadResource<ezShaderResource>(sTemp);
    }

    // Permutation Variables
    {
      ezUInt16 uiPermVars;
      (*Stream) >> uiPermVars;

      m_Desc.m_PermutationVars.Reserve(uiPermVars);

      for (ezUInt16 i = 0; i < uiPermVars; ++i)
      {
        (*Stream) >> sTemp;
        (*Stream) >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          ezPermutationVar& pv = m_Desc.m_PermutationVars.ExpandAndGetRef();
          pv.m_sName.Assign(sTemp.GetData());
          pv.m_sValue.Assign(sTemp2.GetData());
        }
      }
    }

    // 2D Textures
    {
      ezUInt16 uiTextures = 0;
      (*Stream) >> uiTextures;

      m_Desc.m_Texture2DBindings.Reserve(uiTextures);

      for (ezUInt16 i = 0; i < uiTextures; ++i)
      {
        (*Stream) >> sTemp;
        (*Stream) >> sTemp2;

        if (sTemp.IsEmpty() || sTemp2.IsEmpty())
          continue;

        ezMaterialResourceDescriptor::Texture2DBinding& tc = m_Desc.m_Texture2DBindings.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = ezResourceManager::LoadResource<ezTexture2DResource>(sTemp2);
      }
    }

    // Cube Textures
    if (uiVersion >= 3)
    {
      ezUInt16 uiTextures = 0;
      (*Stream) >> uiTextures;

      m_Desc.m_TextureCubeBindings.Reserve(uiTextures);

      for (ezUInt16 i = 0; i < uiTextures; ++i)
      {
        (*Stream) >> sTemp;
        (*Stream) >> sTemp2;

        if (sTemp.IsEmpty() || sTemp2.IsEmpty())
          continue;

        ezMaterialResourceDescriptor::TextureCubeBinding& tc = m_Desc.m_TextureCubeBindings.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = ezResourceManager::LoadResource<ezTextureCubeResource>(sTemp2);
      }
    }


    if (uiVersion >= 2)
    {
      // Shader constants

      ezUInt16 uiConstants = 0;

      (*Stream) >> uiConstants;

      m_Desc.m_Parameters.Reserve(uiConstants);

      ezVariant vTemp;

      for (ezUInt16 i = 0; i < uiConstants; ++i)
      {
        (*Stream) >> sTemp;
        (*Stream) >> vTemp;

        if (sTemp.IsEmpty() || !vTemp.IsValid())
          continue;

        ezMaterialResourceDescriptor::Parameter& tc = m_Desc.m_Parameters.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = vTemp;
      }
    }
  }

  if (sAbsFilePath.HasExtension("ezMaterial"))
  {
    ezStringBuilder tmp;
    ezOpenDdlReader reader;

    if (reader.ParseDocument(*Stream, 0, ezLog::GetThreadLocalLogSystem()).Failed())
    {
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
    }

    const ezOpenDdlReaderElement* pRoot = reader.GetRootElement();

    const ezOpenDdlReaderElement* pBase = pRoot->FindChildOfType(ezOpenDdlPrimitiveType::String, "BaseMaterial");
    const ezOpenDdlReaderElement* pshader = pRoot->FindChildOfType(ezOpenDdlPrimitiveType::String, "Shader");

    // Read the base material
    if (pBase)
    {
      tmp = pBase->GetPrimitivesString()[0];
      m_Desc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>(tmp);
    }

    // Read the shader
    if (pshader)
    {
      tmp = pshader->GetPrimitivesString()[0];
      m_Desc.m_hShader = ezResourceManager::LoadResource<ezShaderResource>(tmp);
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
          ezPermutationVar& pv = m_Desc.m_PermutationVars.ExpandAndGetRef();

          tmp = pName->GetPrimitivesString()[0];
          pv.m_sName.Assign(tmp.GetData());

          tmp = pValue->GetPrimitivesString()[0];
          pv.m_sValue.Assign(tmp.GetData());
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
          ezMaterialResourceDescriptor::Parameter& sc = m_Desc.m_Parameters.ExpandAndGetRef();

          tmp = pName->GetPrimitivesString()[0];
          sc.m_Name.Assign(tmp.GetData());

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
          ezMaterialResourceDescriptor::Texture2DBinding& tc = m_Desc.m_Texture2DBindings.ExpandAndGetRef();

          tmp = pName->GetPrimitivesString()[0];
          tc.m_Name.Assign(tmp.GetData());

          tmp = pValue->GetPrimitivesString()[0];
          tc.m_Value = ezResourceManager::LoadResource<ezTexture2DResource>(tmp);
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("TextureCube"))
      {
        const ezOpenDdlReaderElement* pName = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Variable");
        const ezOpenDdlReaderElement* pValue = pChild->FindChildOfType(ezOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          ezMaterialResourceDescriptor::TextureCubeBinding& tc = m_Desc.m_TextureCubeBindings.ExpandAndGetRef();

          tmp = pName->GetPrimitivesString()[0];
          tc.m_Name.Assign(tmp.GetData());

          tmp = pValue->GetPrimitivesString()[0];
          tc.m_Value = ezResourceManager::LoadResource<ezTextureCubeResource>(tmp);
        }
      }
    }
  }

  if (m_Desc.m_hBaseMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pBaseMaterial(m_Desc.m_hBaseMaterial, ezResourceAcquireMode::PointerOnly);
    pBaseMaterial->m_ModifiedEvent.AddEventHandler(ezMakeDelegate(&ezMaterialResource::OnBaseMaterialModified, this));
  }

  m_OriginalDesc = m_Desc;

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);

  return res;
}

void ezMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezMaterialResource) +
    (ezUInt32)(m_Desc.m_PermutationVars.GetHeapMemoryUsage() +
               m_Desc.m_Parameters.GetHeapMemoryUsage() +
               m_Desc.m_Texture2DBindings.GetHeapMemoryUsage() +
               m_Desc.m_TextureCubeBindings.GetHeapMemoryUsage() +
               m_OriginalDesc.m_PermutationVars.GetHeapMemoryUsage() +
               m_OriginalDesc.m_Parameters.GetHeapMemoryUsage() +
               m_OriginalDesc.m_Texture2DBindings.GetHeapMemoryUsage() +
               m_OriginalDesc.m_TextureCubeBindings.GetHeapMemoryUsage());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;

}

ezResourceLoadDesc ezMaterialResource::CreateResource(const ezMaterialResourceDescriptor& descriptor)
{
  m_Desc = descriptor;
  m_OriginalDesc = descriptor;

  ezResourceLoadDesc res;
  res.m_State = ezResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (m_Desc.m_hBaseMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pBaseMaterial(m_Desc.m_hBaseMaterial, ezResourceAcquireMode::PointerOnly);
    pBaseMaterial->m_ModifiedEvent.AddEventHandler(ezMakeDelegate(&ezMaterialResource::OnBaseMaterialModified, this));
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  return res;
}

void ezMaterialResource::OnBaseMaterialModified(const ezMaterialResource* pModifiedMaterial)
{
  EZ_ASSERT_DEV(m_Desc.m_hBaseMaterial == pModifiedMaterial, "Implementation error");

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void ezMaterialResource::OnResourceEvent(const ezResourceEvent& resourceEvent)
{
  if (resourceEvent.m_EventType != ezResourceEventType::ResourceContentUpdated)
    return;

  if (m_hCachedShader == resourceEvent.m_pResource)
  {
    m_iLastConstantsModified.Increment();
  }
}

bool ezMaterialResource::IsModified()
{
  return m_iLastModified != m_iLastUpdated;
}

bool ezMaterialResource::AreContantsModified()
{
  return m_iLastConstantsModified != m_iLastConstantsUpdated;
}

void ezMaterialResource::UpdateCaches()
{
  if (!IsModified())
    return;

  m_iLastUpdated = m_iLastModified;

  EZ_LOCK(m_CacheMutex);

  m_hCachedShader.Invalidate();
  m_CachedPermutationVars.Clear();
  m_CachedParameters.Clear();
  m_CachedTexture2DBindings.Clear();
  m_CachedTextureCubeBindings.Clear();

  ezHybridArray<ezMaterialResource*, 16> materialHierarchy;
  ezMaterialResource* pCurrentMaterial = this;

  while (true)
  {
    materialHierarchy.PushBack(pCurrentMaterial);

    const ezMaterialResourceHandle& hParentMaterial = pCurrentMaterial->m_Desc.m_hBaseMaterial;
    if (!hParentMaterial.IsValid())
      break;

    pCurrentMaterial = ezResourceManager::BeginAcquireResource(hParentMaterial, ezResourceAcquireMode::AllowFallback);
  }

  // set state of parent material first
  for (ezUInt32 i = materialHierarchy.GetCount(); i-- > 0; )
  {
    ezMaterialResource* pMaterial = materialHierarchy[i];
    const ezMaterialResourceDescriptor& desc = pMaterial->m_Desc;

    if (desc.m_hShader.IsValid())
      m_hCachedShader = desc.m_hShader;

    for (const auto& permutationVar : desc.m_PermutationVars)
    {
      m_CachedPermutationVars.Insert(permutationVar.m_sName, permutationVar.m_sValue);
    }

    for (const auto& param : desc.m_Parameters)
    {
      m_CachedParameters.Insert(param.m_Name, param.m_Value);
    }

    for (const auto& textureBinding : desc.m_Texture2DBindings)
    {
      m_CachedTexture2DBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    for (const auto& textureBinding : desc.m_TextureCubeBindings)
    {
      m_CachedTextureCubeBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    // The last material is this material and was not acquired.
    if (i != 0)
    {
      ezResourceManager::EndAcquireResource(pMaterial);
    }
    materialHierarchy[i] = nullptr;
  }
}

void ezMaterialResource::UpdateConstantBuffer(ezShaderPermutationResource* pShaderPermutation)
{
  UpdateCaches();

  if (pShaderPermutation == nullptr)
    return;

  ezTempHashedString sConstantBufferName("ezMaterialConstants");
  const ezShaderResourceBinding* pBinding = pShaderPermutation->GetShaderStageBinary(ezGALShaderStage::PixelShader)->GetShaderResourceBinding(sConstantBufferName);
  if (pBinding == nullptr)
  {
    pBinding = pShaderPermutation->GetShaderStageBinary(ezGALShaderStage::VertexShader)->GetShaderResourceBinding(sConstantBufferName);
  }

  const ezShaderConstantBufferLayout* pLayout = pBinding != nullptr ? pBinding->m_pLayout : nullptr;
  if (pLayout == nullptr)
    return;

  m_iLastConstantsUpdated = m_iLastConstantsModified;

  if (m_hConstantBufferStorage.IsInvalidated())
  {
    m_hConstantBufferStorage = ezRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize);
  }

  ezConstantBufferStorageBase* pStorage = nullptr;
  if (ezRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage))
  {
    ezArrayPtr<ezUInt8> data = pStorage->GetRawDataForWriting();
    if (data.GetCount() != pLayout->m_uiTotalSize)
    {
      ezRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
      m_hConstantBufferStorage = ezRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize);

      EZ_VERIFY(ezRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage), "");
    }

    {
      EZ_LOCK(m_CacheMutex);

      for (auto& constant : pLayout->m_Constants)
      {
        ezUInt8* pDest = &data[constant.m_uiOffset];

        ezVariant* pValue = nullptr;
        m_CachedParameters.TryGetValue(constant.m_sName, pValue);

        constant.CopyDataFormVariant(pDest, pValue);
      }
    }
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);

