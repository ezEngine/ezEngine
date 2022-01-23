#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/ui_ShaderTemplateDlg.h>
#include <QDialog>

class ezDocument;

class ezQtShaderTemplateDlg : public QDialog, public Ui_ShaderTemplateDlg
{
  Q_OBJECT

public:
  ezQtShaderTemplateDlg(QWidget* parent, const ezDocument* pDoc);

  ezString m_sResult;

private Q_SLOTS:
  void on_Buttons_accepted();
  void on_Buttons_rejected();
  void on_Browse_clicked();
  void on_ShaderTemplate_currentIndexChanged(int idx);

private:
  struct Template
  {
    ezString m_sName;
    ezString m_sPath;
    ezString m_sContent;
    ezHybridArray<ezString, 16> m_Vars;
  };

  ezHybridArray<Template, 32> m_Templates;
};
