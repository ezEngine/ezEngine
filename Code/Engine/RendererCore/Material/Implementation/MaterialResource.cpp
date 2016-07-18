#include <RendererCore/PCH.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialResource, 1, ezRTTIDefaultAllocator<ezMaterialResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMaterialResource::ezMaterialResource() : ezResource<ezMaterialResource, ezMaterialResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
  m_uiLastModified = 0;
  m_uiLastUpdated = 0;
}


ezTempHashedString ezMaterialResource::GetPermutationValue(const ezTempHashedString& sName)
{
  for (auto& permutationVar : m_Desc.m_PermutationVars)
  {
    if (permutationVar.m_sName == sName)
    {
      return permutationVar.m_sValue;
    }
  }

  if (m_Desc.m_hBaseMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pBaseMaterial(m_Desc.m_hBaseMaterial, ezResourceAcquireMode::AllowFallback);
    return pBaseMaterial->GetPermutationValue(sName);
  }

  return ezTempHashedString("");
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
  
  m_uiLastModified = ezMath::Max(ezRenderLoop::GetFrameCounter(), m_uiLastUpdated + 1);
}

void ezMaterialResource::SetTextureBinding(const char* szName, ezTextureResourceHandle value)
{
  ezTempHashedString sName(szName);

  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_Desc.m_TextureBindings.GetCount(); ++i)
  {
    if (m_Desc.m_TextureBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_TextureBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_Desc.m_TextureBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != ezInvalidIndex)
    {
      m_Desc.m_TextureBindings.RemoveAtSwap(uiIndex);
    }    
  }
}

ezResourceLoadDesc ezMaterialResource::UnloadData(Unload WhatToUnload)
{
  m_Desc.Clear();
  if (!m_hConstantBufferStorage.IsInvalidated())
  {
    ezRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
    m_hConstantBufferStorage.Invalidate();
  }

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezMaterialResource::UpdateContent(ezStreamReader* Stream)
{
  m_Desc.Clear();

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
    EZ_ASSERT_DEV(uiVersion <= 2, "Unknown ezMaterialBin version %u", uiVersion);

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

    // Textures
    {
      ezUInt16 uiTextures = 0;
      (*Stream) >> uiTextures;

      m_Desc.m_TextureBindings.Reserve(uiTextures);

      for (ezUInt16 i = 0; i < uiTextures; ++i)
      {
        (*Stream) >> sTemp;
        (*Stream) >> sTemp2;

        if (sTemp.IsEmpty() || sTemp2.IsEmpty())
          continue;

        ezMaterialResourceDescriptor::TextureBinding& tc = m_Desc.m_TextureBindings.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = ezResourceManager::LoadResource<ezTextureResource>(sTemp2);
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
    ezExtendedJSONReader json;
    json.SetLogInterface(ezGlobalLog::GetOrCreateInstance());

    if (json.Parse(*Stream).Failed())
    {
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
    }

    ezResult Conversion(EZ_FAILURE);

    // Read the base material
    {
      ezVariant* pValue = nullptr;
      if (json.GetTopLevelObject().TryGetValue("BaseMaterial", pValue))
      {
        ezString sValue = pValue->ConvertTo<ezString>(&Conversion);

        if (Conversion.Failed())
        {
          ezLog::Error("'BaseMaterial' variable is malformed.");
        }
        else if (!sValue.IsEmpty())
        {
          m_Desc.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>(sValue);
        }
      }
    }

    // Read the shader
    {
      ezVariant* pValue = nullptr;
      if (json.GetTopLevelObject().TryGetValue("Shader", pValue))
      {
        ezString sValue = pValue->ConvertTo<ezString>(&Conversion);

        if (Conversion.Failed())
        {
          ezLog::Error("'Shader' variable is malformed.");
        }
        else if (!sValue.IsEmpty())
        {
          m_Desc.m_hShader = ezResourceManager::LoadResource<ezShaderResource>(sValue);
        }
      }
    }

    // Read the shader permutation variables
    {
      ezVariant* pValue = nullptr;
      if (json.GetTopLevelObject().TryGetValue("Permutations", pValue))
      {
        if (!pValue->IsA<ezVariantDictionary>())
        {
          ezLog::Error("'Permutations' variable is not an object");
        }
        else
        {
          const ezVariantDictionary& dict = pValue->Get<ezVariantDictionary>();

          m_Desc.m_PermutationVars.Reserve(dict.GetCount());
          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            ezPermutationVar& pv = m_Desc.m_PermutationVars.ExpandAndGetRef();
            pv.m_sName.Assign(it.Key().GetData());
            pv.m_sValue.Assign(it.Value().ConvertTo<ezString>(&Conversion).GetData());

            if (Conversion.Failed())
            {
              ezLog::Error("'Permutations' object has member '%s' that is not convertible to a string", it.Key().GetData());
              m_Desc.m_PermutationVars.PopBack();
            }
          }
        }
      }
    }

    // Read the shader constants
    {
      ezVariant* pValue = nullptr;
      if (json.GetTopLevelObject().TryGetValue("Constants", pValue))
      {
        if (!pValue->IsA<ezVariantDictionary>())
        {
          ezLog::Error("'Constants' variable is not an object");
        }
        else
        {
          const ezVariantDictionary& dict = pValue->Get<ezVariantDictionary>();

          m_Desc.m_Parameters.Reserve(dict.GetCount());
          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            ezMaterialResourceDescriptor::Parameter& sc = m_Desc.m_Parameters.ExpandAndGetRef();
            sc.m_Name.Assign(it.Key().GetData());
            sc.m_Value = it.Value();
          }
        }
      }
    }

    // Read the texture references
    {
      ezVariant* pValue = nullptr;
      if (json.GetTopLevelObject().TryGetValue("Textures", pValue))
      {
        if (!pValue->IsA<ezVariantDictionary>())
        {
          ezLog::Error("'Textures' variable is not an object");
        }
        else
        {
          const ezVariantDictionary& dict = pValue->Get<ezVariantDictionary>();

          m_Desc.m_TextureBindings.Reserve(dict.GetCount());
          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            ezMaterialResourceDescriptor::TextureBinding& tc = m_Desc.m_TextureBindings.ExpandAndGetRef();
            tc.m_Name.Assign(it.Key().GetData());

            const ezString sTextureRef = it.Value().ConvertTo<ezString>(&Conversion);

            if (Conversion.Failed())
            {
              ezLog::Error("'Textures' object has member '%s' that is not convertible to a string", it.Key().GetData());
              m_Desc.m_TextureBindings.PopBack();
              continue;
            }

            tc.m_Value = ezResourceManager::LoadResource<ezTextureResource>(sTextureRef);
          }
        }
      }
    }
  }

  return res;
}

void ezMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezMaterialResource) +
                                     (ezUInt32) (m_Desc.m_PermutationVars.GetHeapMemoryUsage() +
                                                 m_Desc.m_Parameters.GetHeapMemoryUsage() +
                                                 m_Desc.m_TextureBindings.GetHeapMemoryUsage());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;

}

ezResourceLoadDesc ezMaterialResource::CreateResource(const ezMaterialResourceDescriptor& descriptor)
{
  m_Desc = descriptor;

  ezResourceLoadDesc res;
  res.m_State = ezResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}

void ezMaterialResource::UpdateConstantBuffer(ezHashTable<ezHashedString, ezVariant>& parameters, ezShaderPermutationResource* pShaderPermutation, ezUInt64 uiLastModified)
{
  ezTempHashedString sConstantBufferName("MaterialConstants");
  const ezShaderResourceBinding* pBinding = pShaderPermutation->GetShaderStageBinary(ezGALShaderStage::PixelShader)->GetShaderResourceBinding(sConstantBufferName);
  if (pBinding == nullptr)
  {
    pBinding = pShaderPermutation->GetShaderStageBinary(ezGALShaderStage::VertexShader)->GetShaderResourceBinding(sConstantBufferName);
  }

  const ezShaderConstantBufferLayout* pLayout = pBinding != nullptr ? pBinding->m_pLayout : nullptr;
  if (pLayout == nullptr)
    return;

  if (m_hConstantBufferStorage.IsInvalidated())
  {
    m_hConstantBufferStorage = ezRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize);
  }

  ezConstantBufferStorageBase* pStorage = nullptr;
  if (ezRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage))
  {
    ezArrayPtr<ezUInt8> data = pStorage->GetRawDataForWriting();

    for (auto& constant : pLayout->m_Constants)
    {
      ezUInt8* pDest = &data[constant.m_uiOffset];

      ezVariant* pValue = nullptr;
      parameters.TryGetValue(constant.m_sName, pValue);

      constant.CopyDataFormVariant(pDest, pValue);
    }
  }

  m_uiLastUpdated = uiLastModified;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);

