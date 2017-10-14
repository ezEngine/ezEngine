#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Status.h>

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

  struct Info
  {
    ezAssetDocumentGenerator* m_pGenerator = nullptr;
    ezAssetDocGeneratorPriority m_Priority;
    ezString m_sOutputFileParentRelative;
    ezString m_sOutputFileAbsolute;
    ezString m_sName;
    ezString m_sIcon;
  };

  struct ImportData
  {
    ezString m_sInputFileRelative;
    ezString m_sInputFileParentRelative;
    ezString m_sInputFileAbsolute;
    ezInt32 m_iSelectedOption = -1;
    ezString m_sImportError; // error text
    bool m_bDoNotImport = false;

    ezHybridArray<ezAssetDocumentGenerator::Info, 4> m_ImportOptions;
  };

  static void ImportAssets();
  static void ImportAssets(const ezHybridArray<ezString, 16>& filesToImport);
  static void ExecuteImport(ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports);

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) = 0;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& mode, ezDocument*& out_pGeneratedDocument) = 0;
  virtual const char* GetDocumentExtension() const = 0;

  bool SupportsFileType(const char* szFile) const;
  void BuildFileDialogFilterString(ezStringBuilder& out_Filter) const;
  void AppendFileFilterStrings(ezStringBuilder& out_Filter, bool& semicolon) const;

protected:
  void AddSupportedFileType(const char* szExtension);

private:
  static void CreateGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& out_Generators);
  static void DestroyGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& generators);
  static ezResult DetermineInputAndOutputFiles(ImportData& data, Info& option);

  ezHybridArray<ezString, 16> m_SupportedFileTypes;
};

