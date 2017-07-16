#include <PCH.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <Foundation/Configuration/CVar.h>
#include <QTimer>

EZ_IMPLEMENT_SINGLETON(ezQtCVarPanel);

ezQtCVarPanel::ezQtCVarPanel()
  : ezQtApplicationPanel("Panel.CVar")
  , m_SingletonRegistrar(this)
{
  setWindowIcon(ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/CVar.png"));
  setWindowTitle(QString::fromUtf8(ezTranslate("Panel.CVar")));

  m_pCVarWidget = new ezQtCVarWidget(this);
  setWidget(m_pCVarWidget);

  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezQtCVarPanel::EngineProcessMsgHandler, this));

  connect(m_pCVarWidget, &ezQtCVarWidget::onBoolChanged, this, &ezQtCVarPanel::BoolChanged);
  connect(m_pCVarWidget, &ezQtCVarWidget::onFloatChanged, this, &ezQtCVarPanel::FloatChanged);
  connect(m_pCVarWidget, &ezQtCVarWidget::onIntChanged, this, &ezQtCVarPanel::IntChanged);
  connect(m_pCVarWidget, &ezQtCVarWidget::onStringChanged, this, &ezQtCVarPanel::StringChanged);
}

ezQtCVarPanel::~ezQtCVarPanel()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtCVarPanel::EngineProcessMsgHandler, this));
}

void ezQtCVarPanel::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
  case ezToolsProjectEvent::Type::ProjectClosing:
    {
      m_EngineCVarState.Clear();
      m_pCVarWidget->Clear();

      // fallthrough
  case ezToolsProjectEvent::Type::ProjectOpened:
      setEnabled(e.m_Type == ezToolsProjectEvent::Type::ProjectOpened);
    }
    break;
  }

  ezQtApplicationPanel::ToolsProjectEventHandler(e);
}

void ezQtCVarPanel::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezCVarMsgToEditor>())
      {
        const ezCVarMsgToEditor* pMsg = static_cast<const ezCVarMsgToEditor*>(e.m_pMsg);
        
        bool bExisted = false;
        auto& cvar = m_EngineCVarState.FindOrAdd(pMsg->m_sName, &bExisted).Value();
        cvar.m_sDescription = pMsg->m_sDescription;
        cvar.m_sPlugin = pMsg->m_sPlugin;

        switch (pMsg->m_Value.GetType())
        {
        case ezVariantType::Float:
          cvar.m_uiType = ezCVarType::Float;
          cvar.m_fValue = pMsg->m_Value.ConvertTo<float>();
          break;
        case ezVariantType::Int32:
          cvar.m_uiType = ezCVarType::Int;
          cvar.m_iValue = pMsg->m_Value.ConvertTo<int>();
          break;
        case ezVariantType::Bool:
          cvar.m_uiType = ezCVarType::Bool;
          cvar.m_bValue = pMsg->m_Value.ConvertTo<bool>();
          break;
        case ezVariantType::String:
          cvar.m_uiType = ezCVarType::String;
          cvar.m_sValue = pMsg->m_Value.ConvertTo<ezString>();
          break;
        }

        if (!bExisted)
          m_bRebuildUI = true;

        if (!m_bUpdateUI)
        {
          m_bUpdateUI = true;

          // don't do this every single time, otherwise we would spam this during project load
          QTimer::singleShot(100, this, SLOT(UpdateUI()));
        }
      }
    }
    break;
  }
}

void ezQtCVarPanel::UpdateUI()
{
  m_bUpdateUI = false;

  if (m_bRebuildUI)
  {
    m_pCVarWidget->RebuildCVarUI(m_EngineCVarState);
  }
  else
  {
    m_pCVarWidget->UpdateCVarUI(m_EngineCVarState);
  }
}

void ezQtCVarPanel::BoolChanged(const char* szCVar, bool newValue)
{
  ezChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtCVarPanel::FloatChanged(const char* szCVar, float newValue)
{
  ezChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtCVarPanel::IntChanged(const char* szCVar, int newValue)
{
  ezChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtCVarPanel::StringChanged(const char* szCVar, const char* newValue)
{
  ezChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

