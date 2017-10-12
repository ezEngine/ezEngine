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

  static void ImportAssets();
  static void ImportAssets(const ezHybridArray<ezString, 16>& filesToImport);

  virtual void GetImportModes(const char* szPath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) = 0;
  virtual ezStatus Generate(const char* szPath, const ezAssetDocumentGenerator::Info& mode) = 0;
  virtual const char* GetDocumentExtension() const = 0;

  bool SupportsFileType(const char* szFile) const;
  void BuildFileDialogFilterString(ezStringBuilder& out_Filter) const;
  void AppendFileFilterStrings(ezStringBuilder& out_Filter, bool& semicolon) const;

protected:
  void AddSupportedFileType(const char* szExtension);

private:
  static void CreateGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& out_Generators);
  static void DestroyGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& generators);
  static void ExecuteImport(const ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports);

  ezHybridArray<ezString, 16> m_SupportedFileTypes;
};

