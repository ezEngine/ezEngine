#include <PCH.h>

#include <EnginePluginScene/PickingRenderPass/PickingRenderPass.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPickingRenderPass, 1, ezRTTIDefaultAllocator<ezPickingRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PickSelected", m_bPickSelected),
    EZ_MEMBER_PROPERTY("PickingPosition", m_PickingPosition),
    EZ_MEMBER_PROPERTY("MarqueePickPos0", m_MarqueePickPosition0),
    EZ_MEMBER_PROPERTY("MarqueePickPos1", m_MarqueePickPosition1),
    EZ_MEMBER_PROPERTY("MarqueeActionID", m_uiMarqueeActionID),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPickingRenderPass::ezPickingRenderPass()
    : ezRenderPipelinePass("EditorPickingRenderPass")
{
  m_bPickSelected = true;
  AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));

  m_PickingPosition.Set(-1);
  m_MarqueePickPosition0.Set(-1);
  m_MarqueePickPosition1.Set(-1);
  m_uiMarqueeActionID = 0xFFFFFFFF;
}

ezPickingRenderPass::~ezPickingRenderPass()
{
  DestroyTarget();
}

ezGALTextureHandle ezPickingRenderPass::GetPickingIdRT() const
{
  return m_hPickingIdRT;
}

ezGALTextureHandle ezPickingRenderPass::GetPickingDepthRT() const
{
  return m_hPickingDepthRT;
}

bool ezPickingRenderPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
                                                      ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  m_TargetRect = view.GetViewport();

  return true;
}

void ezPickingRenderPass::InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
                                                 const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  DestroyTarget();
  CreateTarget();
}

void ezPickingRenderPass::Execute(const ezRenderViewContext& renderViewContext,
                                  const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
                                  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  const ezRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
  m_uiWindowWidth = (ezUInt32)viewPortRect.width;
  m_uiWindowHeight = (ezUInt32)viewPortRect.height;

  const ezGALTexture* pDepthTexture = ezGALDevice::GetDefaultDevice()->GetTexture(m_hPickingDepthRT);
  EZ_ASSERT_DEV(m_uiWindowWidth == pDepthTexture->GetDescription().m_uiWidth, "");
  EZ_ASSERT_DEV(m_uiWindowHeight == pDepthTexture->GetDescription().m_uiHeight, "");

  renderViewContext.m_pRenderContext->SetViewportAndRenderTargetSetup(viewPortRect, m_RenderTargetSetup);

  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f));

  ezViewRenderMode::Enum viewRenderMode = renderViewContext.m_pViewData->m_ViewRenderMode;
  if (viewRenderMode == ezViewRenderMode::WireframeColor || viewRenderMode == ezViewRenderMode::WireframeMonochrome)
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING_WIREFRAME");
  else
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING");

  // copy selection to set for faster checks
  m_SelectionSet.Clear();

  auto batchList = GetPipeline()->GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Selection);
  const ezUInt32 uiBatchCount = batchList.GetBatchCount();
  for (ezUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const ezRenderDataBatch& batch = batchList.GetBatch(i);
    for (auto it = batch.GetIterator<ezRenderData>(); it.IsValid(); ++it)
    {
      m_SelectionSet.Insert(it->m_hOwner);
    }
  }

  // filter out all selected objects
  ezRenderDataBatch::Filter filter([&](const ezRenderData* pRenderData) { return m_SelectionSet.Contains(pRenderData->m_hOwner); });

  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaque, filter);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMasked, filter);

  if (m_bPickSelected)
  {
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Selection);
  }

  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleOpaque);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_FORWARD");

  {{// download the picking information from the GPU
    if (m_uiWindowWidth != 0 &&
        m_uiWindowHeight != 0){ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(GetPickingDepthRT());

  ezMat4 mProj;
  renderViewContext.m_pCamera->GetProjectionMatrix((float)m_uiWindowWidth / m_uiWindowHeight, mProj);
  ezMat4 mView = renderViewContext.m_pCamera->GetViewMatrix();

  if (mProj.IsNaN())
    return;

  // Double precision version
  /*
  {
    ezMat4d dView, dProj, dMVP;
    CopyMatD(dView, mView);
    CopyMatD(dProj, mProj);

    dMVP = dProj * dView;
    auto res = dMVP.Invert(0.00000001);

    if (res.Failed())
      ezLog::Debug("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");

    m_PickingInverseViewProjectionMatrix = dMVP;
  }
  */

  ezMat4 inv = mProj * mView;
  if (inv.Invert(0).Failed())
  {
    ezLog::Warning("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");
    return;
  }

  m_PickingInverseViewProjectionMatrix = inv;

  m_PickingResultsDepth.Clear();
  m_PickingResultsDepth.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);

  ezGALSystemMemoryDescription MemDesc;
  MemDesc.m_uiRowPitch = 4 * m_uiWindowWidth;
  MemDesc.m_uiSlicePitch = 4 * m_uiWindowWidth * m_uiWindowHeight;

  MemDesc.m_pData = m_PickingResultsDepth.GetData();
  ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescsDepth(&MemDesc, 1);
  ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(GetPickingDepthRT(), &SysMemDescsDepth);
}
}
}

{
  // download the picking information from the GPU
  if (m_uiWindowWidth != 0 && m_uiWindowHeight != 0)
  {
    ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(GetPickingIdRT());

    ezMat4 mProj;
    renderViewContext.m_pCamera->GetProjectionMatrix((float)m_uiWindowWidth / m_uiWindowHeight, mProj);
    ezMat4 mView = renderViewContext.m_pCamera->GetViewMatrix();

    if (mProj.IsNaN())
      return;

    // Double precision version
    /*
    {
      ezMat4d dView, dProj, dMVP;
      CopyMatD(dView, mView);
      CopyMatD(dProj, mProj);

      dMVP = dProj * dView;
      auto res = dMVP.Invert(0.00000001);

      if (res.Failed())
        ezLog::Debug("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");

      m_PickingInverseViewProjectionMatrix = dMVP;
    }
    */

    ezMat4 inv = mProj * mView;
    if (inv.Invert(0).Failed())
    {
      ezLog::Warning("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");
      return;
    }

    m_PickingInverseViewProjectionMatrix = inv;

    m_PickingResultsID.Clear();
    m_PickingResultsID.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);

    ezGALSystemMemoryDescription MemDesc;
    MemDesc.m_uiRowPitch = 4 * m_uiWindowWidth;
    MemDesc.m_uiSlicePitch = 4 * m_uiWindowWidth * m_uiWindowHeight;

    MemDesc.m_pData = m_PickingResultsID.GetData();
    ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
    ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(GetPickingIdRT(), &SysMemDescs);
  }
}
}

void ezPickingRenderPass::ReadBackProperties(ezView* pView)
{
  ReadBackPropertiesSinglePick(pView);
  ReadBackPropertiesMarqueePick(pView);
}

void ezPickingRenderPass::CreateTarget()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Create render target for picking
  ezGALTextureCreationDescription tcd;
  tcd.m_bAllowDynamicMipGeneration = false;
  tcd.m_bAllowShaderResourceView = false;
  tcd.m_bAllowUAV = false;
  tcd.m_bCreateRenderTarget = true;
  tcd.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
  tcd.m_ResourceAccess.m_bReadBack = true;
  tcd.m_Type = ezGALTextureType::Texture2D;
  tcd.m_uiWidth = (ezUInt32)m_TargetRect.width;
  tcd.m_uiHeight = (ezUInt32)m_TargetRect.height;

  m_hPickingIdRT = pDevice->CreateTexture(tcd);

  tcd.m_Format = ezGALResourceFormat::DFloat;
  tcd.m_ResourceAccess.m_bReadBack = true;

  m_hPickingDepthRT = pDevice->CreateTexture(tcd);

  m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hPickingIdRT))
      .SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hPickingDepthRT));
}

void ezPickingRenderPass::DestroyTarget()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_RenderTargetSetup.DestroyAllAttachedViews();
  if (!m_hPickingIdRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hPickingIdRT);
    m_hPickingIdRT.Invalidate();
  }

  if (!m_hPickingDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hPickingDepthRT);
    m_hPickingDepthRT.Invalidate();
  }
}

void ezPickingRenderPass::ReadBackPropertiesSinglePick(ezView* pView)
{
  const ezUInt32 x = (ezUInt32)m_PickingPosition.x;
  const ezUInt32 y = (ezUInt32)m_PickingPosition.y;
  const ezUInt32 uiIndex = (y * m_uiWindowWidth) + x;

  if (uiIndex >= m_PickingResultsDepth.GetCount() || x >= m_uiWindowWidth || y >= m_uiWindowHeight)
  {
    // ezLog::Error("Picking position {0}, {1} is outside the available picking area of {2} * {3}", x, y, m_uiWindowWidth,
    // m_uiWindowHeight);
    return;
  }

  m_PickingPosition.Set(-1);

  ezVec3 vNormal(0);
  ezVec3 vPickingRayStartPosition(0);
  ezVec3 vPickedPosition(0);
  {
    const float fDepth = m_PickingResultsDepth[uiIndex];
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight,
                                                ezVec3((float)x, (float)(m_uiWindowHeight - y), fDepth), vPickedPosition);
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight,
                                                ezVec3((float)x, (float)(m_uiWindowHeight - y), 0), vPickingRayStartPosition);

    float fOtherDepths[4] = {fDepth, fDepth, fDepth, fDepth};
    ezVec3 vOtherPos[4];
    ezVec3 vNormals[4];

    if ((ezUInt32)x + 1 < m_uiWindowWidth)
      fOtherDepths[0] = m_PickingResultsDepth[(y * m_uiWindowWidth) + x + 1];
    if (x > 0)
      fOtherDepths[1] = m_PickingResultsDepth[(y * m_uiWindowWidth) + x - 1];
    if ((ezUInt32)y + 1 < m_uiWindowHeight)
      fOtherDepths[2] = m_PickingResultsDepth[((y + 1) * m_uiWindowWidth) + x];
    if (y > 0)
      fOtherDepths[3] = m_PickingResultsDepth[((y - 1) * m_uiWindowWidth) + x];

    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight,
                                                ezVec3((float)(x + 1), (float)(m_uiWindowHeight - y), fOtherDepths[0]), vOtherPos[0]);
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight,
                                                ezVec3((float)(x - 1), (float)(m_uiWindowHeight - y), fOtherDepths[1]), vOtherPos[1]);
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight,
                                                ezVec3((float)x, (float)(m_uiWindowHeight - (y + 1)), fOtherDepths[2]), vOtherPos[2]);
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_PickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight,
                                                ezVec3((float)x, (float)(m_uiWindowHeight - (y - 1)), fOtherDepths[3]), vOtherPos[3]);

    vNormals[0] = ezPlane(vPickedPosition, vOtherPos[0], vOtherPos[2]).m_vNormal;
    vNormals[1] = ezPlane(vPickedPosition, vOtherPos[2], vOtherPos[1]).m_vNormal;
    vNormals[2] = ezPlane(vPickedPosition, vOtherPos[1], vOtherPos[3]).m_vNormal;
    vNormals[3] = ezPlane(vPickedPosition, vOtherPos[3], vOtherPos[0]).m_vNormal;

    vNormal = (vNormals[0] + vNormals[1] + vNormals[2] + vNormals[3]).GetNormalized();
  }

  ezUInt32 uiPickID = m_PickingResultsID[uiIndex];
  if (uiPickID == 0)
  {
    for (ezInt32 radius = 1; radius < 10; ++radius)
    {
      ezInt32 left = ezMath::Max<ezInt32>(x - radius, 0);
      ezInt32 right = ezMath::Min<ezInt32>(x + radius, m_uiWindowWidth - 1);
      ezInt32 top = ezMath::Max<ezInt32>(y - radius, 0);
      ezInt32 bottom = ezMath::Min<ezInt32>(y + radius, m_uiWindowHeight - 1);

      for (ezInt32 xt = left; xt <= right; ++xt)
      {
        const ezUInt32 idxt = (top * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto done;
      }

      for (ezInt32 xt = left; xt <= right; ++xt)
      {
        const ezUInt32 idxt = (bottom * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto done;
      }
    }

  done:;
  }

  pView->SetRenderPassReadBackProperty(GetName(), "PickingMatrix", m_PickingInverseViewProjectionMatrix);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingID", uiPickID);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingDepth", m_PickingResultsDepth[uiIndex]);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingNormal", vNormal);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingRayStartPosition", vPickingRayStartPosition);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingPosition", vPickedPosition);
}

void ezPickingRenderPass::ReadBackPropertiesMarqueePick(ezView* pView)
{
  const ezUInt32 x0 = (ezUInt32)m_MarqueePickPosition0.x;
  const ezUInt32 y0 = (ezUInt32)m_MarqueePickPosition0.y;
  const ezUInt32 x1 = (ezUInt32)m_MarqueePickPosition1.x;
  const ezUInt32 y1 = (ezUInt32)m_MarqueePickPosition1.y;
  const ezUInt32 uiIndex1 = (y0 * m_uiWindowWidth) + x0;
  const ezUInt32 uiIndex2 = (y0 * m_uiWindowWidth) + x0;

  if ((uiIndex1 >= m_PickingResultsDepth.GetCount() || x0 >= m_uiWindowWidth || y0 >= m_uiWindowHeight) ||
      (uiIndex2 >= m_PickingResultsDepth.GetCount() || x1 >= m_uiWindowWidth || y1 >= m_uiWindowHeight))
  {
    return;
  }

  m_MarqueePickPosition0.Set(-1);
  m_MarqueePickPosition1.Set(-1);
  pView->SetRenderPassReadBackProperty(GetName(), "MarqueeActionID", m_uiMarqueeActionID);

  ezHybridArray<ezUInt32, 32> IDs;
  ezVariantArray resArray;

  const ezUInt32 lowX = ezMath::Min(x0, x1);
  const ezUInt32 highX = ezMath::Max(x0, x1);
  const ezUInt32 lowY = ezMath::Min(y0, y1);
  const ezUInt32 highY = ezMath::Max(y0, y1);

  ezUInt32 offset = 0;

  for (ezUInt32 y = lowY; y < highY; y += 1)
  {
    for (ezUInt32 x = lowX + offset; x < highX; x += 2)
    {
      const ezUInt32 uiIndex = (y * m_uiWindowWidth) + x;

      const ezUInt32 id = m_PickingResultsID[uiIndex];

      // prevent duplicates
      if (IDs.Contains(id))
        continue;

      IDs.PushBack(id);
      resArray.PushBack(id);
    }

    // only evaluate every second pixel, in a checker board pattern
    offset = (offset + 1) % 2;
  }

  pView->SetRenderPassReadBackProperty(GetName(), "MarqueeResult", resArray);
}
