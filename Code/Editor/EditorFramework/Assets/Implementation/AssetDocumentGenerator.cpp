#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/Assets/AssetProcessor.h>
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

bool ezAssetDocumentGenerator::SupportsFileType(ezStringView sFile) const
{
  ezStringBuilder tmp = ezPathUtils::GetFileExtension(sFile);

  if (tmp.IsEmpty())
    tmp = sFile;

  tmp.ToLower();

  return m_SupportedFileTypes.Contains(tmp);
}

void ezAssetDocumentGenerator::BuildFileDialogFilterString(ezStringBuilder& out_sFilter) const
{
  bool semicolon = false;
  out_sFilter.SetFormat("{0} (", GetDocumentExtension());
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

void ezAssetDocumentGenerator::CreateGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& out_generators)
{
  ezRTTI::ForEachDerivedType<ezAssetDocumentGenerator>(
    [&](const ezRTTI* pRtti)
    {
      out_generators.PushBack(pRtti->GetAllocator()->Allocate<ezAssetDocumentGenerator>());
    },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);

  // sort by name
  out_generators.Sort([](ezAssetDocumentGenerator* lhs, ezAssetDocumentGenerator* rhs) -> bool
    { return lhs->GetDocumentExtension().Compare_NoCase(rhs->GetDocumentExtension()) < 0; });
}

void ezAssetDocumentGenerator::DestroyGenerators(const ezHybridArray<ezAssetDocumentGenerator*, 16>& generators)
{
  for (ezAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }
}

void ezAssetDocumentGenerator::ImportAssets(const ezDynamicArray<ezString>& filesToImport)
{
  ezAssetProcessor::GetSingleton()->m_iPauseProcessing.Increment();
  EZ_SCOPE_EXIT(ezAssetProcessor::GetSingleton()->m_iPauseProcessing.Decrement());

  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions> allImports;
  allImports.Reserve(filesToImport.GetCount());

  CreateImportOptionList(filesToImport, allImports, generators);

  SortAndSelectBestImportOption(allImports);

  if (!allImports.IsEmpty())
  {
    ezQtAssetImportDlg dlg(QApplication::activeWindow(), allImports);
    dlg.exec();
  }

  DestroyGenerators(generators);
}

void ezAssetDocumentGenerator::GetSupportsFileTypes(ezSet<ezString>& out_extensions)
{
  out_extensions.Clear();

  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);
  for (auto pGen : generators)
  {
    for (const ezString& ext : pGen->m_SupportedFileTypes)
    {
      out_extensions.Insert(ext);
    }
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

void ezAssetDocumentGenerator::CreateImportOptionList(const ezDynamicArray<ezString>& filesToImport, ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions>& allImports, const ezHybridArray<ezAssetDocumentGenerator*, 16>& generators)
{
  ezQtEditorApp* pApp = ezQtEditorApp::GetSingleton();
  ezStringBuilder sInputRelative, sGroup;

  for (const ezString& sInputAbsolute : filesToImport)
  {
    sInputRelative = sInputAbsolute;

    if (!pApp->MakePathDataDirectoryRelative(sInputRelative))
    {
      // error, file is not in data directory -> skip
      continue;
    }

    for (ezAssetDocumentGenerator* pGen : generators)
    {
      if (pGen->SupportsFileType(sInputRelative))
      {
        sGroup = pGen->GetGeneratorGroup();

        ImportGroupOptions* pData = nullptr;
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
          pData->m_sInputFileRelative = sInputRelative;
        }

        ezHybridArray<ezAssetDocumentGenerator::ImportMode, 4> options;
        pGen->GetImportModes(sInputAbsolute, options);

        for (auto& option : options)
        {
          option.m_pGenerator = pGen;
        }

        pData->m_ImportOptions.PushBackRange(options);
      }
    }
  }
}

void ezAssetDocumentGenerator::SortAndSelectBestImportOption(ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions>& allImports)
{
  allImports.Sort([](const ezAssetDocumentGenerator::ImportGroupOptions& lhs, const ezAssetDocumentGenerator::ImportGroupOptions& rhs) -> bool
    { return lhs.m_sInputFileRelative < rhs.m_sInputFileRelative; });

  for (auto& singleImport : allImports)
  {
    singleImport.m_ImportOptions.Sort([](const ezAssetDocumentGenerator::ImportMode& lhs, const ezAssetDocumentGenerator::ImportMode& rhs) -> bool
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

ezStatus ezAssetDocumentGenerator::Import(ezStringView sInputFileAbs, ezStringView sMode, bool bOpenDocument)
{
  ezStringBuilder ext = sInputFileAbs.GetFileExtension();
  ext.ToLower();

  if (!m_SupportedFileTypes.Contains(ext))
    return ezStatus(ezFmt("Files of type '{}' cannot be imported as '{}' documents.", ext, GetDocumentExtension()));

  ezHybridArray<ezDocument*, 16> pGeneratedDocs;
  EZ_SUCCEED_OR_RETURN(Generate(sInputFileAbs, sMode, pGeneratedDocs));

  for (ezDocument* pDoc : pGeneratedDocs)
  {
    const ezString sDocPath = pDoc->GetDocumentPath();

    pDoc->SaveDocument(true).LogFailure();
    pDoc->GetDocumentManager()->CloseDocument(pDoc);

    if (bOpenDocument)
    {
      ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sDocPath);
    }
  }

  return ezStatus(EZ_SUCCESS);
}
