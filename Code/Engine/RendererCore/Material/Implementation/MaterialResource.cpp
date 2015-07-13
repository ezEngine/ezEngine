#include <RendererCore/PCH.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezMaterialResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialResource::ezMaterialResource() : ezResource<ezMaterialResource, ezMaterialResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezMaterialResource::UnloadData(Unload WhatToUnload)
{
  m_Desc.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezMaterialResource::UpdateContent(ezStreamReaderBase* Stream)
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
    EZ_ASSERT_DEV(uiVersion == 1, "Unknown ezMaterialBin version %u", uiVersion);

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
          ezMaterialResourceDescriptor::PermutationVar& pv = m_Desc.m_PermutationVars.ExpandAndGetRef();
          pv.m_Name.Assign(sTemp.GetData());
          pv.m_Value.Assign(sTemp2.GetData());
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
        tc.m_NameHash = ezTempHashedString(sTemp.GetData()).GetHash();
        tc.m_Value = ezResourceManager::LoadResource<ezTextureResource>(sTemp2);
      }
    }
  }

  if (sAbsFilePath.HasExtension("ezMaterial"))
  {
    ezExtendedJSONReader json;
    json.SetLogInterface(ezGlobalLog::GetInstance());

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
            ezMaterialResourceDescriptor::PermutationVar& pv = m_Desc.m_PermutationVars.ExpandAndGetRef();
            pv.m_Name.Assign(it.Key().GetData());
            pv.m_Value.Assign(it.Value().ConvertTo<ezString>(&Conversion).GetData());

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

          m_Desc.m_ShaderConstants.Reserve(dict.GetCount());
          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            ezMaterialResourceDescriptor::ShaderConstant& sc = m_Desc.m_ShaderConstants.ExpandAndGetRef();
            sc.m_NameHash = ezTempHashedString(it.Key().GetData()).GetHash();
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
            tc.m_NameHash = ezTempHashedString(it.Key().GetData()).GetHash();

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
                                                 m_Desc.m_ShaderConstants.GetHeapMemoryUsage() +
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



EZ_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);

