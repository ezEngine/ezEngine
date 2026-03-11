#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/State/PipelineCache.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, PipelineCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezGALPipelineCache);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezGALPipelineCache* pDummy = ezGALPipelineCache::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

EZ_IMPLEMENT_SINGLETON(ezGALPipelineCache);

ezGALPipelineCache::ezGALPipelineCache()
  : m_SingletonRegistrar(this)
{
  ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezGALPipelineCache::GALDeviceEventHandler, this));
}

ezGALPipelineCache::~ezGALPipelineCache()
{
  ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezGALPipelineCache::GALDeviceEventHandler, this));
}

void ezGALPipelineCache::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case ezGALDeviceEvent::AfterInit:
      m_pDevice = e.m_pDevice;
      break;
    case ezGALDeviceEvent::BeforeShutdown:
      Clear();
      break;
    default:
      break;
  }
}

void ezGALPipelineCache::Clear()
{
  EZ_LOCK(m_Mutex);

  for (auto it = m_GraphicsPipelines.GetIterator(); it.IsValid(); ++it)
  {
    m_pDevice->DestroyGraphicsPipeline(it.Value());
  }
  m_GraphicsPipelines.Clear();

  for (auto it = m_ComputePipelines.GetIterator(); it.IsValid(); ++it)
  {
    m_pDevice->DestroyComputePipeline(it.Value());
  }
  m_ComputePipelines.Clear();
}

ezGALGraphicsPipelineHandle ezGALPipelineCache::GetPipeline(const ezGALGraphicsPipelineCreationDescription& description)
{
  ezGALPipelineCache* pCache = ezGALPipelineCache::GetSingleton();

  ezGALGraphicsPipelineHandle hGraphicsPipeline = pCache->TryGetPipeline<ezGALGraphicsPipelineHandle>(description, pCache->m_GraphicsPipelines);

  if (hGraphicsPipeline.IsInvalidated())
  {
    hGraphicsPipeline = pCache->m_pDevice->CreateGraphicsPipeline(description);
    if (hGraphicsPipeline.IsInvalidated())
    {
      return {};
    }

    if (pCache->TryInsertPipeline<ezGALGraphicsPipelineHandle>(description, hGraphicsPipeline, pCache->m_GraphicsPipelines).Failed())
    {
      // Already created and inserted, reduce ref count again.
      pCache->m_pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);
    }
  }

  return hGraphicsPipeline;
}

ezGALComputePipelineHandle ezGALPipelineCache::GetPipeline(const ezGALComputePipelineCreationDescription& description)
{
  ezGALPipelineCache* pCache = ezGALPipelineCache::GetSingleton();

  ezGALComputePipelineHandle hComputePipeline = pCache->TryGetPipeline<ezGALComputePipelineHandle>(description, pCache->m_ComputePipelines);

  if (hComputePipeline.IsInvalidated())
  {
    hComputePipeline = pCache->m_pDevice->CreateComputePipeline(description);
    if (hComputePipeline.IsInvalidated())
    {
      return {};
    }

    if (pCache->TryInsertPipeline<ezGALComputePipelineHandle>(description, hComputePipeline, pCache->m_ComputePipelines).Failed())
    {
      // Already created and inserted, reduce ref count again.
      pCache->m_pDevice->DestroyComputePipeline(hComputePipeline);
    }
  }

  return hComputePipeline;
}

ezUInt32 ezGALPipelineCache::CacheKeyHasher::Hash(const ezGALPipelineCache::GraphicsPipelineCacheKey& a)
{
  return a.m_uiHash;
}

bool ezGALPipelineCache::CacheKeyHasher::Equal(const ezGALPipelineCache::GraphicsPipelineCacheKey& a, const ezGALPipelineCache::GraphicsPipelineCacheKey& b)
{
  return a.m_uiHash == b.m_uiHash && a.m_Desc == b.m_Desc;
}

ezUInt32 ezGALPipelineCache::CacheKeyHasher::Hash(const ezGALPipelineCache::ComputePipelineCacheKey& a)
{
  return a.m_uiHash;
}

bool ezGALPipelineCache::CacheKeyHasher::Equal(const ezGALPipelineCache::ComputePipelineCacheKey& a, const ezGALPipelineCache::ComputePipelineCacheKey& b)
{
  return a.m_uiHash == b.m_uiHash && a.m_Desc == b.m_Desc;
}


EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_PipelineCache);
