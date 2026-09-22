#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class ezPlayerApplication : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  ezPlayerApplication();

protected:
  virtual ezResult BeforeCoreSystemsStartup() override;

private:
  /// Returns EZ_FAILURE if no project could be identified and m_sAppProjectPath was set to the SDK root instead.
  ezResult DetermineProjectPath();

  /// Appends the project name to the application name, so that every project gets its own ":appdata" folder.
  /// Must be called after DetermineProjectPath() and before the data directories are configured.
  void MakeApplicationNameProjectSpecific();
};
