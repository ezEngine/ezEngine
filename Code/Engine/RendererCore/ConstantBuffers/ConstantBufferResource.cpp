#include <RendererCore/PCH.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <RendererCore/RendererCore.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <Foundation/Logging/Log.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConstantBufferResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezConstantBufferResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();


ezConstantBufferResource::ezConstantBufferResource()
{
  m_uiMaxQualityLevel = 1;
  m_Flags |= ezResourceFlags::UpdateOnMainThread;
}

void ezConstantBufferResource::UnloadData(bool bFullUnload)
{
  m_uiLoadedQualityLevel = 0;
  m_LoadingState = ezResourceLoadState::Uninitialized;

  m_Bytes.Clear();

  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);
  m_hGALConstantBuffer.Invalidate();
}

void ezConstantBufferResource::UpdateContent(ezStreamReaderBase* Stream)
{
  EZ_REPORT_FAILURE("This resource type does not support loading data from file.");
}

void ezConstantBufferResource::UpdateMemoryUsage()
{
  SetMemoryUsageCPU(m_Bytes.GetCount());
  SetMemoryUsageGPU(m_Bytes.GetCount());
}

void ezConstantBufferResource::CreateResource(const ezConstantBufferResourceDescriptorBase& descriptor)
{
  m_bHasBeenModified = true;
  m_Bytes.SetCount(descriptor.m_uiSize);
  ezMemoryUtils::Copy<ezUInt8>(m_Bytes.GetData(), descriptor.m_pBytes, descriptor.m_uiSize);

  m_uiLoadedQualityLevel = 1;
  m_LoadingState = ezResourceLoadState::Loaded;

  m_hGALConstantBuffer = ezGALDevice::GetDefaultDevice()->CreateConstantBuffer(m_Bytes.GetCount());
}

void ezConstantBufferResource::UploadStateToGPU(ezGALContext* pContext)
{
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  /// \todo Does it have benefits to only upload a part of the constant buffer? If yes, it should be easy to track the modified range

  pContext->UpdateBuffer(m_hGALConstantBuffer, 0, m_Bytes.GetData(), m_Bytes.GetCount());
}

void ezRendererCore::BindConstantBuffer(ezGALContext* pContext, const ezTempHashedString& sSlotName, const ezConstantBufferResourceHandle& hConstantBuffer)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ContextState& cs = s_ContextState[pContext];

  cs.m_BoundConstantBuffers[sSlotName.GetHash()] = hConstantBuffer;

  cs.m_bConstantBufferBindingsChanged = true;
}

ezUInt8* ezRendererCore::InternalBeginModifyConstantBuffer(ezConstantBufferResourceHandle hConstantBuffer, ezGALContext* pContext)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ContextState& cs = s_ContextState[pContext];

  EZ_ASSERT(cs.m_pCurrentlyModifyingBuffer == nullptr, "Only one buffer can be modified at a time. Call EndModifyConstantBuffer before updating another buffer.");

  cs.m_pCurrentlyModifyingBuffer = ezResourceManager::BeginAcquireResource<ezConstantBufferResource>(hConstantBuffer);

  return cs.m_pCurrentlyModifyingBuffer->m_Bytes.GetData();
}

void ezRendererCore::EndModifyConstantBuffer(ezGALContext* pContext)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ContextState& cs = s_ContextState[pContext];

  EZ_ASSERT(cs.m_pCurrentlyModifyingBuffer != nullptr, "No buffer is currently being modified. Call BeginModifyConstantBuffer before calling EndModifyConstantBuffer.");

  cs.m_pCurrentlyModifyingBuffer->m_bHasBeenModified = true;

  ezResourceManager::EndAcquireResource(cs.m_pCurrentlyModifyingBuffer);

  cs.m_bConstantBufferBindingsChanged = true; // make sure the next drawcall triggers an upload
  cs.m_pCurrentlyModifyingBuffer = nullptr;
  /// \todo Upload immediately ?
}

void ezRendererCore::ApplyConstantBufferBindings(ezGALContext* pContext, const ezShaderStageBinary* pBinary)
{
  const auto& cs = s_ContextState[pContext];

  // Update Material CB
  if (pBinary->m_uiMaterialCBSize > 0)
  {
    // if not yet existing, create the constant buffer
    if (!pBinary->m_hMaterialCB.IsValid())
    {
      ezConstantBufferResourceDescriptorRawBytes BufferDesc(pBinary->m_uiMaterialCBSize);

      static ezUInt32 uiUniqueID = 0;
      ++uiUniqueID;

      ezStringBuilder sTemp;
      sTemp.Format("MaterialCB_%u", uiUniqueID);

      pBinary->m_hMaterialCB = ezResourceManager::CreateResource<ezConstantBufferResource>(sTemp, BufferDesc);
    }

    // now fill the buffer
    if (pBinary->m_uiLastBufferModification < s_LastMaterialParamModification)
    {
      const ezUInt64 uiLastModTime = pBinary->m_uiLastBufferModification;

      pBinary->m_uiLastBufferModification = s_LastMaterialParamModification;

      ezUInt32 uiCurVar = 0;
      for (; uiCurVar < pBinary->m_MaterialParameters.GetCount(); ++uiCurVar)
      {
        const auto& mp = pBinary->m_MaterialParameters[uiCurVar];

        const MaterialParam* pMatParam = GetMaterialParameterPointer(mp.m_uiNameHash);

        if (pMatParam == nullptr)
        {
          // todo LOG ?
          continue;
        }

        // found at least one modified variable -> lock resource, update CB, etc.
        if (pMatParam->m_LastModification > uiLastModTime)
        {
          ezUInt8* pBufferData = BeginModifyConstantBuffer<ezUInt8>(pBinary->m_hMaterialCB, pContext);

          // go through the remaining variables (no need to update the previous ones)
          for (; uiCurVar < pBinary->m_MaterialParameters.GetCount(); ++uiCurVar)
          {
            const auto& mp = pBinary->m_MaterialParameters[uiCurVar];

            // m_pCachedValues holds the cached pointer to the MaterialParam, to prevent map lookups (pointer stays valid forever)
            const MaterialParam* pMatParam = (const MaterialParam*) mp.m_pCachedValues;

            if (mp.m_pCachedValues == nullptr)
            {
              pMatParam = GetMaterialParameterPointer(mp.m_uiNameHash);
              mp.m_pCachedValues = (void*) pMatParam;
            }

            // if the pointer is still null, the value is unknown, continue trying to get the value next time
            if (pMatParam == nullptr)
            {
              // todo LOG ?
              continue;
            }

            // copy the value from the registry into the constant buffer
            ezMemoryUtils::Copy(&pBufferData[mp.m_uiOffset], reinterpret_cast<const ezUInt8*>(pMatParam) +sizeof(MaterialParam), pMatParam->m_uiDataSize);
          }

          EndModifyConstantBuffer(pContext);

          break;
        }
      }
    }

    // make sure "our" material CB is bound -> only works as long as all MaterialCB's are identical across vertex shader, pixel shader, etc.
    BindConstantBuffer(pContext, "MaterialCB", pBinary->m_hMaterialCB);
  }

  for (const auto& rb : pBinary->m_ShaderResourceBindings)
  {
    if (rb.m_Type != ezShaderStageResource::ConstantBuffer)
      continue;

    const ezUInt32 uiResourceHash = rb.m_Name.GetHash();

    ezConstantBufferResourceHandle* hResource;
    if (!cs.m_BoundConstantBuffers.TryGetValue(uiResourceHash, hResource))
    {
      ezLog::Error("No resource is bound for constant buffer slot '%s'", rb.m_Name.GetData());
      continue;
    }

    if (hResource == nullptr || !hResource->IsValid())
    {
      ezLog::Error("An invalid resource is bound for constant buffer slot '%s'", rb.m_Name.GetData());
      continue;
    }

    ezResourceLock<ezConstantBufferResource> l(*hResource, ezResourceAcquireMode::AllowFallback);

    l->UploadStateToGPU(pContext);

    pContext->SetConstantBuffer(rb.m_iSlot, l->GetGALBufferHandle());
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_ConstantBuffers_ConstantBufferResource);

