#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/RoundedRectVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezRoundedRectVisualizerAdapter::ezRoundedRectVisualizerAdapter() = default;

ezRoundedRectVisualizerAdapter::~ezRoundedRectVisualizerAdapter() = default;

void ezRoundedRectVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezRoundedRectVisualizerAttribute* pAttr = static_cast<const ezRoundedRectVisualizerAttribute*>(m_pVisualizerAttr);

  m_hLinesInner.ConfigureHandle(nullptr, ezEngineGizmoHandleType::CustomLines, pAttr->m_InnerColor, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);
  m_hLinesOuter.ConfigureHandle(nullptr, ezEngineGizmoHandleType::CustomLines, pAttr->m_OuterColor, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hLinesInner);
  pAssetDocument->AddSyncObject(&m_hLinesOuter);
}


static void BuildRoundedRectLines(ezDynamicArray<ezVec3>& ref_lines, float fHalfX, float fHalfY, float fRadius)
{
  ref_lines.Clear();

  // Straight edge along X (top and bottom), skipped when halfX == 0.
  if (fHalfX > 0.0f)
  {
    ref_lines.PushBack(ezVec3(-fHalfX, -(fHalfY + fRadius), 0.0f));
    ref_lines.PushBack(ezVec3(+fHalfX, -(fHalfY + fRadius), 0.0f));

    ref_lines.PushBack(ezVec3(+fHalfX, +(fHalfY + fRadius), 0.0f));
    ref_lines.PushBack(ezVec3(-fHalfX, +(fHalfY + fRadius), 0.0f));
  }
  // Straight edge along Y (left and right), skipped when halfY == 0.
  if (fHalfY > 0.0f)
  {
    ref_lines.PushBack(ezVec3(+(fHalfX + fRadius), -fHalfY, 0.0f));
    ref_lines.PushBack(ezVec3(+(fHalfX + fRadius), +fHalfY, 0.0f));

    ref_lines.PushBack(ezVec3(-(fHalfX + fRadius), +fHalfY, 0.0f));
    ref_lines.PushBack(ezVec3(-(fHalfX + fRadius), -fHalfY, 0.0f));
  }

  if (fRadius <= 0.0f)
    return;

  // 4 quarter-circle arcs at the corners.
  constexpr ezUInt32 uiSegmentsPerQuarter = 8;
  constexpr float fStep = ezMath::Pi<float>() * 0.5f / uiSegmentsPerQuarter;

  const ezVec2 vCorners[4] = {
    ezVec2(+fHalfX, -fHalfY),    // bottom-right
    ezVec2(+fHalfX, +fHalfY),    // top-right
    ezVec2(-fHalfX, +fHalfY),    // top-left
    ezVec2(-fHalfX, -fHalfY),    // bottom-left
  };
  const float fStartAngles[4] = {
    -ezMath::Pi<float>() * 0.5f, // bottom-right: -90° → 0°
    0.0f,                        // top-right:    0° → 90°
    ezMath::Pi<float>() * 0.5f,  // top-left:     90° → 180°
    ezMath::Pi<float>(),         // bottom-left:  180° → 270°
  };

  for (ezUInt32 c = 0; c < 4; ++c)
  {
    for (ezUInt32 s = 0; s < uiSegmentsPerQuarter; ++s)
    {
      const ezAngle fA0 = ezAngle::MakeFromRadian(fStartAngles[c] + s * fStep);
      const ezAngle fA1 = ezAngle::MakeFromRadian(fStartAngles[c] + (s + 1) * fStep);

      ref_lines.PushBack(ezVec3(vCorners[c].x + fRadius * ezMath::Cos(fA0), vCorners[c].y + fRadius * ezMath::Sin(fA0), 0.0f));
      ref_lines.PushBack(ezVec3(vCorners[c].x + fRadius * ezMath::Cos(fA1), vCorners[c].y + fRadius * ezMath::Sin(fA1), 0.0f));
    }
  }
}

void ezRoundedRectVisualizerAdapter::Update()
{
  const ezRoundedRectVisualizerAttribute* pAttr = static_cast<const ezRoundedRectVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  float fSizeX = 0;
  float fSizeY = 0;
  float fInnerRadius = 0;
  float fOuterRadius = 0;

  if (!pAttr->GetPropHalfSizeX().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeX()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezRoundedRectVisualizerAttribute 'size x'");
    fSizeX = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropHalfSizeY().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeY()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezRoundedRectVisualizerAttribute 'size y'");
    fSizeY = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropInnerRadius().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropInnerRadius()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezRoundedRectVisualizerAttribute 'inner'");
    fInnerRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropOuterRadius().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropOuterRadius()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezRoundedRectVisualizerAttribute 'outer'");
    fOuterRadius = value.ConvertTo<float>();
  }

  ezTempHybridArray<ezVec3, 64> ref_lines;

  if (fSizeX > 0 || fSizeY > 0 || fInnerRadius > 0)
  {
    BuildRoundedRectLines(ref_lines, fSizeX, fSizeY, fInnerRadius);
    m_hLinesInner.SetLines(ref_lines);
    m_hLinesInner.SetVisible(m_bVisualizerIsVisible);
  }
  else
  {
    m_hLinesInner.SetVisible(false);
  }

  if (fOuterRadius > 0)
  {
    BuildRoundedRectLines(ref_lines, fSizeX, fSizeY, fInnerRadius + fOuterRadius);
    m_hLinesOuter.SetLines(ref_lines);
    m_hLinesOuter.SetVisible(m_bVisualizerIsVisible);
  }
  else
  {
    m_hLinesOuter.SetVisible(false);
  }
}

void ezRoundedRectVisualizerAdapter::UpdateGizmoTransform()
{
  const ezRoundedRectVisualizerAttribute* pAttr = static_cast<const ezRoundedRectVisualizerAttribute*>(m_pVisualizerAttr);

  ezTransform t = GetObjectTransform();
  t.m_vPosition += t.m_qRotation * pAttr->m_vOffset;

  m_hLinesInner.SetTransformation(t);
  m_hLinesOuter.SetTransformation(t);
}
