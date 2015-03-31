#include <Core/PCH.h>
#include <Core/Application/Config/ApplicationConfig.h>

ezString ezApplicationConfig::s_sProjectDir;

void ezApplicationConfig::SetProjectDirectory(const char* szProjectDir)
{
  s_sProjectDir = szProjectDir;
}

const ezString& ezApplicationConfig::GetProjectDirectory()
{
  EZ_ASSERT_DEV(!s_sProjectDir.IsEmpty(), "The project directory has not been set through 'ezApplicationConfig::SetProjectDirectory'.");
  return s_sProjectDir;
}


