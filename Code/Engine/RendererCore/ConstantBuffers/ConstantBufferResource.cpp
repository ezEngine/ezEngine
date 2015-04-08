#include <RendererCore/PCH.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <Foundation/Logging/Log.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConstantBufferResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezConstantBufferResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();


ezConstantBufferResource::ezConstantBufferResource() : ezResource<ezConstantBufferResource, ezConstantBufferResourceDescriptorBase>(DoUpdate::OnMainThread, 1)
{

}

ezResourceLoadDesc ezConstantBufferResource::UnloadData(Unload WhatToUnload)
{
  m_Bytes.Clear();

  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);
  m_hGALConstantBuffer.Invalidate();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0; // not reloadable
  res.m_State = ezResourceState::Unloaded;

  return res;
}

void ezConstantBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezConstantBufferResource) + m_Bytes.GetCount();
  out_NewMemoryUsage.m_uiMemoryGPU = m_Bytes.GetCount();
}

ezResourceLoadDesc ezConstantBufferResource::UpdateContent(ezStreamReaderBase* Stream)
{
  EZ_REPORT_FAILURE("This resource type does not support loading data from file.");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezConstantBufferResource::CreateResource(const ezConstantBufferResourceDescriptorBase& descriptor)
{
  m_bHasBeenModified = true;
  m_Bytes.SetCount(descriptor.m_uiSize);
  ezMemoryUtils::Copy<ezUInt8>(m_Bytes.GetData(), descriptor.m_pBytes, descriptor.m_uiSize);

  m_hGALConstantBuffer = ezGALDevice::GetDefaultDevice()->CreateConstantBuffer(m_Bytes.GetCount());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezConstantBufferResource::UploadStateToGPU(ezGALContext* pContext)
{
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  /// \todo Does it have benefits to only upload a part of the constant buffer? If yes, it should be easy to track the modified range

  pContext->UpdateBuffer(m_hGALConstantBuffer, 0, m_Bytes.GetData(), m_Bytes.GetCount());
}

void ezRenderContext::BindConstantBuffer(const ezTempHashedString& sSlotName, const ezConstantBufferResourceHandle& hConstantBuffer)
{
  m_BoundConstantBuffers[sSlotName.GetHash()] = hConstantBuffer;

  m_StateFlags.Add(ezRenderContextFlags::ConstantBufferBindingChanged);
}

ezUInt8* ezRenderContext::InternalBeginModifyConstantBuffer(ezConstantBufferResourceHandle hConstantBuffer)
{
  EZ_ASSERT_DEV(m_pCurrentlyModifyingBuffer == nullptr, "Only one buffer can be modified at a time. Call EndModifyConstantBuffer before updating another buffer.");

  m_pCurrentlyModifyingBuffer = ezResourceManager::BeginAcquireResource<ezConstantBufferResource>(hConstantBuffer);

  return m_pCurrentlyModifyingBuffer->m_Bytes.GetData();
}

void ezRenderContext::EndModifyConstantBuffer()
{
  EZ_ASSERT_DEV(m_pCurrentlyModifyingBuffer != nullptr, "No buffer is currently being modified. Call BeginModifyConstantBuffer before calling EndModifyConstantBuffer.");

  m_pCurrentlyModifyingBuffer->m_bHasBeenModified = true;

  ezResourceManager::EndAcquireResource(m_pCurrentlyModifyingBuffer);

  // make sure the next drawcall triggers an upload
  m_StateFlags.Add(ezRenderContextFlags::ConstantBufferBindingChanged);

  m_pCurrentlyModifyingBuffer = nullptr;
}

void ezRenderContext::ApplyConstantBufferBindings(const ezShaderStageBinary* pBinary)
{
  // Update Material CB
  if (pBinary->m_pMaterialParamCB && pBinary->m_pMaterialParamCB->m_uiMaterialCBSize > 0)
  {
    // if not yet existing, create the constant buffer
    if (!pBinary->m_pMaterialParamCB->m_hMaterialCB.IsValid())
    {
      ezConstantBufferResourceDescriptorRawBytes BufferDesc(pBinary->m_pMaterialParamCB->m_uiMaterialCBSize);

      static ezUInt32 uiUniqueID = 0;
      ++uiUniqueID;

      ezStringBuilder sTemp;
      sTemp.Format("MaterialCB_%u", uiUniqueID);

      pBinary->m_pMaterialParamCB->m_hMaterialCB = ezResourceManager::CreateResource<ezConstantBufferResource>(sTemp, BufferDesc);
    }

    // now fill the buffer
    if (pBinary->m_pMaterialParamCB->m_uiLastBufferModification < s_LastMaterialParamModification)
    {
      const ezUInt64 uiLastModTime = pBinary->m_pMaterialParamCB->m_uiLastBufferModification;

      pBinary->m_pMaterialParamCB->m_uiLastBufferModification = s_LastMaterialParamModification;

      ezUInt32 uiCurVar = 0;
      for (; uiCurVar < pBinary->m_pMaterialParamCB->m_MaterialParameters.GetCount(); ++uiCurVar)
      {
        const auto& mp = pBinary->m_pMaterialParamCB->m_MaterialParameters[uiCurVar];

        const MaterialParam* pMatParam = GetMaterialParameterPointer(mp.m_uiNameHash);

        if (pMatParam == nullptr)
        {
          // todo LOG ?
          continue;
        }

        // found at least one modified variable -> lock resource, update CB, etc.
        if (pMatParam->m_LastModification > uiLastModTime)
        {
          ezUInt8* pBufferData = BeginModifyConstantBuffer<ezUInt8>(pBinary->m_pMaterialParamCB->m_hMaterialCB);

          // go through the remaining variables (no need to update the previous ones)
          for (; uiCurVar < pBinary->m_pMaterialParamCB->m_MaterialParameters.GetCount(); ++uiCurVar)
          {
            const auto& mp = pBinary->m_pMaterialParamCB->m_MaterialParameters[uiCurVar];

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

            if (pMatParam->m_LastModification > uiLastModTime)
            {
              // copy the value from the registry into the constant buffer
              ezMemoryUtils::Copy(&pBufferData[mp.m_uiOffset], reinterpret_cast<const ezUInt8*>(pMatParam) + sizeof(MaterialParam), pMatParam->m_uiDataSize);
            }
          }

          EndModifyConstantBuffer();

          break;
        }
      }
    }

    // make sure "our" material CB is bound -> only works as long as all MaterialCB's are identical across vertex shader, pixel shader, etc.
    BindConstantBuffer("MaterialCB", pBinary->m_pMaterialParamCB->m_hMaterialCB);
  }

  for (const auto& rb : pBinary->m_ShaderResourceBindings)
  {
    if (rb.m_Type != ezShaderStageResource::ConstantBuffer)
      continue;

    const ezUInt32 uiResourceHash = rb.m_Name.GetHash();

    ezConstantBufferResourceHandle* hResource;
    if (!m_BoundConstantBuffers.TryGetValue(uiResourceHash, hResource))
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

    l->UploadStateToGPU(m_pGALContext);

    m_pGALContext->SetConstantBuffer(rb.m_iSlot, l->GetGALBufferHandle());
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_ConstantBuffers_ConstantBufferResource);

