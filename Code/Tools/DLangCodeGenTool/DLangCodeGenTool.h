#include "CppStructure.h"
#include "DLangGenerator.h"
#include "RapidXML/rapidxml.hpp"

#include <Core/Scripting/LuaWrapper.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/CommandLineOptions.h>

using namespace rapidxml;

class DLangCodeGenTool : public ezApplication
{
public:
  typedef ezApplication SUPER;

  DLangCodeGenTool()
    : ezApplication("DLangCodeGenTool")
  {
  }

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual Execution Run() override;

  ezString m_sXmlFile;
  ezUniquePtr<DLangGenerator> m_pGenerator;

private:
  ezResult ParseArguments();

  CppStructure m_Structure;

private:
  void FindTargetFiles();
  void RemovePreviousAutogen();
  void CleanTargetFiles();
  void ReadFileContent(const char* szFile);
  void WriteFileContent(const char* szFile);
  void ProcessAllFiles();
  void ProcessFileCodeGen();
  void ProcessFileCommands();
  static int Lua_Struct(lua_State* context);
  static int Lua_Class(lua_State* context);
  static int Lua_Enum(lua_State* context);
  static int Lua_InterfaceStruct(lua_State* context);
  static int Lua_SynthesizeTemplate(lua_State* context);
  static int Lua_WhitelistType(lua_State* context);

  enum class Phase
  {
    GatherInfo,
    GenerateCode,
  };

  Phase m_Phase = Phase::GatherInfo;
  ezStringBuilder m_sCurFileContent;
  const char* m_szCurFileInsertPos = nullptr;
  ezSet<ezString> m_TargetFiles;
  ezLuaWrapper m_Lua;
};
