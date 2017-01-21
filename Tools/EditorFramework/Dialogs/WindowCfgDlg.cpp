#include <PCH.h>
#include <EditorFramework/Dialogs/WindowCfgDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezQtWindowCfgDlg::ezQtWindowCfgDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);
  LoadDescs();

  m_ComboMonitor->addItem("Primary");
  m_ComboMonitor->addItem("Monitor 0");
  m_ComboMonitor->addItem("Monitor 1");
  m_ComboMonitor->addItem("Monitor 2");
  m_ComboMonitor->addItem("Monitor 3");
  m_ComboMonitor->addItem("Monitor 4");
  m_ComboMonitor->addItem("Monitor 5");

  m_ComboMode->addItem("Window (Fixed Resolution)");
  m_ComboMode->addItem("Window (Resizable)");
  m_ComboMode->addItem("Fullscreen (Borderless Wnd)");
  m_ComboMode->addItem("Fullscreen (Fixed Resolution)");

  FillUI(m_Descs[m_uiCurDesc]);

  m_ComboWnd->addItem("Project Default");
  m_ComboWnd->addItem("User Specific (ezPlayer)");
  m_ComboWnd->setCurrentIndex(0);
}

void ezQtWindowCfgDlg::FillUI(const ezWindowCreationDesc& desc)
{
  m_LineEditTitle->setText(QString::fromUtf8(desc.m_Title.GetData()));
  m_ComboMonitor->setCurrentIndex(ezMath::Clamp<ezInt32>(desc.m_iMonitor, -1, 5) + 1);
  m_ComboMode->setCurrentIndex(ezMath::Clamp<ezInt32>(desc.m_WindowMode, 0, 3));

  m_SpinResX->setValue(desc.m_Resolution.width);
  m_SpinResY->setValue(desc.m_Resolution.height);
}

void ezQtWindowCfgDlg::GrabUI(ezWindowCreationDesc& desc)
{
  desc.m_Title = m_LineEditTitle->text().toUtf8().data();
  desc.m_iMonitor = m_ComboMonitor->currentIndex() - 1;
  desc.m_WindowMode = (ezWindowMode::Enum)(m_ComboMode->currentIndex());
  desc.m_Resolution.width = m_SpinResX->value();
  desc.m_Resolution.height = m_SpinResY->value();
}

void ezQtWindowCfgDlg::LoadDescs()
{
  ezStringBuilder sPath;

  {
    sPath = ezApplicationConfig::GetProjectDirectory();
    sPath.AppendPath("Window.ddl");

    m_Descs[0].LoadFromDDL(sPath);
  }

  {
    sPath = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sPath.AppendPath("Window.ddl");

    m_Descs[1].LoadFromDDL(sPath);
  }
}

void ezQtWindowCfgDlg::SaveDescs()
{
  ezStringBuilder sPath;

  {
    sPath = ezApplicationConfig::GetProjectDirectory();
    sPath.AppendPath("Window.ddl");

    m_Descs[0].SaveToDDL(sPath);
  }

  {
    sPath = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sPath.AppendPath("Window.ddl");

    m_Descs[1].SaveToDDL(sPath);
  }
}

void ezQtWindowCfgDlg::on_m_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == m_ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    GrabUI(m_Descs[m_uiCurDesc]);

    SaveDescs();
    accept();
    return;
  }

  if (button == m_ButtonBox->button(QDialogButtonBox::StandardButton::Cancel))
  {
    reject();
    return;
  }
}

void ezQtWindowCfgDlg::on_m_ComboWnd_currentIndexChanged(int index)
{
  GrabUI(m_Descs[m_uiCurDesc]);
  m_uiCurDesc = index;
  FillUI(m_Descs[m_uiCurDesc]);
}

