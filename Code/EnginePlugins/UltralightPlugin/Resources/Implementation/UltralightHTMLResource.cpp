#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>

#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/Textures/TextureUtils.h>

#include <UltralightPlugin/Resources/UltralightHTMLResource.h>
#include <UltralightPlugin/Integration/GPUDriverEz.h>

extern ultralight::RefPtr<ultralight::Renderer> s_renderer;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUltralightHTMLResource, 1, ezRTTIDefaultAllocator<ezUltralightHTMLResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezUltralightHTMLResource::ezUltralightHTMLResource()
    : ezTexture2DResource()
{
  ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(ezMakeDelegate(&ezUltralightHTMLResource::HACK_UPDATE_METHOD, this));
}

ezUltralightHTMLResource::~ezUltralightHTMLResource()
{
  ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezUltralightHTMLResource::HACK_UPDATE_METHOD, this));
}


void ezUltralightHTMLResource::OnBeginLoading(ultralight::View* caller)
{
}

void ezUltralightHTMLResource::OnFinishLoading(ultralight::View* caller)
{
}

void ezUltralightHTMLResource::OnUpdateHistory(ultralight::View* caller)
{
}

void ezUltralightHTMLResource::OnDOMReady(ultralight::View* caller)
{
  m_hGALTexture[0] = static_cast<ezUltralightGPUDriver*>(ultralight::Platform::instance().gpu_driver())->GetTextureHandleForTextureId(caller->render_target().texture_id);
}


void ezUltralightHTMLResource::OnChangeTitle(ultralight::View* caller, const ultralight::String& title)
{
}

void ezUltralightHTMLResource::OnChangeURL(ultralight::View* caller, const ultralight::String& url)
{
}

void ezUltralightHTMLResource::OnChangeTooltip(ultralight::View* caller, const ultralight::String& tooltip)
{
}

void ezUltralightHTMLResource::OnChangeCursor(ultralight::View* caller, ultralight::Cursor cursor)
{
}

void ezUltralightHTMLResource::OnAddConsoleMessage(ultralight::View* caller, ultralight::MessageSource source, ultralight::MessageLevel level, const ultralight::String& message, uint32_t line_number, uint32_t column_number, const ultralight::String& source_id)
{
}

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezUltralightHTMLResource);

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezUltralightHTMLResource, ezUltralightHTMLResourceDescriptor)
{
  ezResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = ezResourceState::Loaded;

  m_View = s_renderer->CreateView(descriptor.m_uiWidth, descriptor.m_uiHeight, false);
  m_View->set_load_listener(this);
  m_View->set_view_listener(this);

  m_View->Reload();

  if (!descriptor.m_sHTMLContent.IsEmpty())
  {
    m_View->LoadHTML(descriptor.m_sHTMLContent.GetData());
    m_View->set_needs_paint(true);
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto renderTarget = m_View->render_target();

  m_hGALTexture[m_uiLoadedTextures] = static_cast<ezUltralightGPUDriver*>(ultralight::Platform::instance().gpu_driver())->GetTextureHandleForTextureId(renderTarget.texture_id);
  m_uiLoadedTextures++;

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  ezGALSamplerStateCreationDescription SamplerDesc;

  m_hSamplerState = pDevice->CreateSamplerState(SamplerDesc);
  EZ_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  m_Type = ezGALTextureType::Texture2D;
  m_Format = ezGALResourceFormat::BGRAUByteNormalized;
  m_uiWidth = descriptor.m_uiWidth;
  m_uiHeight = descriptor.m_uiHeight;

  return ret;
}

ezResourceLoadDesc ezUltralightHTMLResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 1;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezUltralightHTMLResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezUltralightHTMLResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  ezUltralightHTMLResourceDescriptor desc;

  desc.Load(*Stream);

  m_uiLoadedTextures = 0;
  CreateResource(std::move(desc));

  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezUltralightHTMLResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0; // TODO
}

void ezUltralightHTMLResource::HACK_UPDATE_METHOD(const ezGALDeviceEvent& event)
{
  if (event.m_Type == ezGALDeviceEvent::BeforeBeginFrame)
  {
    if (m_View.get() != nullptr)
    {
      m_View->set_needs_paint(true);
      m_hGALTexture[0] = static_cast<ezUltralightGPUDriver*>(ultralight::Platform::instance().gpu_driver())->GetTextureHandleForTextureId(m_View->render_target().texture_id);
    }
  }
}

void ezUltralightHTMLResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_sHTMLContent;
  stream << m_sHTMLFileName;

  stream << m_uiWidth;
  stream << m_uiHeight;
}

void ezUltralightHTMLResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid ultralight HTML descriptor version {0}", uiVersion);

  stream >> m_sHTMLContent;
  stream >> m_sHTMLFileName;

  stream >> m_uiWidth;
  stream >> m_uiHeight;
}

EZ_STATICLINK_FILE(UltralightPlugin, UltralightPlugin_Resources_UltralightHTMLResource);
