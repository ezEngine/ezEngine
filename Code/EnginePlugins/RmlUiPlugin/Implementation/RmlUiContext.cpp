#include <RmlUiPluginPCH.h>

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
#include <RmlUiPlugin/RmlUiContext.h>

ezRmlUiContext::ezRmlUiContext(const char* szName, const ezVec2U32& initialSize)
{
  m_pContext = Rml::Core::CreateContext(szName, Rml::Core::Vector2i(initialSize.x, initialSize.y));
}

ezRmlUiContext::~ezRmlUiContext()
{
  Rml::Core::RemoveContext(m_pContext->GetName());
}

ezResult ezRmlUiContext::LoadDocumentFromFile(const char* szFile)
{
  if (m_pDocument != nullptr)
  {
    m_pContext->UnloadDocument(m_pDocument);
    m_pDocument = nullptr;
  }

  if (ezStringUtils::IsNullOrEmpty(szFile) == false)
  {
    if (m_pDocument = m_pContext->LoadDocument(szFile))
    {
      m_pDocument->Show();
    }
  }

  return m_pDocument != nullptr ? EZ_SUCCESS : EZ_FAILURE;
}

void ezRmlUiContext::Update()
{
  if (m_pDocument != nullptr)
  {
    m_pContext->Update();
  }
}

void ezRmlUiContext::SetOffset(const ezVec2I32& offset)
{
  m_Offset = offset;
}

void ezRmlUiContext::SetSize(const ezVec2U32& size)
{
  if (m_pDocument != nullptr)
  {
    m_pContext->SetDimensions(Rml::Core::Vector2i(size.x, size.y));
  }
}

void ezRmlUiContext::SetDpiScale(float fScale)
{
  if (m_pDocument != nullptr)
  {
    m_pContext->SetDensityIndependentPixelRatio(fScale);
  }
}

Rml::Core::Context* ezRmlUiContext::GetRmlContext()
{
  return m_pContext;
}

void ezRmlUiContext::ExtractRenderData(ezRmlUiInternal::Extractor& extractor)
{
  if (m_uiExtractedFrame != ezRenderWorld::GetFrameCounter())
  {
    extractor.BeginExtraction(m_Offset);

    m_pContext->Render();

    extractor.EndExtraction();

    m_uiExtractedFrame = ezRenderWorld::GetFrameCounter();
    m_pRenderData = extractor.GetRenderData();
  }
}
