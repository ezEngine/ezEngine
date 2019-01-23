#include <PCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezCameraVisualizerAdapter::ezCameraVisualizerAdapter() {}

ezCameraVisualizerAdapter::~ezCameraVisualizerAdapter() {}

void ezCameraVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezCameraVisualizerAttribute* pAttr = static_cast<const ezCameraVisualizerAttribute*>(m_pVisualizerAttr);

  m_BoxGizmo.Configure(nullptr, ezEngineGizmoHandleType::LineBox, ezColor::DodgerBlue, false, false, true);
  m_FrustumGizmo.Configure(nullptr, ezEngineGizmoHandleType::Frustum, ezColor::DodgerBlue, false, false, true);
  m_NearPlaneGizmo.Configure(nullptr, ezEngineGizmoHandleType::LineRect, ezColor::LightBlue, false, false, true);
  m_FarPlaneGizmo.Configure(nullptr, ezEngineGizmoHandleType::LineRect, ezColor::PaleVioletRed, false, false, true);

  pAssetDocument->AddSyncObject(&m_BoxGizmo);
  pAssetDocument->AddSyncObject(&m_FrustumGizmo);
  pAssetDocument->AddSyncObject(&m_NearPlaneGizmo);
  pAssetDocument->AddSyncObject(&m_FarPlaneGizmo);

  m_BoxGizmo.SetVisible(m_bVisualizerIsVisible);
  m_FrustumGizmo.SetVisible(m_bVisualizerIsVisible);
  m_NearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  m_FarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
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
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetModeProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezInt32>(), "Invalid property bound to ezCameraVisualizerAttribute 'mode'");
    iMode = value.ConvertTo<ezInt32>();
  }

  if (!pAttr->GetNearPlaneProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetNearPlaneProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCameraVisualizerAttribute 'near plane'");
    fNearPlane = value.ConvertTo<float>();
  }

  if (!pAttr->GetFarPlaneProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFarPlaneProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCameraVisualizerAttribute 'far plane'");
    fFarPlane = value.ConvertTo<float>();
  }

  if (iMode == ezCameraMode::OrthoFixedHeight || iMode == ezCameraMode::OrthoFixedWidth)
  {
    float fDimensions = 1.0f;

    if (!pAttr->GetOrthoDimProperty().IsEmpty())
    {
      ezVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOrthoDimProperty()), value);

      EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCameraVisualizerAttribute 'ortho dim'");
      fDimensions = value.ConvertTo<float>();
    }

    {
      const float fRange = fFarPlane - fNearPlane;

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fRange, fDimensions, fDimensions);
      m_LocalTransformFrustum.m_vPosition.Set(fNearPlane + fRange * 0.5f, 0, 0);
    }

    m_BoxGizmo.SetVisible(m_bVisualizerIsVisible);
    m_FrustumGizmo.SetVisible(false);
    m_NearPlaneGizmo.SetVisible(false);
    m_FarPlaneGizmo.SetVisible(false);

    m_LocalTransformNearPlane.SetIdentity();
    m_LocalTransformFarPlane.SetIdentity();
  }
  else
  {
    float fFOV = 45.0f;

    if (!pAttr->GetFovProperty().IsEmpty())
    {
      ezVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFovProperty()), value);

      EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCameraVisualizerAttribute 'fov'");
      fFOV = value.ConvertTo<float>();
    }

    {
      const float fAngleScale = ezMath::Tan(ezAngle::Degree(fFOV) * 0.5f);
      const float fFrustumScale = ezMath::Min(fFarPlane, 10.0f);
      const float fFarPlaneScale = ezMath::Min(fFarPlane, 9.0f);
      ;

      // indicate whether the shown far plane is the actual distance, or just the maximum visualization distance
      m_FarPlaneGizmo.SetColor(fFarPlane > 9.0f ? ezColor::DodgerBlue : ezColor::PaleVioletRed);

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fFrustumScale, fAngleScale * fFrustumScale, fAngleScale * fFrustumScale);
      m_LocalTransformFrustum.m_vPosition.Set(0, 0, 0);

      m_LocalTransformNearPlane.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      m_LocalTransformNearPlane.m_vScale.Set(fAngleScale * fNearPlane, fAngleScale * fNearPlane, 1);
      m_LocalTransformNearPlane.m_vPosition.Set(fNearPlane, 0, 0);

      m_LocalTransformFarPlane.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      m_LocalTransformFarPlane.m_vScale.Set(fAngleScale * fFarPlaneScale, fAngleScale * fFarPlaneScale, 1);
      m_LocalTransformFarPlane.m_vPosition.Set(fFarPlaneScale, 0, 0);
    }

    m_BoxGizmo.SetVisible(false);
    m_FrustumGizmo.SetVisible(m_bVisualizerIsVisible);
    m_NearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
    m_FarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  }
}

void ezCameraVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t = GetObjectTransform();
  m_BoxGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_FrustumGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_NearPlaneGizmo.SetTransformation(t * m_LocalTransformNearPlane);
  m_FarPlaneGizmo.SetTransformation(t * m_LocalTransformFarPlane);
}
