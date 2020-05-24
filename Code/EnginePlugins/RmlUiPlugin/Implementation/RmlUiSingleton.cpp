#include <RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/FileInterface.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

EZ_IMPLEMENT_SINGLETON(ezRmlUi);

ezRmlUi::ezRmlUi()
  : m_SingletonRegistrar(this)
{
  m_pExtractor = EZ_DEFAULT_NEW(ezRmlUiInternal::Extractor);
  Rml::Core::SetRenderInterface(m_pExtractor.Borrow());

  m_pFileInterface = EZ_DEFAULT_NEW(ezRmlUiInternal::FileInterface);
  Rml::Core::SetFileInterface(m_pFileInterface.Borrow());

  m_pSystemInterface = EZ_DEFAULT_NEW(ezRmlUiInternal::SystemInterface);
  Rml::Core::SetSystemInterface(m_pSystemInterface.Borrow());

  Rml::Core::Initialise();

  Rml::Core::LoadFontFace("UI/Raleway-Regular.ttf");
}

ezRmlUi::~ezRmlUi()
{
  Rml::Core::Shutdown();
}

ezRmlUiContext* ezRmlUi::CreateContext()
{
  m_Contexts.PushBack(EZ_DEFAULT_NEW(ezRmlUiContext));

  return m_Contexts.PeekBack().Borrow();
}

void ezRmlUi::DeleteContext(ezRmlUiContext* pContext)
{
  for (ezUInt32 i = 0; i < m_Contexts.GetCount(); ++i)
  {
    if (m_Contexts[i] == pContext)
    {
      m_Contexts.RemoveAtAndCopy(i);
      break;
    }
  }
}

void ezRmlUi::ExtractContext(ezRmlUiContext& context, ezMsgExtractRenderData& msg)
{
  if (context.m_pDocument == nullptr)
    return;

  // Unfortunately we need to hold a lock for the whole extraction of a context since RmlUi is not thread safe.
  EZ_LOCK(m_ExtractionMutex);

  if (context.m_uiExtractedFrame != ezRenderWorld::GetFrameCounter())
  {
    m_pExtractor->BeginExtraction();

    context.m_pContext->Render();

    m_pExtractor->EndExtraction();

    context.m_uiExtractedFrame = ezRenderWorld::GetFrameCounter();
    context.m_pRenderData = m_pExtractor->GetRenderData();
  }

  if (context.m_pRenderData != nullptr)
  {
    msg.AddRenderData(context.m_pRenderData, ezDefaultRenderDataCategories::GUI, ezRenderData::Caching::Never);
  }
}
