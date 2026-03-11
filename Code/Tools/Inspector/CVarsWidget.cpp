#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qspinbox.h>

class ezCommandInterpreterInspector : public ezCommandInterpreter
{
public:
  virtual void Interpret(ezCommandInterpreterState& inout_state) override
  {
    ezTelemetryMessage Msg;
    Msg.SetMessageID('CMD', 'EXEC');
    Msg.GetWriter() << inout_state.m_sInput;
    ezTelemetry::SendToServer(Msg);
  }

  virtual void AutoComplete(ezCommandInterpreterState& inout_state) override
  {
    ezTelemetryMessage Msg;
    Msg.SetMessageID('CMD', 'COMP');
    Msg.GetWriter() << inout_state.m_sInput;
    ezTelemetry::SendToServer(Msg);
  }
};

ezQtCVarsWidget* ezQtCVarsWidget::s_pWidget = nullptr;

ezQtCVarsWidget::ezQtCVarsWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "CVars", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(CVarWidget);

  setIcon(QIcon(":/GuiFoundation/Icons/CVar.svg"));

  connect(CVarWidget, &ezQtCVarWidget::onBoolChanged, this, &ezQtCVarsWidget::BoolChanged);
  connect(CVarWidget, &ezQtCVarWidget::onFloatChanged, this, &ezQtCVarsWidget::FloatChanged);
  connect(CVarWidget, &ezQtCVarWidget::onIntChanged, this, &ezQtCVarsWidget::IntChanged);
  connect(CVarWidget, &ezQtCVarWidget::onStringChanged, this, &ezQtCVarsWidget::StringChanged);

  CVarWidget->GetConsole().SetCommandInterpreter(EZ_DEFAULT_NEW(ezCommandInterpreterInspector));

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

      if (sd.m_bNewEntry)
        bUpdateCVarsTable = true;

      bFillCVarsTable = true;
    }
  }

  if (bUpdateCVarsTable)
    s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);
  else if (bFillCVarsTable)
    s_pWidget->CVarWidget->UpdateCVarUI(s_pWidget->m_CVars);
}

void ezQtCVarsWidget::ProcessTelemetryConsole(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage msg;
  ezStringBuilder tmp;

  while (ezTelemetry::RetrieveMessage('CMD', msg) == EZ_SUCCESS)
  {
    if (msg.GetMessageID() == 'RES')
    {
      msg.GetReader() >> tmp;
      s_pWidget->CVarWidget->AddConsoleStrings(tmp);
    }
  }
}

void ezQtCVarsWidget::SyncAllCVarsToServer()
{
  for (auto it = m_CVars.GetIterator(); it.IsValid(); ++it)
    SendCVarUpdateToServer(it.Key().GetData(), it.Value());
}

void ezQtCVarsWidget::SendCVarUpdateToServer(ezStringView sName, const ezCVarWidgetData& cvd)
{
  ezTelemetryMessage Msg;
  Msg.SetMessageID('SVAR', ' SET');
  Msg.GetWriter() << sName;
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

void ezQtCVarsWidget::BoolChanged(ezStringView sCVar, bool newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_bValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void ezQtCVarsWidget::FloatChanged(ezStringView sCVar, float newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_fValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void ezQtCVarsWidget::IntChanged(ezStringView sCVar, int newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_iValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void ezQtCVarsWidget::StringChanged(ezStringView sCVar, ezStringView sNewValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_sValue = sNewValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}
