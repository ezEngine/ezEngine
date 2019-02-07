#include <InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <MainWindow.moc.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qspinbox.h>

ezQtCVarsWidget* ezQtCVarsWidget::s_pWidget = nullptr;

ezQtCVarsWidget::ezQtCVarsWidget(QWidget* parent)
    : QDockWidget(parent)
{
  s_pWidget = this;

  setupUi(this);

  connect(CVarWidget, &ezQtCVarWidget::onBoolChanged, this, &ezQtCVarsWidget::BoolChanged);
  connect(CVarWidget, &ezQtCVarWidget::onFloatChanged, this, &ezQtCVarsWidget::FloatChanged);
  connect(CVarWidget, &ezQtCVarWidget::onIntChanged, this, &ezQtCVarsWidget::IntChanged);
  connect(CVarWidget, &ezQtCVarWidget::onStringChanged, this, &ezQtCVarsWidget::StringChanged);

  ResetStats();
}

void ezQtCVarsWidget::ResetStats()
{
  m_CVarsBackup = m_CVars;
  m_CVars.Clear();
  CVarWidget->Clear();
}

void ezQtCVarsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage msg;

  bool bUpdateCVarsTable = false;
  bool bFillCVarsTable = false;

  while (ezTelemetry::RetrieveMessage('CVAR', msg) == EZ_SUCCESS)
  {
    if (msg.GetMessageID() == ' CLR')
    {
      s_pWidget->m_CVars.Clear();
    }

    if (msg.GetMessageID() == 'SYNC')
    {
      for (auto it = s_pWidget->m_CVars.GetIterator(); it.IsValid(); ++it)
      {
        auto var = s_pWidget->m_CVarsBackup.Find(it.Key());

        if (var.IsValid() && it.Value().m_uiType == var.Value().m_uiType)
        {
          it.Value().m_bValue = var.Value().m_bValue;
          it.Value().m_fValue = var.Value().m_fValue;
          it.Value().m_sValue = var.Value().m_sValue;
          it.Value().m_iValue = var.Value().m_iValue;
        }
      }

      s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);

      s_pWidget->SyncAllCVarsToServer();
    }

    if (msg.GetMessageID() == 'DATA')
    {
      ezString sName;
      msg.GetReader() >> sName;

      ezCVarWidgetData& sd = s_pWidget->m_CVars[sName];

      msg.GetReader() >> sd.m_sPlugin;
      msg.GetReader() >> sd.m_uiType;
      msg.GetReader() >> sd.m_sDescription;

      switch (sd.m_uiType)
      {
        case ezCVarType::Bool:
          msg.GetReader() >> sd.m_bValue;
          break;
        case ezCVarType::Float:
          msg.GetReader() >> sd.m_fValue;
          break;
        case ezCVarType::Int:
          msg.GetReader() >> sd.m_iValue;
          break;
        case ezCVarType::String:
          msg.GetReader() >> sd.m_sValue;
          break;
      }

      if (sd.m_iTableRow == -1)
        bUpdateCVarsTable = true;

      bFillCVarsTable = true;
    }
  }

  if (bUpdateCVarsTable)
    s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);
  else if (bFillCVarsTable)
    s_pWidget->CVarWidget->UpdateCVarUI(s_pWidget->m_CVars);
}

void ezQtCVarsWidget::SyncAllCVarsToServer()
{
  for (auto it = m_CVars.GetIterator(); it.IsValid(); ++it)
    SendCVarUpdateToServer(it.Key().GetData(), it.Value());
}

void ezQtCVarsWidget::SendCVarUpdateToServer(const char* szName, const ezCVarWidgetData& cvd)
{
  ezTelemetryMessage Msg;
  Msg.SetMessageID('SVAR', ' SET');
  Msg.GetWriter() << szName;
  Msg.GetWriter() << cvd.m_uiType;

  switch (cvd.m_uiType)
  {
    case ezCVarType::Bool:
      Msg.GetWriter() << cvd.m_bValue;
      break;

    case ezCVarType::Float:
      Msg.GetWriter() << cvd.m_fValue;
      break;

    case ezCVarType::Int:
      Msg.GetWriter() << cvd.m_iValue;
      break;

    case ezCVarType::String:
      Msg.GetWriter() << cvd.m_sValue;
      break;
  }

  ezTelemetry::SendToServer(Msg);
}

void ezQtCVarsWidget::BoolChanged(const char* szCVar, bool newValue)
{
  auto& cvarData = m_CVars[szCVar];
  cvarData.m_bValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}

void ezQtCVarsWidget::FloatChanged(const char* szCVar, float newValue)
{
  auto& cvarData = m_CVars[szCVar];
  cvarData.m_fValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}

void ezQtCVarsWidget::IntChanged(const char* szCVar, int newValue)
{
  auto& cvarData = m_CVars[szCVar];
  cvarData.m_iValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}

void ezQtCVarsWidget::StringChanged(const char* szCVar, const char* newValue)
{
  auto& cvarData = m_CVars[szCVar];
  cvarData.m_sValue = newValue;
  SendCVarUpdateToServer(szCVar, cvarData);
}
