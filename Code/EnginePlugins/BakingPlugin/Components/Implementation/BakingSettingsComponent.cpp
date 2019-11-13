#include <BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Components/BakingSettingsComponent.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Device/Device.h>

struct ezBakingSettingsComponent::RenderDebugViewTask : public ezTask
{
  virtual void Execute() override
  {
    if (HasBeenCanceled())
      return;
  }

  ezMat4 m_ViewProjectionMatrix = ezMat4::IdentityMatrix();
};

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezBakingSettingsComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ShowDebugOverlay", GetShowDebugOverlay, SetShowDebugOverlay),
    EZ_ACCESSOR_PROPERTY("ShowDebugProbes", GetShowDebugProbes, SetShowDebugProbes),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Baking"),
    new ezLongOpAttribute("ezLongOpProxy_BakeScene")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezBakingSettingsComponent::ezBakingSettingsComponent() = default;
ezBakingSettingsComponent::~ezBakingSettingsComponent() = default;

void ezBakingSettingsComponent::OnActivated()
{
}

void ezBakingSettingsComponent::OnDeactivated()
{
  ezTaskSystem::WaitForTask(m_pRenderDebugViewTask.Borrow());

  GetOwner()->UpdateLocalBounds();
}

void ezBakingSettingsComponent::SetShowDebugOverlay(bool bShow)
{
  if (m_bShowDebugOverlay != bShow)
  {
    m_bShowDebugOverlay = bShow;
    GetOwner()->UpdateLocalBounds();

    if (bShow && m_pRenderDebugViewTask == nullptr)
    {
      m_pRenderDebugViewTask = EZ_DEFAULT_NEW(RenderDebugViewTask);
    }
  }
}

void ezBakingSettingsComponent::SetShowDebugProbes(bool bShow)
{
  if (m_bShowDebugProbes != bShow)
  {
    m_bShowDebugProbes = bShow;
    GetOwner()->UpdateLocalBounds();
  }
}

void ezBakingSettingsComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  if (m_bShowDebugOverlay || m_bShowDebugProbes)
  {
    msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
  }
}

void ezBakingSettingsComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (m_bShowDebugOverlay)
  {
    if (ezBakingScene* pBakingScene = ezBakingScene::Get(*GetWorld()))
    {
      if (m_pRenderDebugViewTask->m_ViewProjectionMatrix != msg.m_pView->GetViewProjectionMatrix(ezCameraEye::Left))
      {
        ezTaskSystem::CancelTask(m_pRenderDebugViewTask.Borrow());

        m_pRenderDebugViewTask->m_ViewProjectionMatrix = msg.m_pView->GetViewProjectionMatrix(ezCameraEye::Left);

        ezTaskSystem::StartSingleTask(m_pRenderDebugViewTask.Borrow(), ezTaskPriority::LongRunning);
      }
    }

    ezRectFloat viewport = msg.m_pView->GetViewport();
    ezRectFloat rectInPixel = ezRectFloat(10.0f, 10.0f, ezMath::Ceil(viewport.width / 3.0f), ezMath::Ceil(viewport.height / 3.0f));

    ezDebugRenderer::Draw2DRectangle(msg.m_pView->GetHandle(), rectInPixel, 0.0f, ezColor::White,
      ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hDebugViewTexture));
  }
}

void ezBakingSettingsComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();
}

void ezBakingSettingsComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();
}
