#include <RendererCore/PCH.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Foundation/IO/ExtendedJSONReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezMaterialResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialResource::ezMaterialResource()
{
  m_uiLoadedQualityLevel = 0;
  m_uiMaxQualityLevel = 1;
}

void ezMaterialResource::UnloadData(bool bFullUnload)
{
  m_Desc.Clear();

  m_LoadingState = ezResourceLoadState::Uninitialized;
}

void ezMaterialResource::UpdateContent(ezStreamReaderBase* Stream)
{
  m_Desc.Clear();

  m_uiLoadedQualityLevel = 1;
  m_LoadingState = ezResourceLoadState::Loaded;

  if (Stream == nullptr)
    return; /// \todo need failure state for resources

  ezExtendedJSONReader json;
  json.SetLogInterface(ezGlobalLog::GetInstance());

  if (json.Parse(*Stream).Failed())
  {
    /// \todo Need failed resource loading state
    return;
  }

  EZ_LOG_BLOCK("ezMaterialResource::UpdateContent", GetResourceID().GetData());

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

void ezMaterialResource::UpdateMemoryUsage()
{
  /// \todo Need a function on containers to query the memory usage (including over-allocated elements)

  SetMemoryUsageCPU(sizeof(ezMaterialResource) +
                    m_Desc.m_PermutationVars.GetCount() * sizeof(ezMaterialResourceDescriptor::PermutationVar) + 
                    m_Desc.m_ShaderConstants.GetCount() * sizeof(ezMaterialResourceDescriptor::ShaderConstant) +
                    m_Desc.m_TextureBindings.GetCount() * sizeof(ezMaterialResourceDescriptor::TextureBinding));

  SetMemoryUsageGPU(0);
}

void ezMaterialResource::CreateResource(const ezMaterialResourceDescriptor& descriptor)
{
  m_Desc = descriptor;

  m_uiMaxQualityLevel = 1;
  m_uiLoadedQualityLevel = 1;

  m_LoadingState = ezResourceLoadState::Loaded;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);

