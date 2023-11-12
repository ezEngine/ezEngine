#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezCameraVisualizerAdapter::ezCameraVisualizerAdapter() = default;

ezCameraVisualizerAdapter::~ezCameraVisualizerAdapter() = default;

void ezCameraVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  m_hBoxGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::LineBox, ezColor::DodgerBlue, ezGizmoFlags::Visualizer | ezGizmoFlags::ShowInOrtho);
  m_hFrustumGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::Frustum, ezColor::DodgerBlue, ezGizmoFlags::Visualizer | ezGizmoFlags::ShowInOrtho);
  m_hNearPlaneGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::LineRect, ezColor::LightBlue, ezGizmoFlags::Visualizer | ezGizmoFlags::ShowInOrtho);
  m_hFarPlaneGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::LineRect, ezColor::PaleVioletRed, ezGizmoFlags::Visualizer | ezGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hBoxGizmo);
  pAssetDocument->AddSyncObject(&m_hFrustumGizmo);
  pAssetDocument->AddSyncObject(&m_hNearPlaneGizmo);
  pAssetDocument->AddSyncObject(&m_hFarPlaneGizmo);

  m_hBoxGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hFrustumGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hNearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hFarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezCameraVisualizerAdapter::Update()
{

  const ezCameraVisualizerAttribute* pAttr = static_cast<const ezCameraVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  float fNearPlane = 1.0f;
  float fFarPlane = 10.0f;
  ezInt32 iMode = 0;

  if (!pAttr->GetModeProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetModeProperty()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezInt32>(), "Invalid property bound to ezCameraVisualizerAttribute 'mode'");
    iMode = value.ConvertTo<ezInt32>();
  }

  if (!pAttr->GetNearPlaneProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetNearPlaneProperty()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCameraVisualizerAttribute 'near plane'");
    fNearPlane = value.ConvertTo<float>();
  }

  if (!pAttr->GetFarPlaneProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFarPlaneProperty()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCameraVisualizerAttribute 'far plane'");
    fFarPlane = value.ConvertTo<float>();
  }

  if (iMode == ezCameraMode::OrthoFixedHeight || iMode == ezCameraMode::OrthoFixedWidth)
  {
    float fDimensions = 1.0f;

    if (!pAttr->GetOrthoDimProperty().IsEmpty())
    {
      ezVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOrthoDimProperty()), value).AssertSuccess();

      EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCameraVisualizerAttribute 'ortho dim'");
      fDimensions = value.ConvertTo<float>();
    }

    {
      const float fRange = fFarPlane - fNearPlane;

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fRange, fDimensions, fDimensions);
      m_LocalTransformFrustum.m_vPosition.Set(fNearPlane + fRange * 0.5f, 0, 0);
    }

    m_hBoxGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hFrustumGizmo.SetVisible(false);
    m_hNearPlaneGizmo.SetVisible(false);
    m_hFarPlaneGizmo.SetVisible(false);

    m_LocalTransformNearPlane.SetIdentity();
    m_LocalTransformFarPlane.SetIdentity();
  }
  else
  {
    float fFOV = 45.0f;

    if (!pAttr->GetFovProperty().IsEmpty())
    {
      ezVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFovProperty()), value).AssertSuccess();

      EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCameraVisualizerAttribute 'fov'");
      fFOV = value.ConvertTo<float>();
    }

    {
      const float fAngleScale = ezMath::Tan(ezAngle::MakeFromDegree(fFOV) * 0.5f);
      const float fFrustumScale = ezMath::Min(fFarPlane, 10.0f);
      const float fFarPlaneScale = ezMath::Min(fFarPlane, 9.0f);
      ;

      // indicate whether the shown far plane is the actual distance, or just the maximum visualization distance
      m_hFarPlaneGizmo.SetColor(fFarPlane > 9.0f ? ezColor::DodgerBlue : ezColor::PaleVioletRed);

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fFrustumScale, fAngleScale * fFrustumScale, fAngleScale * fFrustumScale);
      m_LocalTransformFrustum.m_vPosition.Set(0, 0, 0);

      m_LocalTransformNearPlane.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(90));
      m_LocalTransformNearPlane.m_vScale.Set(fAngleScale * fNearPlane, fAngleScale * fNearPlane, 1);
      m_LocalTransformNearPlane.m_vPosition.Set(fNearPlane, 0, 0);

      m_LocalTransformFarPlane.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(90));
      m_LocalTransformFarPlane.m_vScale.Set(fAngleScale * fFarPlaneScale, fAngleScale * fFarPlaneScale, 1);
      m_LocalTransformFarPlane.m_vPosition.Set(fFarPlaneScale, 0, 0);
    }

    m_hBoxGizmo.SetVisible(false);
    m_hFrustumGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hNearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hFarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  }
}

void ezCameraVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t = GetObjectTransform();
  m_hBoxGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_hFrustumGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_hNearPlaneGizmo.SetTransformation(t * m_LocalTransformNearPlane);
  m_hFarPlaneGizmo.SetTransformation(t * m_LocalTransformFarPlane);
}
