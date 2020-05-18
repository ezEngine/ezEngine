#include <RmlUiPluginPCH.h>

#include <RmlUiPlugin/RmlUi.h>

EZ_IMPLEMENT_SINGLETON(ezRmlUi);

ezRmlUi::ezRmlUi()
  : m_SingletonRegistrar(this)
{




  Rml::Core::Initialise();
}

ezRmlUi::~ezRmlUi()
{
  Rml::Core::Shutdown();
}
