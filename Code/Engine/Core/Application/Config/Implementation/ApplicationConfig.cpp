#include <Core/PCH.h>
#include <Core/Application/Config/ApplicationConfig.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezApplicationConfig, ezNoBase, 1, ezRTTINoAllocator );
EZ_END_STATIC_REFLECTED_TYPE();

ezString ezApplicationConfig::s_sProjectDir;

void ezApplicationConfig::SetProjectDirectory(const char* szProjectDir)
{
  s_sProjectDir = szProjectDir;
}

const char* ezApplicationConfig::GetProjectDirectory()
{
  EZ_ASSERT_DEV(!s_sProjectDir.IsEmpty(), "The project directory has not been set through 'ezApplicationConfig::SetProjectDirectory'.");
  return s_sProjectDir.GetData();
}


