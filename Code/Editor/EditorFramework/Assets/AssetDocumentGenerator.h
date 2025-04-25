#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Status.h>

class ezDocument;

enum class ezAssetDocGeneratorPriority
{
  Undecided,
  LowPriority,
  DefaultPriority,
  HighPriority,
  ENUM_COUNT
};

/// \brief Provides functionality for importing files as asset documents.
///
/// Derived from this class to add a custom importer (see existing derived classes for examples).
/// Each importer typically handles one target asset type.
class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentGenerator : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentGenerator, ezReflectedClass);

public:
  ezAssetDocumentGenerator();
  ~ezAssetDocumentGenerator();

  /// \brief Describes one option to import an asset.
  ///
  /// The name is used to identify which option the user chose.
  /// The priority should be set to pick a 'likely' option for the UI to prefer.
  struct ImportMode
  {
    ezAssetDocumentGenerator* m_pGenerator = nullptr; ///< automatically set by ezAssetDocumentGenerator
    ezAssetDocGeneratorPriority m_Priority = ezAssetDocGeneratorPriority::Undecided;
    ezString m_sName;
    ezString m_sIcon;
  };

  /// \brief Creates a list of all importable file extensions. Note that this is an expensive function so the the result should be cached.
  /// \param out_Extensions List of all file extensions that can be imported.
  static void GetSupportsFileTypes(ezSet<ezString>& out_extensions);

  /// \brief Opens a file browse dialog to let the user choose which files to import.
  ///
  /// After the user chose one or multiple files, opens the "Asset Import" dialog to let them choose details.
  static void ImportAssets();

  /// \brief Opens the "Asset Import" dialog to let the user choose how to import the given files.
  static void ImportAssets(const ezDynamicArray<ezString>& filesToImport);

  /// \brief Imports the given file with the mode. Must be a mode that the generator supports.
  ezStatus Import(ezStringView sInputFileAbs, ezStringView sMode, bool bOpenDocument);

  /// \brief Used to fill out which import modes may be available for the given asset.
  ///
  /// Note: sAbsInputFile may be empty, in this case it should fill out the array for "general purpose" import (any file of the supported types).
  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ImportMode>& out_modes) const = 0;

  /// \brief Returns the target asset document file extension.
  virtual ezStringView GetDocumentExtension() const = 0;

  /// \brief Allows to merge the import modes of multiple generators in the UI in one group.
  ///
  /// Not really used anymore, but theoretically allows to show the same file multiple times in the import dialog,
  /// such that one can import it as multiple different asset types.
  virtual ezStringView GetGeneratorGroup() const = 0;

  /// \brief Tells the generator to create a new asset document with the chosen mode.
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) = 0;

  /// \brief Returns whether this generator supports the given file type for import.
  bool SupportsFileType(ezStringView sFile) const;

  /// \brief Instantiates all currently available generators.
  static void CreateGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& out_generators);

  /// \brief Destroys the previously instantiated generators.
  static void DestroyGenerators(const ezHybridArray<ezAssetDocumentGenerator*, 16>& generators);

protected:
  void AddSupportedFileType(ezStringView sExtension);

private:
  void BuildFileDialogFilterString(ezStringBuilder& out_sFilter) const;
  void AppendFileFilterStrings(ezStringBuilder& out_sFilter, bool& ref_bSemicolon) const;

  friend class ezQtAssetImportDlg;

  struct ImportGroupOptions
  {
    ezString m_sGroup;
    ezString m_sInputFileRelative;
    ezString m_sInputFileAbsolute;
    ezInt32 m_iSelectedOption = -1;

    ezHybridArray<ezAssetDocumentGenerator::ImportMode, 4> m_ImportOptions;
  };

  static void SortAndSelectBestImportOption(ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions>& allImports);
  static void CreateImportOptionList(const ezDynamicArray<ezString>& filesToImport, ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions>& allImports, const ezHybridArray<ezAssetDocumentGenerator*, 16>& generators);

  ezHybridArray<ezString, 16> m_SupportedFileTypes;
};
