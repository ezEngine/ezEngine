#include <RmlUiPluginPCH.h>

#include <RmlUiPlugin/RmlUiSingleton.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>

EZ_IMPLEMENT_SINGLETON(ezRmlUi);

ezRmlUi::ezRmlUi()
  : m_SingletonRegistrar(this)
{
  m_pSystemInterface = EZ_DEFAULT_NEW(ezRmlUiInternal::SystemInterface);
  Rml::Core::SetSystemInterface(m_pSystemInterface.Borrow());

  Rml::Core::Initialise();
}

ezRmlUi::~ezRmlUi()
{
  Rml::Core::Shutdown();
}
