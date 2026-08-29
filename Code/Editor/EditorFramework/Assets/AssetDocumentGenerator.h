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

/// Provides functionality for importing files as asset documents.
///
/// Derived from this class to add a custom importer (see existing derived classes for examples).
/// Each importer typically handles one target asset type.
class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentGenerator : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentGenerator, ezReflectedClass);

public:
  ezAssetDocumentGenerator();
  ~ezAssetDocumentGenerator();

  /// Describes one option to import an asset.
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

  enum class ImportResult
  {
    Imported,      ///< A new asset document was created.
    AlreadyExists, ///< An asset for this source file already existed. Nothing was created or changed.
  };

  /// Creates a list of all importable file extensions. Note that this is an expensive function so the the result should be cached.
  /// \param out_Extensions List of all file extensions that can be imported.
  static void GetSupportsFileTypes(ezSet<ezString>& out_extensions);

  /// Opens a file browse dialog to let the user choose which files to import.
  ///
  /// After the user chose one or multiple files, opens the "Asset Import" dialog to let them choose details.
  static void ImportAssets();

  /// Opens the "Asset Import" dialog to let the user choose how to import the given files.
  static void ImportAssets(const ezDynamicArray<ezString>& filesToImport);

  /// Imports the given file with the mode. Must be a mode that the generator supports.
  ///
  /// Importing a file that was imported before is not an error, it does nothing and reports
  /// ImportResult::AlreadyExists. Distinguishing the two cases is only possible through out_pResult,
  /// the returned status is a success either way.
  ///
  /// \param out_pDocumentPath The document that was created, or the one that already existed. Also set
  ///        when generating fails, in which case it is the path that would have been used. Left alone
  ///        only when the file type isn't supported at all, since then there is no target path.
  ezStatus Import(ezStringView sInputFileAbs, ezStringView sMode, bool bOpenDocument, ImportResult* out_pResult = nullptr,
    ezStringBuilder* out_pDocumentPath = nullptr);

  /// Returns the path of the document that importing the given file would produce.
  ///
  /// The source path with GetDocumentExtension() substituted, which is the convention every generator
  /// follows. A generator that creates several documents from one file returns the primary one here.
  /// For modes that derive the document names from the file's content, this path may never be created.
  ezStringBuilder GetImportTargetPath(ezStringView sInputFileAbs) const;

  /// Whether Import() should run Generate() at all.
  ///
  /// The default returns false if GetImportTargetPath() already exists, which is the correct answer
  /// for every generator that turns one source file into one document.
  ///
  /// Override this for modes where one source file produces multiple documents: there the primary
  /// path existing says nothing about whether the others do, so the generator has to decide, or
  /// return true and skip individual documents while generating.
  virtual bool NeedsImport(ezStringView sInputFileAbs, ezStringView sMode) const;

  /// Used to fill out which import modes may be available for the given asset.
  ///
  /// Note: sAbsInputFile may be empty, in this case it should fill out the array for "general purpose" import (any file of the supported types).
  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ImportMode>& out_modes) const = 0;

  /// Returns the target asset document file extension.
  virtual ezStringView GetDocumentExtension() const = 0;

  /// Allows to merge the import modes of multiple generators in the UI in one group.
  ///
  /// Not really used anymore, but theoretically allows to show the same file multiple times in the import dialog,
  /// such that one can import it as multiple different asset types.
  virtual ezStringView GetGeneratorGroup() const = 0;

  /// Tells the generator to create a new asset document with the chosen mode.
  ///
  /// Import() only calls this when NeedsImport() said there is something to do, so an implementation
  /// that maps one file to one document doesn't have to check whether its target already exists.
  /// Returning success with an empty array means nothing was left to create, which Import() reports
  /// as ImportResult::AlreadyExists.
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) = 0;

  /// Returns whether this generator supports the given file type for import.
  bool SupportsFileType(ezStringView sFile) const;

  /// Instantiates all currently available generators.
  static void CreateGenerators(ezDynamicArray<ezAssetDocumentGenerator*>& out_generators);

  /// Destroys the previously instantiated generators.
  static void DestroyGenerators(const ezDynamicArray<ezAssetDocumentGenerator*>& generators);

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
  static void CreateImportOptionList(const ezDynamicArray<ezString>& filesToImport, ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions>& allImports, const ezDynamicArray<ezAssetDocumentGenerator*>& generators);

  ezHybridArray<ezString, 16> m_SupportedFileTypes;
};
