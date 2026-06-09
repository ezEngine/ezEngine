#include <EditorPluginTerrain/EditorPluginTerrainPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush3DVisualizerAdapter.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezTerrainBrush3DVisualizerAdapter::ezTerrainBrush3DVisualizerAdapter() = default;

ezTerrainBrush3DVisualizerAdapter::~ezTerrainBrush3DVisualizerAdapter() = default;

void ezTerrainBrush3DVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezTerrainBrush3DVisualizerAttribute* pAttr = static_cast<const ezTerrainBrush3DVisualizerAttribute*>(m_pVisualizerAttr);

  m_hLinesInner.ConfigureHandle(nullptr, ezEngineGizmoHandleType::CustomLines, pAttr->m_InnerColor, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);
  m_hLinesOuter.ConfigureHandle(nullptr, ezEngineGizmoHandleType::CustomLines, pAttr->m_OuterColor, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hLinesInner);
  pAssetDocument->AddSyncObject(&m_hLinesOuter);
}

void BuildRoundedRectAtZ(ezDynamicArray<ezVec3>& ref_lines, float fHalfX, float fHalfY, float fRadius, float fZ);

// Draws a rounded rectangle in the XZ plane at the given Y offset.
// Delegates to BuildRoundedRectAtZ, then rotates the appended points into XZ by swapping Y and Z.
static void BuildRoundedRectAtY(ezDynamicArray<ezVec3>& ref_lines, float fHalfX, float fHalfZ, float fRadius, float fY)
{
  const ezUInt32 uiBefore = ref_lines.GetCount();
  BuildRoundedRectAtZ(ref_lines, fHalfX, fHalfZ, fRadius, fY);
  for (ezUInt32 i = uiBefore; i < ref_lines.GetCount(); ++i)
    ezMath::Swap(ref_lines[i].y, ref_lines[i].z);
}

static void BuildRoundedBoxLines(ezDynamicArray<ezVec3>& ref_lines, float fHalfX, float fHalfYBottom, float fHalfYTop, float fHalfZ, float fRadius)
{
  ref_lines.Clear();

  // Two rounded rectangles in the XY plane:
  // - at Z = -fHalfZ with Y half-extent fHalfYBottom
  // - at Z = +fHalfZ with Y half-extent fHalfYTop
  BuildRoundedRectAtZ(ref_lines, fHalfX, fHalfYBottom, fRadius, -fHalfZ);
  BuildRoundedRectAtZ(ref_lines, fHalfX, fHalfYTop, fRadius, +fHalfZ);

  // 4 connecting edges between the corner arc-centers of the two faces, forming the trapezoid sides.
  const float fCornerX[2] = {+fHalfX, -fHalfX};
  const float fCornerYBottom[2] = {+fHalfYBottom, -fHalfYBottom};
  const float fCornerYTop[2] = {+fHalfYTop, -fHalfYTop};

  for (ezUInt32 xi = 0; xi < 2; ++xi)
  {
    ref_lines.PushBack(ezVec3(fCornerX[xi], fCornerYBottom[0] + fRadius, -fHalfZ));
    ref_lines.PushBack(ezVec3(fCornerX[xi], fCornerYTop[0] + fRadius, +fHalfZ));

    ref_lines.PushBack(ezVec3(fCornerX[xi], fCornerYBottom[1] - fRadius, -fHalfZ));
    ref_lines.PushBack(ezVec3(fCornerX[xi], fCornerYTop[1] - fRadius, +fHalfZ));
  }
}

void ezTerrainBrush3DVisualizerAdapter::Update()
{
  const ezTerrainBrush3DVisualizerAttribute* pAttr = static_cast<const ezTerrainBrush3DVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  float fSizeX = 0;
  float fSizeYBottom = 0;
  float fSizeYTop = 0;
  float fSizeZ = 0;
  float fInnerRadius = 0;
  float fOuterRadius = 0;

  if (!pAttr->GetPropHalfSizeX().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeX()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush3DVisualizerAttribute 'size x'");
    fSizeX = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropHalfSizeYBottom().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeYBottom()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush3DVisualizerAttribute 'size y-bottom'");
    fSizeYBottom = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropHalfSizeYTop().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeYTop()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush3DVisualizerAttribute 'size y-top'");
    fSizeYTop = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropHalfSizeZ().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropHalfSizeZ()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush3DVisualizerAttribute 'size z'");
    fSizeZ = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropInnerRadius().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropInnerRadius()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush3DVisualizerAttribute 'inner radius'");
    fInnerRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetPropOuterRadius().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPropOuterRadius()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezTerrainBrush3DVisualizerAttribute 'outer radius'");
    fOuterRadius = value.ConvertTo<float>();
  }

  if (fSizeZ == 0)
  {
    fSizeYBottom = ezMath::Max(fSizeYBottom, fSizeYTop);
    fSizeYTop = fSizeYBottom;
  }

  ezTempHybridArray<ezVec3, 128> ref_lines;

  if (fSizeX > 0 || fSizeYBottom > 0 || fSizeYTop > 0 || fSizeZ > 0 || fInnerRadius > 0)
  {
    BuildRoundedBoxLines(ref_lines, fSizeX, fSizeYBottom, fSizeYTop, fSizeZ, fInnerRadius);

    if (fInnerRadius > 0)
    {
      BuildRoundedRectAtY(ref_lines, fSizeX, fSizeZ, fInnerRadius, 0.0f);
    }

    m_hLinesInner.SetLines(ref_lines);
    m_hLinesInner.SetVisible(m_bVisualizerIsVisible);
  }
  else
  {
    m_hLinesInner.SetVisible(false);
  }

  if (fOuterRadius > 0)
  {
    BuildRoundedBoxLines(ref_lines, fSizeX, fSizeYBottom, fSizeYTop, fSizeZ, fInnerRadius + fOuterRadius);
    m_hLinesOuter.SetLines(ref_lines);
    m_hLinesOuter.SetVisible(m_bVisualizerIsVisible);
  }
  else
  {
    m_hLinesOuter.SetVisible(false);
  }
}

void ezTerrainBrush3DVisualizerAdapter::UpdateGizmoTransform()
{
  const ezTerrainBrush3DVisualizerAttribute* pAttr = static_cast<const ezTerrainBrush3DVisualizerAttribute*>(m_pVisualizerAttr);

  ezTransform t = GetObjectTransform();
  t.m_vPosition += t.m_qRotation * pAttr->m_vOffset;

  m_hLinesInner.SetTransformation(t);
  m_hLinesOuter.SetTransformation(t);
}
