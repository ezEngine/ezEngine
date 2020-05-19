#include <RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/FileInterface.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

EZ_IMPLEMENT_SINGLETON(ezRmlUi);

ezRmlUi::ezRmlUi()
  : m_SingletonRegistrar(this)
{
  m_pFileInterface = EZ_DEFAULT_NEW(ezRmlUiInternal::FileInterface);
  Rml::Core::SetFileInterface(m_pFileInterface.Borrow());

  m_pSystemInterface = EZ_DEFAULT_NEW(ezRmlUiInternal::SystemInterface);
  Rml::Core::SetSystemInterface(m_pSystemInterface.Borrow());

  Rml::Core::Initialise();
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
