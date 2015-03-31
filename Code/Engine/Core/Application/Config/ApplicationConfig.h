#pragma once

#include <Core/Basics.h>
#include <Foundation/Strings/String.h>

class EZ_CORE_DLL ezApplicationConfig
{
public:

  static void SetProjectDirectory(const char* szProjectDir);
  static const ezString& GetProjectDirectory();

  virtual ezResult Save() = 0;
  virtual void Load() = 0;

  virtual void Apply() = 0;

private:
  static ezString s_sProjectDir;
};
