#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentGenerator, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAssetDocumentGenerator::ezAssetDocumentGenerator() = default;

ezAssetDocumentGenerator::~ezAssetDocumentGenerator() = default;

void ezAssetDocumentGenerator::AddSupportedFileType(ezStringView sExtension)
{
  ezStringBuilder tmp = sExtension;
  tmp.ToLower();

  m_SupportedFileTypes.PushBack(tmp);
}


void ezAssetDocumentGenerator::GetSupportedFileTypes(ezSet<ezString>& ref_extensions) const
{
  for (const ezString& ext : m_SupportedFileTypes)
  {
    ref_extensions.Insert(ext);
  }
}

bool ezAssetDocumentGenerator::SupportsFileType(ezStringView sFile) const
{
  ezStringBuilder tmp = ezPathUtils::GetFileExtension(sFile);
  tmp.ToLower();

  return m_SupportedFileTypes.Contains(tmp);
}

void ezAssetDocumentGenerator::BuildFileDialogFilterString(ezStringBuilder& out_sFilter) const
{
  bool semicolon = false;
  out_sFilter.Format("{0} (", GetDocumentExtension());
  AppendFileFilterStrings(out_sFilter, semicolon);
  out_sFilter.Append(")");
}

void ezAssetDocumentGenerator::AppendFileFilterStrings(ezStringBuilder& out_sFilter, bool& ref_bSemicolon) const
{
  for (const ezString& ext : m_SupportedFileTypes)
  {
    ezStringBuilder extWithStarDot;
    extWithStarDot.AppendFormat("*.{0}", ext);

    if (const char* pos = out_sFilter.FindSubString(extWithStarDot.GetData()))
    {
      const char afterExt = *(pos + extWithStarDot.GetElementCount());

      if (afterExt == '\0' || afterExt == ';')
        continue;
    }

    if (ref_bSemicolon)
    {
      out_sFilter.AppendFormat("; {0}", extWithStarDot.GetView());
    }
    else
    {
      out_sFilter.Append(extWithStarDot.GetView());
      ref_bSemicolon = true;
    }
  }
}

void ezAssetDocumentGenerator::CreateGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& out_Generators)
{
  ezRTTI::ForEachDerivedType<ezAssetDocumentGenerator>(
    [&](const ezRTTI* pRtti)
    {
      out_Generators.PushBack(pRtti->GetAllocator()->Allocate<ezAssetDocumentGenerator>());
    },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);

  // sort by name
  out_Generators.Sort([](ezAssetDocumentGenerator* lhs, ezAssetDocumentGenerator* rhs) -> bool
    { return lhs->GetDocumentExtension().Compare_NoCase(rhs->GetDocumentExtension()) < 0; });
}

void ezAssetDocumentGenerator::DestroyGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& generators)
{
  for (ezAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }

  generators.Clear();
}


void ezAssetDocumentGenerator::ExecuteImport(ezDynamicArray<ImportData>& ref_allImports)
{
  for (auto& data : ref_allImports)
  {
    if (data.m_iSelectedOption < 0)
      continue;

    EZ_LOG_BLOCK("Asset Import", data.m_sInputFileParentRelative);

    auto& option = data.m_ImportOptions[data.m_iSelectedOption];

    if (DetermineInputAndOutputFiles(data, option).Failed())
      continue;

    ezDocument* pGeneratedDoc = nullptr;
    const ezStatus status = option.m_pGenerator->Generate(data.m_sInputFileRelative, option, pGeneratedDoc);

    if (pGeneratedDoc)
    {
      pGeneratedDoc->SaveDocument(true).LogFailure();
      pGeneratedDoc->GetDocumentManager()->CloseDocument(pGeneratedDoc);

      ezQtEditorApp::GetSingleton()->OpenDocumentQueued(option.m_sOutputFileAbsolute);
    }

    if (status.Failed())
    {
      data.m_sImportMessage = status.m_sMessage;
      ezLog::Error("Asset import failed: '{0}'", status.m_sMessage);
    }
    else
    {
      data.m_sImportMessage.Clear();
      data.m_bDoNotImport = true;
      ezLog::Success("Generated asset document '{0}'", option.m_sOutputFileAbsolute);
    }
  }
}


ezResult ezAssetDocumentGenerator::DetermineInputAndOutputFiles(ImportData& data, Info& option)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder inputFile = data.m_sInputFileParentRelative;
  if (!pApp->MakeParentDataDirectoryRelativePathAbsolute(inputFile, true))
  {
    data.m_sImportMessage = "Input file could not be located";
    return EZ_FAILURE;
  }

  data.m_sInputFileAbsolute = inputFile;

  if (!pApp->MakePathDataDirectoryRelative(inputFile))
  {
    data.m_sImportMessage = "Input file is not in any known data directory";
    return EZ_FAILURE;
  }

  data.m_sInputFileRelative = inputFile;

  ezStringBuilder outputFile = option.m_sOutputFileParentRelative;
  if (!pApp->MakeParentDataDirectoryRelativePathAbsolute(outputFile, false))
  {
    data.m_sImportMessage = "Target file location could not be found";
    return EZ_FAILURE;
  }

  option.m_sOutputFileAbsolute = outputFile;

  // don't create it when it already exists
  if (ezOSFile::ExistsFile(outputFile))
  {
    data.m_bDoNotImport = true;
    data.m_sImportMessage = "Target file already exists";
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezAssetDocumentGenerator::ImportAssets(const ezDynamicArray<ezString>& filesToImport)
{
  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  ezDynamicArray<ezAssetDocumentGenerator::ImportData> allImports;
  allImports.Reserve(filesToImport.GetCount());

  CreateImportOptionList(filesToImport, allImports, generators);

  SortAndSelectBestImportOption(allImports);

  ezQtAssetImportDlg dlg(QApplication::activeWindow(), allImports);
  dlg.exec();

  DestroyGenerators(generators);
}

void ezAssetDocumentGenerator::GetSupportsFileTypes(ezSet<ezString>& out_extensions)
{
  out_extensions.Clear();

  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);
  for (auto pGen : generators)
  {
    pGen->GetSupportedFileTypes(out_extensions);
  }
  DestroyGenerators(generators);
}

void ezAssetDocumentGenerator::ImportAssets()
{
  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  ezStringBuilder singleFilter, fullFilter, allExtensions;
  bool semicolon = false;

  for (auto pGen : generators)
  {
    pGen->AppendFileFilterStrings(allExtensions, semicolon);
    pGen->BuildFileDialogFilterString(singleFilter);
    fullFilter.Append(singleFilter, "\n");
  }

  fullFilter.Append("All files (*.*)");
  fullFilter.Prepend("All asset files (", allExtensions, ")\n");

  static ezStringBuilder s_StartDir;
  if (s_StartDir.IsEmpty())
  {
    s_StartDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
  }

  QStringList filenames = QFileDialog::getOpenFileNames(QApplication::activeWindow(), "Import Assets", s_StartDir.GetData(),
    QString::fromUtf8(fullFilter.GetData()), nullptr, QFileDialog::Option::DontResolveSymlinks);

  DestroyGenerators(generators);

  if (filenames.empty())
    return;

  s_StartDir = filenames[0].toUtf8().data();
  s_StartDir.PathParentDirectory();

  ezHybridArray<ezString, 16> filesToImport;
  for (QString s : filenames)
  {
    filesToImport.PushBack(s.toUtf8().data());
  }

  ImportAssets(filesToImport);
}

void ezAssetDocumentGenerator::CreateImportOptionList(const ezDynamicArray<ezString>& filesToImport,
  ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports, const ezHybridArray<ezAssetDocumentGenerator*, 16>& generators)
{
  ezQtEditorApp* pApp = ezQtEditorApp::GetSingleton();
  ezStringBuilder sInputParentRelative, sInputRelative, sGroup;

  for (const ezString& sInputAbsolute : filesToImport)
  {
    sInputParentRelative = sInputAbsolute;
    sInputRelative = sInputAbsolute;

    if (!pApp->MakePathDataDirectoryParentRelative(sInputParentRelative) || !pApp->MakePathDataDirectoryRelative(sInputRelative))
    {
      auto& data = allImports.ExpandAndGetRef();
      data.m_sInputFileAbsolute = sInputAbsolute;
      data.m_sInputFileParentRelative = sInputParentRelative;
      data.m_sInputFileRelative = sInputRelative;
      data.m_sImportMessage = "File is not located in any data directory.";
      data.m_bDoNotImport = true;
      continue;
    }

    for (ezAssetDocumentGenerator* pGen : generators)
    {
      if (pGen->SupportsFileType(sInputParentRelative))
      {
        sGroup = pGen->GetGeneratorGroup();

        ImportData* pData = nullptr;
        for (auto& importer : allImports)
        {
          if (importer.m_sGroup == sGroup && importer.m_sInputFileAbsolute == sInputAbsolute)
          {
            pData = &importer;
          }
        }

        if (pData == nullptr)
        {
          pData = &allImports.ExpandAndGetRef();
          pData->m_sGroup = sGroup;
          pData->m_sInputFileAbsolute = sInputAbsolute;
          pData->m_sInputFileParentRelative = sInputParentRelative;
          pData->m_sInputFileRelative = sInputRelative;
        }

        ezHybridArray<ezAssetDocumentGenerator::Info, 4> options;
        pGen->GetImportModes(sInputParentRelative, options);

        for (auto& option : options)
        {
          option.m_pGenerator = pGen;
        }

        pData->m_ImportOptions.PushBackRange(options);
      }
    }
  }
}

void ezAssetDocumentGenerator::SortAndSelectBestImportOption(ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports)
{
  allImports.Sort([](const ezAssetDocumentGenerator::ImportData& lhs, const ezAssetDocumentGenerator::ImportData& rhs) -> bool
    { return lhs.m_sInputFileParentRelative < rhs.m_sInputFileParentRelative; });

  for (auto& singleImport : allImports)
  {
    singleImport.m_ImportOptions.Sort([](const ezAssetDocumentGenerator::Info& lhs, const ezAssetDocumentGenerator::Info& rhs) -> bool
      { return ezTranslate(lhs.m_sName).Compare_NoCase(ezTranslate(rhs.m_sName)) < 0; });

    ezUInt32 uiNumPrios[(ezUInt32)ezAssetDocGeneratorPriority::ENUM_COUNT] = {0};
    ezUInt32 uiBestPrio[(ezUInt32)ezAssetDocGeneratorPriority::ENUM_COUNT] = {0};

    for (ezUInt32 i = 0; i < singleImport.m_ImportOptions.GetCount(); ++i)
    {
      uiNumPrios[(ezUInt32)singleImport.m_ImportOptions[i].m_Priority]++;
      uiBestPrio[(ezUInt32)singleImport.m_ImportOptions[i].m_Priority] = i;
    }

    singleImport.m_iSelectedOption = -1;
    for (ezUInt32 prio = (ezUInt32)ezAssetDocGeneratorPriority::HighPriority; prio > (ezUInt32)ezAssetDocGeneratorPriority::Undecided; --prio)
    {
      if (uiNumPrios[prio] == 1)
      {
        singleImport.m_iSelectedOption = uiBestPrio[prio];
        break;
      }

      if (uiNumPrios[prio] > 1)
        break;
    }
  }
}
