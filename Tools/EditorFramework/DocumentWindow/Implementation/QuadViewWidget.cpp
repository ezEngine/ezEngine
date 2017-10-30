#include <PCH.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <QGridLayout>

ezQtQuadViewWidget::ezQtQuadViewWidget(ezAssetDocument* pDocument, ezQtEngineDocumentWindow* pWindow,
  ViewFactory viewFactory, const char* szViewToolBarMapping)
{
  m_pDocument = pDocument;
  m_pWindow = pWindow;
  m_ViewFactory = viewFactory;
  m_sViewToolBarMapping = szViewToolBarMapping;

  m_pViewLayout = new QGridLayout(this);
  m_pViewLayout->setMargin(0);
  m_pViewLayout->setSpacing(4);
  setLayout(m_pViewLayout);

  LoadViewConfigs();
}

ezQtQuadViewWidget::~ezQtQuadViewWidget()
{
  SaveViewConfigs();
}

void ezQtQuadViewWidget::SaveViewConfig(const ezEngineViewConfig& cfg, ezEngineViewPreferences& pref) const
{
  pref.m_vCamPos = cfg.m_Camera.GetPosition();
  pref.m_vCamDir = cfg.m_Camera.GetDirForwards();
  pref.m_vCamUp = cfg.m_Camera.GetDirUp();
  pref.m_uiPerspectiveMode = cfg.m_Perspective;
  pref.m_uiRenderMode = cfg.m_RenderMode;
  pref.m_fFov = cfg.m_Camera.GetFovOrDim();
}

void ezQtQuadViewWidget::LoadViewConfig(ezEngineViewConfig& cfg, ezEngineViewPreferences& pref)
{
  cfg.m_Perspective = (ezSceneViewPerspective::Enum)pref.m_uiPerspectiveMode;
  cfg.m_RenderMode = (ezViewRenderMode::Enum)pref.m_uiRenderMode;
  cfg.m_Camera.LookAt(ezVec3(0), ezVec3(1, 0, 0), ezVec3(0, 0, 1));

  if (cfg.m_Perspective == ezSceneViewPerspective::Perspective)
  {
    ezEditorPreferencesUser* pPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
    cfg.ApplyPerspectiveSetting(pPref->m_fPerspectiveFieldOfView);
  }
  else
  {
    cfg.ApplyPerspectiveSetting(pref.m_fFov);
  }

  pref.m_vCamDir.NormalizeIfNotZero(ezVec3(1, 0, 0));
  pref.m_vCamUp.MakeOrthogonalTo(pref.m_vCamDir);
  pref.m_vCamUp.NormalizeIfNotZero(pref.m_vCamDir.GetOrthogonalVector().GetNormalized());

  cfg.m_Camera.LookAt(pref.m_vCamPos, pref.m_vCamPos + pref.m_vCamDir, pref.m_vCamUp);

}

void ezQtQuadViewWidget::SaveViewConfigs() const
{
  ezQuadViewPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezQuadViewPreferencesUser>(m_pDocument);
  pPreferences->m_bQuadView = m_pWindow->GetViewWidgets().GetCount() == 4;

  SaveViewConfig(m_ViewConfigSingle, pPreferences->m_ViewSingle);
  SaveViewConfig(m_ViewConfigQuad[0], pPreferences->m_ViewQuad0);
  SaveViewConfig(m_ViewConfigQuad[1], pPreferences->m_ViewQuad1);
  SaveViewConfig(m_ViewConfigQuad[2], pPreferences->m_ViewQuad2);
  SaveViewConfig(m_ViewConfigQuad[3], pPreferences->m_ViewQuad3);
}

void ezQtQuadViewWidget::LoadViewConfigs()
{
  ezQuadViewPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezQuadViewPreferencesUser>(m_pDocument);

  LoadViewConfig(m_ViewConfigSingle, pPreferences->m_ViewSingle);
  LoadViewConfig(m_ViewConfigQuad[0], pPreferences->m_ViewQuad0);
  LoadViewConfig(m_ViewConfigQuad[1], pPreferences->m_ViewQuad1);
  LoadViewConfig(m_ViewConfigQuad[2], pPreferences->m_ViewQuad2);
  LoadViewConfig(m_ViewConfigQuad[3], pPreferences->m_ViewQuad3);

  CreateViews(pPreferences->m_bQuadView);
}

void ezQtQuadViewWidget::CreateViews(bool bQuad)
{
  ezQtScopedUpdatesDisabled _(this);
  for (auto pContainer : m_ActiveMainViews)
  {
    delete pContainer;
  }
  m_ActiveMainViews.Clear();

  if (bQuad)
  {
    for (ezUInt32 i = 0; i < 4; ++i)
    {
      ezQtEngineViewWidget* pViewWidget = m_ViewFactory(m_pWindow, &m_ViewConfigQuad[i]);
      ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(m_pWindow, pViewWidget, "EditorPluginScene_ViewToolBar");
      m_ActiveMainViews.PushBack(pContainer);
      m_pViewLayout->addWidget(pContainer, i / 2, i % 2);

    }
  }
  else
  {
    ezQtEngineViewWidget* pViewWidget = m_ViewFactory(m_pWindow, &m_ViewConfigSingle);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(m_pWindow, pViewWidget, "EditorPluginScene_ViewToolBar");
    m_ActiveMainViews.PushBack(pContainer);
    m_pViewLayout->addWidget(pContainer, 0, 0);
  }
}

void ezQtQuadViewWidget::ToggleViews(QWidget* pView)
{
  ezQtEngineViewWidget* pViewport = qobject_cast<ezQtEngineViewWidget*>(pView);
  EZ_ASSERT_DEV(pViewport != nullptr, "ezQtSceneDocumentWindow::ToggleViews must be called with a ezQtSceneViewWidget as parameter!");
  bool bIsQuad = m_ActiveMainViews.GetCount() == 4;
  if (bIsQuad)
  {
    m_ViewConfigSingle = *pViewport->m_pViewConfig;
    m_ViewConfigSingle.m_pLinkedViewConfig = pViewport->m_pViewConfig;
    CreateViews(false);
  }
  else
  {
    if (pViewport->m_pViewConfig->m_pLinkedViewConfig != nullptr)
      *pViewport->m_pViewConfig->m_pLinkedViewConfig = *pViewport->m_pViewConfig;

    CreateViews(true);
  }
}


