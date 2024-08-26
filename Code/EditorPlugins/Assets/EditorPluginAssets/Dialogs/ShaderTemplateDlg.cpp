#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Dialogs/ShaderTemplateDlg.moc.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <RendererCore/Shader/ShaderHelper.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezQtShaderTemplateDlg::ezQtShaderTemplateDlg(QWidget* pParent, const ezDocument* pSceneDoc)
  : QDialog(pParent)
{
  setupUi(this);

  ezStringBuilder tmp;
  ezStringBuilder sSearchDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("ShaderTemplates/*.ezShaderTemplate");

  ezFileSystemIterator it;
  for (it.StartSearch(sSearchDir, ezFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    auto& t = m_Templates.ExpandAndGetRef();

    it.GetStats().GetFullPath(tmp);
    t.m_sPath = tmp;

    tmp = it.GetStats().m_sName;
    tmp.RemoveFileExtension();
    t.m_sName = tmp;

    ezFileReader file;
    if (file.Open(t.m_sPath).Succeeded())
    {
      ezStringBuilder content;
      content.ReadAll(file);

      ezShaderHelper::ezTextSectionizer sec;
      ezShaderHelper::GetShaderSections(content, sec);

      ezUInt32 uiFirstLine = 0;
      ezStringBuilder vars = sec.GetSectionContent(ezShaderHelper::ezShaderSections::TEMPLATE_VARS, uiFirstLine);

      content.ReplaceAll(vars, "");
      content.ReplaceAll("[TEMPLATE_VARS]", "");

      t.m_sContent = content;

      vars.Split(false, t.m_Vars, "\n", "\r");

      for (ezUInt32 ip1 = t.m_Vars.GetCount(); ip1 > 0; --ip1)
      {
        const ezUInt32 i = ip1 - 1;

        ezStringBuilder s = t.m_Vars[i];
        s.Trim(" \t");
        t.m_Vars[i] = s;

        if (s.StartsWith("//"))
        {
          t.m_Vars.RemoveAtAndCopy(i);
        }
      }
    }

    ShaderTemplate->addItem(t.m_sName.GetData());
  }

  ShaderTemplate->setCurrentIndex(0);
}

void ezQtShaderTemplateDlg::on_Buttons_accepted()
{
  int idx = ShaderTemplate->currentIndex();
  if (idx < 0 || idx >= (int)m_Templates.GetCount())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("No shader template selected.");
    return;
  }

  ezStringBuilder relPath = DestinationFile->text().toUtf8().data();
  ezStringBuilder absPath = relPath;

  if (!ezQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(absPath, false))
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("The selected shader file path is invalid.");
    return;
  }

  relPath = absPath;

  if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(relPath))
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("The target shader file is not located inside a data directory of this project.");
    return;
  }

  ezFileWriter fileOut;
  if (fileOut.Open(absPath).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("Could not create target shader file.");
    return;
  }

  ezStringBuilder code = m_Templates[idx].m_sContent;

  // escape to $
  {
    code.ReplaceAll("#if", "$if"); // #if / #ifdef
    code.ReplaceAll("#endif", "$endif");
    code.ReplaceAll("#else", "$else");
    code.ReplaceAll("#elif", "$elif");
    code.ReplaceAll("#define", "$define");
    code.ReplaceAll("#include", "$include");
  }

  // make processable
  {
    code.ReplaceAll("%if", "#if"); // #if / #ifdef
    code.ReplaceAll("%endif", "#endif");
    code.ReplaceAll("%else", "#else");
    code.ReplaceAll("%elif", "#elif");
    code.ReplaceAll("%define", "#define");
    code.ReplaceAll("%include", "#include");
  }

  ezPreprocessor pp;
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.SetFileOpenFunction(
    [code](ezStringView sAbsoluteFile, ezDynamicArray<ezUInt8>& ref_fileContent, ezTimestamp& out_fileModification)
    {
      ref_fileContent.SetCountUninitialized(code.GetElementCount());
      ezMemoryUtils::RawByteCopy(ref_fileContent.GetData(), code.GetData(), code.GetElementCount());
      return EZ_SUCCESS;
    });

  QTableWidget* pTable = TemplateVars;

  for (ezUInt32 row = 0; row < m_Templates[idx].m_Vars.GetCount(); ++row)
  {
    ezVariant defVal;
    ezShaderParser::EnumDefinition enumDef;
    ezShaderParser::ParsePermutationVarConfig(m_Templates[idx].m_Vars[row], defVal, enumDef);

    if (defVal.IsA<bool>())
    {
      if (auto pWidget = qobject_cast<QCheckBox*>(pTable->cellWidget(row, 1)))
      {
        if (pWidget->checkState() == Qt::CheckState::Checked)
        {
          pp.AddCustomDefine(enumDef.m_sName).IgnoreResult();
        }
      }
    }
    else
    {
      ezStringBuilder tmp;
      for (const auto& ed : enumDef.m_Values)
      {
        tmp.SetFormat("{} {}", ed.m_sValueName, ed.m_iValueValue);
        pp.AddCustomDefine(tmp).IgnoreResult();
      }

      if (auto pWidget = qobject_cast<QComboBox*>(pTable->cellWidget(row, 1)))
      {
        tmp.SetFormat("{} {}", enumDef.m_sName, enumDef.m_Values[pWidget->currentIndex()].m_iValueValue);
        pp.AddCustomDefine(tmp).IgnoreResult();
      }
    }
  }

  if (pp.Process("<main>", code).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("Preparing the shader failed.");
    return;
  }

  // return to normal
  {
    code.ReplaceAll("$if", "#if"); // #if / #ifdef
    code.ReplaceAll("$endif", "#endif");
    code.ReplaceAll("$else", "#else");
    code.ReplaceAll("$elif", "#elif");
    code.ReplaceAll("$define", "#define");
    code.ReplaceAll("$include", "#include");
  }

  fileOut.WriteBytes(code.GetData(), code.GetElementCount()).IgnoreResult();

  m_sResult = relPath;

  accept();
}

void ezQtShaderTemplateDlg::on_Buttons_rejected()
{
  m_sResult.Clear();
  reject();
}

void ezQtShaderTemplateDlg::on_Browse_clicked()
{
  static QString sLastDir;
  if (sLastDir.isEmpty())
  {
    sLastDir = ezToolsProject::GetSingleton()->GetProjectDirectory().GetData();
  }

  QString sResult = QFileDialog::getSaveFileName(this, "Create Shader", sLastDir, "ezShader (*.ezShader)", nullptr);

  if (sResult.isEmpty())
    return;

  ezStringBuilder tmp = sResult.toUtf8().data();
  if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(tmp))
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("The selected file is not located inside a data directory of this project.");
    return;
  }

  sLastDir = sResult;
  DestinationFile->setText(tmp.GetData());
}

void ezQtShaderTemplateDlg::on_ShaderTemplate_currentIndexChanged(int idx)
{
  QTableWidget* pTable = TemplateVars;

  pTable->clear();
  pTable->setColumnCount(2);
  pTable->setRowCount(m_Templates[idx].m_Vars.GetCount());

  for (ezUInt32 row = 0; row < m_Templates[idx].m_Vars.GetCount(); ++row)
  {
    ezVariant defVal;
    ezShaderParser::EnumDefinition enumDef;
    ezShaderParser::ParsePermutationVarConfig(m_Templates[idx].m_Vars[row], defVal, enumDef);

    ezStringBuilder varName = enumDef.m_sName;
    varName.TrimWordStart("TEMPLATE_");
    varName.Append("   ");

    pTable->setItem(row, 0, new QTableWidgetItem(varName.GetData()));

    if (defVal.IsA<bool>())
    {
      QCheckBox* pWidget = new QCheckBox();
      pWidget->setCheckState(defVal.Get<bool>() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
      pTable->setCellWidget(row, 1, pWidget);
    }
    else
    {
      QComboBox* pWidget = new QComboBox();

      ezInt32 iDefItem = -1;

      for (ezUInt32 idx2 = 0; idx2 < enumDef.m_Values.GetCount(); ++idx2)
      {
        const auto& e = enumDef.m_Values[idx2];

        if (e.m_iValueValue == enumDef.m_uiDefaultValue)
          iDefItem = idx2;

        varName = e.m_sValueName.GetString();
        varName.TrimWordStart(enumDef.m_sName);
        varName.TrimWordStart("_");

        pWidget->addItem(varName.GetData());
      }

      if (iDefItem != -1)
      {
        pWidget->setCurrentIndex(iDefItem);
      }

      pTable->setCellWidget(row, 1, pWidget);
    }
  }

  pTable->resizeColumnToContents(0);
}
