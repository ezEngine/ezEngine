#include <EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GameEngine/Animation/RotorComponent.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <GameEngine/Animation/SliderComponent.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <GameEngine/Gameplay/SpawnComponent.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipContext, 1, ezRTTIDefaultAllocator<ezAnimationClipContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Animation Clip"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimationClipContext::ezAnimationClipContext() {}

void ezAnimationClipContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = ezDynamicCast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = ezDynamicCast<const ezSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "PreviewMesh" && m_sAnimatedMeshToUse != pMsg->m_sPayload)
    {
      m_sAnimatedMeshToUse = pMsg->m_sPayload;

      auto pWorld = m_pWorld.Borrow();
      EZ_LOCK(pWorld->GetWriteMarker());

      ezStringBuilder sAnimClipGuid;
      ezConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);

      ezAnimatedMeshComponent* pAnimMesh;
      ezSimpleAnimationComponent* pAnimController;

      if (pWorld->TryGetComponent(m_hAnimMeshComponent, pAnimMesh))
        pAnimMesh->DeleteComponent();

      if (pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
        pAnimController->DeleteComponent();

      m_hAnimMeshComponent = ezAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
      m_hAnimControllerComponent = ezSimpleAnimationComponent::CreateComponent(m_pGameObject, pAnimController);

      pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);
      pAnimController->SetAnimationClipFile(sAnimClipGuid);
    }
    else if (pMsg->m_sWhatToDo == "SimulationSpeed")
    {
      SetSpeed((float)pMsg->m_fPayload);
    }

    return;
  }

  if (auto pMsg = ezDynamicCast<const ezSimulationSettingsMsgToEngine*>(pMsg0))
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages

    m_pWorld->SetWorldSimulationEnabled(pMsg->m_bSimulateWorld);
    m_pWorld->GetClock().SetSpeed(pMsg->m_fSimulationSpeed);
    return;
  }

  if (auto pMsg = ezDynamicCast<const ezEditorEngineRestartSimulationMsg*>(pMsg0))
  {
    Restart();
  }

  if (auto pMsg = ezDynamicCast<const ezEditorEngineLoopAnimationMsg*>(pMsg0))
  {
    SetLoop(pMsg->m_bLoop);
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void ezAnimationClipContext::OnInitialize()
{
  auto pWorld = m_pWorld.Borrow();
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;

  // Preview
  {
    obj.m_bDynamic = true;
    obj.m_sName.Assign("SkeletonPreview");
    pWorld->CreateObject(obj, m_pGameObject);
  }

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(120.0f));

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);
    pDirLight->SetCastShadows(true);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
    pAmbLight->SetIntensity(5.0f);
  }
}

ezEngineProcessViewContext* ezAnimationClipContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezAnimationClipViewContext, this);
}

void ezAnimationClipContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezAnimationClipContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezBoundingBoxSphere bounds = GetWorldBounds(m_pWorld.Borrow());

  ezAnimationClipViewContext* pMeshViewContext = static_cast<ezAnimationClipViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezAnimationClipContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pGameObject->UpdateLocalBounds();
    m_pGameObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pGameObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const ezQuerySelectionBBoxMsgToEngine* msg = static_cast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg);

  ezQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void ezAnimationClipContext::Restart()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezSimpleAnimationComponent* pAnimController;
  if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
  {
    pAnimController->SetNormalizedPlaybackPosition(0.0f);
  }
}

void ezAnimationClipContext::SetLoop(bool loop)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezSimpleAnimationComponent* pAnimController;
  if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
  {
    pAnimController->m_AnimationMode = loop ? ezPropertyAnimMode::Loop : ezPropertyAnimMode::Once;
  }
}

void ezAnimationClipContext::SetSpeed(float speed)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezSimpleAnimationComponent* pAnimController;
  if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
  {
    pAnimController->m_fSpeed = speed;
  }
}
