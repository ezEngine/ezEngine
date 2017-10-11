#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

enum class ezAssetDocGeneratorPriority
{
  Undecided,
  LowPriority,
  DefaultPriority,
  HighPriority,
  ENUM_COUNT
};

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentGenerator : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentGenerator, ezReflectedClass);

public:
  ezAssetDocumentGenerator();
  ~ezAssetDocumentGenerator();

  static void ImportAssets(const ezHybridArray<ezString, 16>& filesToImport);

  struct Info
  {
    ezAssetDocumentGenerator* m_pGenerator = nullptr;
    ezAssetDocGeneratorPriority m_Priority;
    ezString m_sOutputFile;
    ezString m_sName;
    ezString m_sIcon;
  };


  struct ImportData
  {
    ezString m_sInputFile;
    ezInt32 m_iSelectedOption = -1;

    ezHybridArray<ezAssetDocumentGenerator::Info, 4> m_ImportOptions;
  };

  virtual void GetImportModes(const char* szPath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) = 0;
  virtual ezStatus Generate(const char* szPath, const ezAssetDocumentGenerator::Info& mode) = 0;
};

