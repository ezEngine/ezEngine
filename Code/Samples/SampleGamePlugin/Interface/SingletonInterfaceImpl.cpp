#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/Interface/SingletonInterface.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>

// BEGIN-DOCS-CODE-SNIPPET: singleton-impl-definition
EZ_IMPLEMENT_SINGLETON(PrintImplementation);

PrintImplementation::PrintImplementation()
  : m_SingletonRegistrar(this) // needed for automatic registration
{
}
// END-DOCS-CODE-SNIPPET

void PrintImplementation::Print(const ezFormatString& text)
{
  ezStringBuilder tmp;
  const char* szFormattedText = text.GetText(tmp);

  ezLog::Info(szFormattedText);
}
