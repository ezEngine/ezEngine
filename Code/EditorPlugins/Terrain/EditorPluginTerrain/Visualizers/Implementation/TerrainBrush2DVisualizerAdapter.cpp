#include <EditorPluginTerrain/EditorPluginTerrainPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush2DVisualizerAdapter.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezTerrainBrush2DVisualizerAdapter::ezTerrainBrush2DVisualizerAdapter() = default;

ezTerrainBrush2DVisualizerAdapter::~ezTerrainBrush2DVisualizerAdapter() = default;

void ezTerrainBrush2DVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezTerrainBrush2DVisualizerAttribute* pAttr = static_cast<const ezTerrainBrush2DVisualizerAttribute*>(m_pVisualizerAttr);

  m_hLinesInner.ConfigureHandle(nullptr, ezEngineGizmoHandleType::CustomLines, pAttr->m_InnerColor, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);
  m_hLinesOuter.ConfigureHandle(nullptr, ezEngineGizmoHandleType::CustomLines, pAttr->m_OuterColor, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hLinesInner);
  pAssetDocument->AddSyncObject(&m_hLinesOuter);
}

// Draws a rounded rectangle in the XY plane at the given Z offset.
void BuildRoundedRectAtZ(ezDynamicArray<ezVec3>& ref_lines, float fHalfX, float fHalfY, float fRadius, float fZ)
{
  if (fHalfX > 0.0f)
  {
    ref_lines.PushBack(ezVec3(-fHalfX, -(fHalfY + fRadius), fZ));
    ref_lines.PushBack(ezVec3(+fHalfX, -(fHalfY + fRadius), fZ));

    ref_lines.PushBack(ezVec3(+fHalfX, +(fHalfY + fRadius), fZ));
    ref_lines.PushBack(ezVec3(-fHalfX, +(fHalfY + fRadius), fZ));
  }
  if (fHalfY > 0.0f)
  {
    ref_lines.PushBack(ezVec3(+(fHalfX + fRadius), -fHalfY, fZ));
    ref_lines.PushBack(ezVec3(+(fHalfX + fRadius), +fHalfY, fZ));

    ref_lines.PushBack(ezVec3(-(fHalfX + fRadius), +fHalfY, fZ));
    ref_lines.PushBack(ezVec3(-(fHalfX + fRadius), -fHalfY, fZ));
  }

  if (fRadius <= 0.0f)
    return;

  constexpr ezUInt32 uiSegmentsPerQuarter = 8;
  constexpr float fStep = ezMath::Pi<float>() * 0.5f / uiSegmentsPerQuarter;

  const ezVec2 vCorners[4] = {
    ezVec2(+fHalfX, -fHalfY),
    ezVec2(+fHalfX, +fHalfY),
    ezVec2(-fHalfX, +fHalfY),
    ezVec2(-fHalfX, -fHalfY),
  };
  const float fStartAngles[4] = {
    -ezMath::Pi<float>() * 0.5f,
    0.0f,
    ezMath::Pi<float>() * 0.5f,
    ezMath::Pi<float>(),
  };

  for (ezUInt32 c = 0; c < 4; ++c)
  {
    for (ezUInt32 s = 0; s < uiSegmentsPerQuarter; ++s)
    {
      const ezAngle fA0 = ezAngle::MakeFromRadian(fStartAngles[c] + s * fStep);
      const ezAngle fA1 = ezAngle::MakeFromRadian(fStartAngles[c] + (s + 1) * fStep);

      ref_lines.PushBack(ezVec3(vCorners[c].x + fRadius * ezMath::Cos(fA0), vCorners[c].y + fRadius * ezMath::Sin(fA0), fZ));
      ref_lines.PushBack(ezVec3(vCorners[c].x + fRadius * ezMath::Cos(fA1), vCorners[c].y + fRadius * ezMath::Sin(fA1), fZ));
    }
  }
}

static void BuildRoundedRectLines(ezDynamicArray<ezVec3>& ref_lines, float fHalfX, float fHalfY, float fRadius)
{
  ref_lines.Clear();
  BuildRoundedRectAtZ(ref_lines, fHalfX, fHalfY, fRadius, 0.0f);
}

void ezTerrainBrush2DVisualizerAdapter::Update()
{
  const ezTerrainBrush2DVisualizerAttribute* pAttr = static_cast<const ezTerrainBrush2DVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  float fSizeX = 0;
  float fSizeY = 0;
  float fInnerRadius = 0;
  float fOuterRadius = 0;

  if (!pAttr->GetPropHalfSizeX().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeX()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush2DVisualizerAttribute 'size x'");
    fSizeX = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropHalfSizeY().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeY()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush2DVisualizerAttribute 'size y'");
    fSizeY = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropInnerRadius().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropInnerRadius()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush2DVisualizerAttribute 'inner'");
    fInnerRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropOuterRadius().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropOuterRadius()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush2DVisualizerAttribute 'outer'");
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

void ezTerrainBrush2DVisualizerAdapter::UpdateGizmoTransform()
{
  const ezTerrainBrush2DVisualizerAttribute* pAttr = static_cast<const ezTerrainBrush2DVisualizerAttribute*>(m_pVisualizerAttr);

  ezTransform t = GetObjectTransform();
  t.m_vPosition += t.m_qRotation * pAttr->m_vOffset;

  m_hLinesInner.SetTransformation(t);
  m_hLinesOuter.SetTransformation(t);
}
