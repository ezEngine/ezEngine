#include <PCH.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentGenerator, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

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

    out_Generators.PushBack(static_cast<ezAssetDocumentGenerator*>(pRtti->GetAllocator()->Allocate()));
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


void ezAssetDocumentGenerator::ExecuteImport(const ezDynamicArray<ImportData>& allImports)
{
  for (auto& data : allImports)
  {
    if (data.m_iSelectedOption < 0)
      continue;

    EZ_LOG_BLOCK("Asset Import", data.m_sInputFile);

    auto& option = data.m_ImportOptions[data.m_iSelectedOption];
    const ezStatus status = option.m_pGenerator->Generate(data.m_sInputFile, option);

    if (status.Failed())
    {
      ezLog::Error("Asset import failed: '{0}'", status.m_sMessage);
    }
    else
    {
      ezLog::Success("Generated asset document '{0}'", option.m_sOutputFile);
    }
  }
}

void ezAssetDocumentGenerator::ImportAssets(const ezHybridArray<ezString, 16>& filesToImport)
{
  ezDynamicArray<ezAssetDocumentGenerator::ImportData> allImports;
  allImports.Reserve(filesToImport.GetCount());

  ezQtEditorApp* pApp = ezQtEditorApp::GetSingleton();
  ezStringBuilder tmp;

  for (const ezString& s : filesToImport)
  {
    tmp = s;

    if (!pApp->MakePathDataDirectoryParentRelative(tmp))
      continue;

    auto& data = allImports.ExpandAndGetRef();
    data.m_sInputFile = tmp;
  }

  allImports.Sort([](const ezAssetDocumentGenerator::ImportData& lhs, const ezAssetDocumentGenerator::ImportData& rhs) -> bool
  {
    return lhs.m_sInputFile < rhs.m_sInputFile;
  });

  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  for (auto& singleImport : allImports)
  {
    for (ezAssetDocumentGenerator* pGen : generators)
    {
      if (pGen->SupportsFileType(singleImport.m_sInputFile))
      {
        pGen->GetImportModes(singleImport.m_sInputFile, singleImport.m_ImportOptions);
      }
    }

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

  ezQtAssetImportDlg dlg(QApplication::activeWindow(), allImports);
  if (dlg.exec() == QDialog::Accepted)
  {
    ExecuteImport(allImports);
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

  QStringList filenames = QFileDialog::getOpenFileNames(QApplication::activeWindow(), "Import Assets", ezToolsProject::GetSingleton()->GetProjectDirectory().GetData(), QString::fromUtf8(fullFilter.GetData()), nullptr, QFileDialog::Option::DontResolveSymlinks);

  DestroyGenerators(generators);

  if (filenames.empty())
    return;

  ezHybridArray<ezString, 16> filesToImport;
  for (QString s : filenames)
  {
    filesToImport.PushBack(s.toUtf8().data());
  }

  ImportAssets(filesToImport);
}
