#pragma once

#include <DLangPlugin/DLangPluginDLL.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Types.h>

class EZ_DLANGPLUGIN_DLL ezDLangCompiler
{
public:
  ezResult Initialize();

  void SetOutputBinary(const char* szOutputBin);

  ezResult CompileProjectLib();

private:
  ezResult FindDmd();
  bool DetectSources();

  bool m_bInitialized = false;
  ezString m_sDmdPath;
  ezSet<ezString> m_SourceFiles;
  ezString m_sOutputBinary;
};
