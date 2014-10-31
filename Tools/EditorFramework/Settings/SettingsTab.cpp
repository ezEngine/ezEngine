#include <PCH.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <EditorFramework/EditorFramework.h>

static ezSettingsTab* g_pInstance = nullptr;

ezSettingsTab* ezSettingsTab::GetInstance()
{
  return g_pInstance;
}

ezSettingsTab::ezSettingsTab() : ezDocumentWindow("Settings")
{
  setCentralWidget(new QWidget());

  EZ_ASSERT(g_pInstance == nullptr, "");
  EZ_ASSERT(centralWidget() != nullptr, "");

  g_pInstance = this;
  setupUi(centralWidget());

  m_pSettingsGrid = new ezSimplePropertyGridWidget(this);
  GroupSettings->layout()->addWidget(m_pSettingsGrid);

  m_pSettingsGrid->BeginProperties();

  ezSettings& s = ezEditorFramework::GetEditorSettings();

  for (auto it = s.GetAllSettings().GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_Flags.IsAnySet(ezSettingsFlags::Hidden))
      continue;

    m_pSettingsGrid->AddProperty(it.Key(), it.Value().m_Value, &it.Value().m_Value);
  }

  m_pSettingsGrid->AddProperty("Bla", true);
  m_pSettingsGrid->AddProperty("Blub", false);
  m_pSettingsGrid->AddProperty("float", 23.5f);
  m_pSettingsGrid->AddProperty("double", 43.5);
  m_pSettingsGrid->AddProperty("int", 17);
  m_pSettingsGrid->AddProperty("string", "test");

  m_pSettingsGrid->EndProperties();

  EZ_VERIFY(connect(m_pSettingsGrid, SIGNAL(value_changed()), this, SLOT(SlotSettingsChanged())) != nullptr, "signal/slot connection failed");
}

ezSettingsTab::~ezSettingsTab()
{
  g_pInstance = nullptr;
}

bool ezSettingsTab::InternalCanClose()
{
  return true;
}

void ezSettingsTab::InternalCloseDocument()
{
}

void ezSettingsTab::SlotSettingsChanged()
{
  const auto& props = m_pSettingsGrid->GetAllProperties();
  ezSettings& s = ezEditorFramework::GetEditorSettings();

  //for (auto it = s.GetAllSettings().GetIterator(); it.IsValid(); ++it)
  //{
  //  if (it.Value().m_Flags.IsAnySet(ezSettingsFlags::Hidden))
  //    continue;

  //  props.IndexOf(

  //  m_pSettingsGrid->AddProperty(it.Key(), it.Value().m_Value);
  //}

}

