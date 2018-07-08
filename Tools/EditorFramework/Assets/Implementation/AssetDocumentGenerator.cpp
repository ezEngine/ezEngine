#include <PCH.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <QFileDialog>
#include <Foundation/IO/OSFile.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentGenerator, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAssetDocumentGenerator::ezAssetDocumentGenerator()
{
}

ezAssetDocumentGenerator::~ezAssetDocumentGenerator()
{
}

void ezAssetDocumentGenerator::AddSupportedFileType(const char* szExtension)
{
  ezStringBuilder tmp = szExtension;
  tmp.ToLower();

  m_SupportedFileTypes.PushBack(tmp);
}

bool ezAssetDocumentGenerator::SupportsFileType(const char* szFile) const
{
  ezStringBuilder tmp = ezPathUtils::GetFileExtension(szFile);
  tmp.ToLower();

  return m_SupportedFileTypes.Contains(tmp);
}

void ezAssetDocumentGenerator::BuildFileDialogFilterString(ezStringBuilder& out_Filter) const
{
  bool semicolon = false;
  out_Filter.Format("{0} (", GetDocumentExtension());
  AppendFileFilterStrings(out_Filter, semicolon);
  out_Filter.Append(")");
}

void ezAssetDocumentGenerator::AppendFileFilterStrings(ezStringBuilder& out_Filter, bool& semicolon) const
{
  for (const ezString ext : m_SupportedFileTypes)
  {
    if (semicolon)
    {
      out_Filter.AppendFormat("; *.{0}", ext);
    }
    else
    {
      out_Filter.AppendFormat("*.{0}", ext);
      semicolon = true;
    }
  }
}

void ezAssetDocumentGenerator::CreateGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& out_Generators)
{
  for (ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezAssetDocumentGenerator>() || !pRtti->GetAllocator()->CanAllocate())
      continue;

    out_Generators.PushBack(pRtti->GetAllocator()->Allocate<ezAssetDocumentGenerator>());
  }

  // sort by name
  out_Generators.Sort([](ezAssetDocumentGenerator* lhs, ezAssetDocumentGenerator* rhs) -> bool
  {
    return ezStringUtils::Compare_NoCase(lhs->GetDocumentExtension(), rhs->GetDocumentExtension()) < 0;
  });
}

void ezAssetDocumentGenerator::DestroyGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& generators)
{
  for (ezAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }

  generators.Clear();
}


void ezAssetDocumentGenerator::ExecuteImport(ezDynamicArray<ImportData>& allImports)
{
  for (auto& data : allImports)
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
      pGeneratedDoc->SaveDocument(true);
      pGeneratedDoc->GetDocumentManager()->CloseDocument(pGeneratedDoc);

      ezQtEditorApp::GetSingleton()->OpenDocument(option.m_sOutputFileAbsolute);
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

void ezAssetDocumentGenerator::ImportAssets(const ezHybridArray<ezString, 16>& filesToImport)
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

  QStringList filenames = QFileDialog::getOpenFileNames(QApplication::activeWindow(), "Import Assets", s_StartDir.GetData(), QString::fromUtf8(fullFilter.GetData()), nullptr, QFileDialog::Option::DontResolveSymlinks);

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

void ezAssetDocumentGenerator::CreateImportOptionList(const ezHybridArray<ezString, 16>& filesToImport, ezDynamicArray<ezAssetDocumentGenerator::ImportData> &allImports, const ezHybridArray<ezAssetDocumentGenerator *, 16>& generators)
{
  ezQtEditorApp* pApp = ezQtEditorApp::GetSingleton();
  ezStringBuilder sInputParentRelative, sInputRelative, sGroup;

  for (const ezString& sInputAbsolute : filesToImport)
  {
    sInputParentRelative = sInputAbsolute;
    sInputRelative = sInputAbsolute;

    if (!pApp->MakePathDataDirectoryParentRelative(sInputParentRelative) ||
      !pApp->MakePathDataDirectoryRelative(sInputRelative))
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
  {
    return lhs.m_sInputFileParentRelative < rhs.m_sInputFileParentRelative;
  });

  for (auto& singleImport : allImports)
  {

    singleImport.m_ImportOptions.Sort([](const ezAssetDocumentGenerator::Info& lhs, const ezAssetDocumentGenerator::Info& rhs) -> bool
    {
      return lhs.m_sName < rhs.m_sName;
    });

    ezUInt32 uiNumPrios[(ezUInt32)ezAssetDocGeneratorPriority::ENUM_COUNT] = { 0 };
    ezUInt32 uiBestPrio[(ezUInt32)ezAssetDocGeneratorPriority::ENUM_COUNT] = { 0 };

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
