#include <RmlUiPluginPCH.h>

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
#include <RmlUiPlugin/RmlUiContext.h>

ezRmlUiContext::ezRmlUiContext()
{
  m_pExtractor = EZ_DEFAULT_NEW(ezRmlUiInternal::Extractor);

  m_pContext = Rml::Core::CreateContext("Test", Rml::Core::Vector2i(400, 400), m_pExtractor.Borrow());
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

void ezRmlUiContext::ExtractRenderData(ezMsgExtractRenderData& msg)
{
  if (m_pDocument == nullptr)
    return;

  if (m_uiExtractedFrame != ezRenderWorld::GetFrameCounter())
  {
    EZ_LOCK(m_ExtractionMutex);

    if (m_uiExtractedFrame != ezRenderWorld::GetFrameCounter())
    {
      m_pExtractor->BeginExtraction();

      m_pContext->Render();

      m_pExtractor->EndExtraction();
    }
  }

  if (ezRenderData* pRenderData = m_pExtractor->GetRenderData())
  {
    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::GUI, ezRenderData::Caching::Never);
  }
}

Rml::Core::Context* ezRmlUiContext::GetRmlContext()
{
  return m_pContext;
}
