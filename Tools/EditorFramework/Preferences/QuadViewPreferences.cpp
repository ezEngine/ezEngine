#include <PCH.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <Foundation/Serialization/GraphPatch.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezEngineViewPreferences, ezNoBase, 2, ezRTTIDefaultAllocator<ezEngineViewPreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CamPos", m_vCamPos),
    EZ_MEMBER_PROPERTY("CamDir", m_vCamDir),
    EZ_MEMBER_PROPERTY("CamUp", m_vCamUp),
    EZ_ENUM_MEMBER_PROPERTY("Perspective", ezSceneViewPerspective, m_uiPerspectiveMode),
    EZ_ENUM_MEMBER_PROPERTY("RenderMode", ezViewRenderMode, m_uiRenderMode),
    EZ_MEMBER_PROPERTY("FOV", m_fFov),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

namespace
{
  /// Patch class
  class ezSceneViewPreferencesPatch_1_2 : public ezGraphPatch
  {
  public:
    ezSceneViewPreferencesPatch_1_2() : ezGraphPatch("ezSceneViewPreferences", 2) {}
    virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
    {
      context.RenameClass("ezEngineViewPreferences");
    }
  };
  ezSceneViewPreferencesPatch_1_2 g_ezSceneViewPreferencesPatch_1_2;
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezQuadViewPreferencesUser, 1, ezRTTIDefaultAllocator<ezQuadViewPreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("QuadView", m_bQuadView)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ViewSingle", m_ViewSingle)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ViewQuad0", m_ViewQuad0)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ViewQuad1", m_ViewQuad1)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ViewQuad2", m_ViewQuad2)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ViewQuad3", m_ViewQuad3)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezQuadViewPreferencesUser::ezQuadViewPreferencesUser() : ezPreferences(Domain::Document, "View")
{
  m_bQuadView = false;

  m_ViewSingle.m_vCamPos.SetZero();
  m_ViewSingle.m_vCamDir.Set(1, 0, 0);
  m_ViewSingle.m_vCamUp.Set(0, 0, 1);
  m_ViewSingle.m_uiPerspectiveMode = ezSceneViewPerspective::Perspective;
  m_ViewSingle.m_uiRenderMode = ezViewRenderMode::Default;
  m_ViewSingle.m_fFov = 70.0f;

  // Top Left: Top Down
  m_ViewQuad0.m_vCamPos.SetZero();
  m_ViewQuad0.m_vCamDir.Set(0, 0, -1);
  m_ViewQuad0.m_vCamUp.Set(1, 0, 0);
  m_ViewQuad0.m_uiPerspectiveMode = ezSceneViewPerspective::Orthogonal_Top;
  m_ViewQuad0.m_uiRenderMode = ezViewRenderMode::WireframeMonochrome;
  m_ViewQuad0.m_fFov = 20.0f;

  // Top Right: Perspective
  m_ViewQuad1.m_vCamPos.SetZero();
  m_ViewQuad1.m_vCamDir.Set(1, 0, 0);
  m_ViewQuad1.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad1.m_uiPerspectiveMode = ezSceneViewPerspective::Perspective;
  m_ViewQuad1.m_uiRenderMode = ezViewRenderMode::Default;
  m_ViewQuad1.m_fFov = 70.0f;

  // Bottom Left: Back to Front
  m_ViewQuad2.m_vCamPos.SetZero();
  m_ViewQuad2.m_vCamDir.Set(1, 0, 0);
  m_ViewQuad2.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad2.m_uiPerspectiveMode = ezSceneViewPerspective::Orthogonal_Front;
  m_ViewQuad2.m_uiRenderMode = ezViewRenderMode::WireframeMonochrome;
  m_ViewQuad2.m_fFov = 20.0f;

  // Bottom Right: Right to Left
  m_ViewQuad3.m_vCamPos.SetZero();
  m_ViewQuad3.m_vCamDir.Set(0, -1, 0);
  m_ViewQuad3.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad3.m_uiPerspectiveMode = ezSceneViewPerspective::Orthogonal_Right;
  m_ViewQuad3.m_uiRenderMode = ezViewRenderMode::WireframeMonochrome;
  m_ViewQuad3.m_fFov = 20.0f;
}
