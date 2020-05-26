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

ezRmlUiContext* ezRmlUi::CreateContext(const char* szName, const ezVec2U32& initialSize)
{
  m_Contexts.PushBack(EZ_DEFAULT_NEW(ezRmlUiContext, szName, initialSize));

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

  context.ExtractRenderData(*m_pExtractor);

  if (context.m_pRenderData != nullptr)
  {
    msg.AddRenderData(context.m_pRenderData, ezDefaultRenderDataCategories::GUI, ezRenderData::Caching::Never);
  }
}
