#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Pipeline/SortingFunctions.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPickingRenderPass, 1, ezRTTIDefaultAllocator<ezPickingRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PickSelected", m_bPickSelected),
    EZ_MEMBER_PROPERTY("PickTransparent", m_bPickTransparent),
    EZ_MEMBER_PROPERTY("PickingPosition", m_PickingPosition),
    EZ_MEMBER_PROPERTY("MarqueePickPos0", m_MarqueePickPosition0),
    EZ_MEMBER_PROPERTY("MarqueePickPos1", m_MarqueePickPosition1),
    EZ_MEMBER_PROPERTY("MarqueeActionID", m_uiMarqueeActionID),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezRenderData::Category s_LitOpaqueWithoutSelection = ezRenderData::RegisterDerivedCategory("LitOpaqueWithoutSelection", ezDefaultRenderDataCategories::LitOpaqueStatic);
static ezRenderData::Category s_LitMaskedWithoutSelection = ezRenderData::RegisterDerivedCategory("LitMaskedWithoutSelection", ezDefaultRenderDataCategories::LitMaskedStatic);
static ezRenderData::Category s_LitMaskedDynamicWithoutSelection = ezRenderData::RegisterDerivedCategory("LitMaskedDynamicWithoutSelection", ezDefaultRenderDataCategories::LitMaskedDynamic);

static ezRenderData::Category s_LitTransparentWithoutSelection = ezRenderData::RegisterDerivedCategory("LitTransparentWithoutSelection", ezDefaultRenderDataCategories::LitTransparent);
static ezRenderData::Category s_SimpleTransparentWithoutSelection = ezRenderData::RegisterDerivedCategory("SimpleTransparentWithoutSelection", ezDefaultRenderDataCategories::SimpleTransparent);

ezPickingRenderPass::ezPickingRenderPass()
  : ezRenderPipelinePass("EditorPickingRenderPass")
{
  m_pGridRenderDataType = ezRTTI::FindTypeByName("ezGridRenderData");
  EZ_ASSERT_DEV(m_pGridRenderDataType != nullptr, "ezGridRenderData type not found. Type renamed?");
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

bool ezPickingRenderPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  m_TargetRect = view.GetViewport();

  return true;
}

void ezPickingRenderPass::InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  DestroyTarget();
  CreateTarget();

  if (m_uiProcessorId == ezInvalidIndex)
  {
    m_uiProcessorId = GetPipeline()->AddRenderDataProcessor(ezMakeDelegate(&ezPickingRenderPass::ProcessPickingRenderData, this));
  }
}

void ezPickingRenderPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  const ezRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
  m_uiWindowWidth = (ezUInt32)viewPortRect.width;
  m_uiWindowHeight = (ezUInt32)viewPortRect.height;

  const ezGALTexture* pDepthTexture = ezGALDevice::GetDefaultDevice()->GetTexture(m_hPickingDepthRT);
  EZ_ASSERT_DEV(m_uiWindowWidth == pDepthTexture->GetDescription().m_uiWidth, "");
  EZ_ASSERT_DEV(m_uiWindowHeight == pDepthTexture->GetDescription().m_uiHeight, "");

  {
    auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, m_RenderTargetSetup, GetName());

    ezViewRenderMode::Enum viewRenderMode = renderViewContext.m_pViewData->m_ViewRenderMode;
    if (viewRenderMode == ezViewRenderMode::WireframeColor || viewRenderMode == ezViewRenderMode::WireframeMonochrome)
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING_WIREFRAME");
    else
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING");

    // Setup clustered data
    auto pClusteredData = GetPipeline()->GetFrameDataProvider<ezClusteredDataProvider>()->GetData(renderViewContext);
    pClusteredData->BindResources(renderViewContext.m_pRenderContext);

    RenderDataWithCategory(renderViewContext, s_LitOpaqueWithoutSelection);
    RenderDataWithCategory(renderViewContext, s_LitMaskedWithoutSelection);

    if (m_bPickTransparent)
    {
      RenderDataWithCategory(renderViewContext, s_LitTransparentWithoutSelection);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);
    }

    if (m_bPickSelected)
    {
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Selection);
    }

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleOpaque);

    if (m_bPickTransparent)
    {
      RenderDataWithCategory(renderViewContext, s_SimpleTransparentWithoutSelection);
    }

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_FORWARD");
  }

  if (m_PendingReadback.m_bReadbackInProgress)
  {
    // Wait for results
    {
      ezEnum<ezGALAsyncResult> res = m_PendingReadback.m_PickingReadback.GetReadbackResult(ezTime::MakeFromHours(1));
      EZ_ASSERT_ALWAYS(res == ezGALAsyncResult::Ready, "Readback of texture failed");
      res = m_PendingReadback.m_PickingDepthReadback.GetReadbackResult(ezTime::MakeFromHours(1));
      EZ_ASSERT_ALWAYS(res == ezGALAsyncResult::Ready, "Readback of texture failed");
      m_PendingReadback.m_bReadbackInProgress = false;
    }

    ezGALTextureSubresource sourceSubResource;
    ezArrayPtr<ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
    ezHybridArray<ezGALSystemMemoryDescription, 1> memory;

    m_PickingResultsDepth.Clear();
    m_PickingResultsID.Clear();
    m_mPickingInverseViewProjectionMatrix = ezMat4::MakeZero();
    // If the resolution has changed, discard the readback result.
    if (m_uiWindowHeight == m_PendingReadback.m_uiWindowHeight && m_uiWindowWidth == m_PendingReadback.m_uiWindowWidth)
    {
      {
        m_PickingResultsDepth.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);
        ezReadbackTextureLock lock = m_PendingReadback.m_PickingDepthReadback.LockTexture(sourceSubResources, memory);
        EZ_ASSERT_ALWAYS(lock, "Failed to lock readback texture");
        const ezGALTexture* pReadbackTexture = ezGALDevice::GetDefaultDevice()->GetTexture(GetPickingDepthRT());
        ezTextureUtils::CopySubResourceToMemory(pReadbackTexture->GetDescription(), sourceSubResource, memory[0], m_PickingResultsDepth.GetByteArrayPtr(), m_uiWindowWidth * sizeof(float));
      }
      {
        m_PickingResultsID.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);
        ezReadbackTextureLock lock = m_PendingReadback.m_PickingReadback.LockTexture(sourceSubResources, memory);
        EZ_ASSERT_ALWAYS(lock, "Failed to lock readback texture");
        const ezGALTexture* pReadbackTexture = ezGALDevice::GetDefaultDevice()->GetTexture(GetPickingIdRT());
        ezTextureUtils::CopySubResourceToMemory(pReadbackTexture->GetDescription(), sourceSubResource, memory[0], m_PickingResultsID.GetByteArrayPtr(), m_uiWindowWidth * sizeof(ezUInt32));
      }
      m_mPickingInverseViewProjectionMatrix = m_PendingReadback.m_mPickingInverseViewProjectionMatrix;
    }
  }

  // Start transferring the picking information from the GPU to the CPU
  if (m_uiWindowWidth != 0 && m_uiWindowHeight != 0)
  {
    m_PendingReadback.m_PickingReadback.ReadbackTexture(*renderViewContext.m_pRenderContext->GetCommandEncoder(), GetPickingIdRT());
    m_PendingReadback.m_PickingDepthReadback.ReadbackTexture(*renderViewContext.m_pRenderContext->GetCommandEncoder(), GetPickingDepthRT());
    renderViewContext.m_pRenderContext->GetCommandEncoder()->Flush();

    ezMat4 mProj;
    renderViewContext.m_pCamera->GetProjectionMatrix((float)m_uiWindowWidth / m_uiWindowHeight, mProj);
    ezMat4 mView = renderViewContext.m_pCamera->GetViewMatrix();

    if (mProj.IsNaN())
      return;

    ezMat4 inv = mProj * mView;
    if (inv.Invert(0).Failed())
    {
      ezLog::Warning("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");
      return;
    }

    m_PendingReadback.m_mPickingInverseViewProjectionMatrix = inv;
    m_PendingReadback.m_uiWindowWidth = m_uiWindowWidth;
    m_PendingReadback.m_uiWindowHeight = m_uiWindowHeight;
    m_PendingReadback.m_bReadbackInProgress = true;
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
  tcd.m_bAllowRenderTargetView = true;
  tcd.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
  tcd.m_Type = ezGALTextureType::Texture2D;
  tcd.m_uiWidth = (ezUInt32)m_TargetRect.width;
  tcd.m_uiHeight = (ezUInt32)m_TargetRect.height;

  m_hPickingIdRT = pDevice->CreateTexture(tcd);

  tcd.m_Format = ezGALResourceFormat::DFloat;

  m_hPickingDepthRT = pDevice->CreateTexture(tcd);

  m_RenderTargetSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(m_hPickingIdRT)).SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hPickingDepthRT));
  m_RenderTargetSetup.SetClearColor(0);
  m_RenderTargetSetup.SetClearDepth().SetClearStencil();
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
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, ezVec3((float)x, (float)y, fDepth), vPickedPosition).IgnoreResult();
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, ezVec3((float)x, (float)y, 0), vPickingRayStartPosition).IgnoreResult();

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

    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, ezVec3((float)(x + 1), (float)y, fOtherDepths[0]), vOtherPos[0]).IgnoreResult();
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, ezVec3((float)(x - 1), (float)y, fOtherDepths[1]), vOtherPos[1]).IgnoreResult();
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, ezVec3((float)x, (float)(y + 1), fOtherDepths[2]), vOtherPos[2]).IgnoreResult();
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, ezVec3((float)x, (float)(y - 1), fOtherDepths[3]), vOtherPos[3]).IgnoreResult();

    vNormals[0].CalculateNormal(vPickedPosition, vOtherPos[0], vOtherPos[2]).IgnoreResult();
    vNormals[1].CalculateNormal(vPickedPosition, vOtherPos[2], vOtherPos[1]).IgnoreResult();
    vNormals[2].CalculateNormal(vPickedPosition, vOtherPos[1], vOtherPos[3]).IgnoreResult();
    vNormals[3].CalculateNormal(vPickedPosition, vOtherPos[3], vOtherPos[0]).IgnoreResult();

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

  pView->SetRenderPassReadBackProperty(GetName(), "PickedMatrix", m_mPickingInverseViewProjectionMatrix);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedID", uiPickID);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedDepth", m_PickingResultsDepth[uiIndex]);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedNormal", vNormal);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedRayStartPosition", vPickingRayStartPosition);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedPosition", vPickedPosition);
}

void ezPickingRenderPass::ReadBackPropertiesMarqueePick(ezView* pView)
{
  const ezUInt32 x0 = (ezUInt32)m_MarqueePickPosition0.x;
  const ezUInt32 y0 = (ezUInt32)m_MarqueePickPosition0.y;
  const ezUInt32 x1 = (ezUInt32)m_MarqueePickPosition1.x;
  const ezUInt32 y1 = (ezUInt32)m_MarqueePickPosition1.y;
  const ezUInt32 uiIndex1 = (y0 * m_uiWindowWidth) + x0;
  const ezUInt32 uiIndex2 = (y0 * m_uiWindowWidth) + x0;

  if ((uiIndex1 >= m_PickingResultsDepth.GetCount() || x0 >= m_uiWindowWidth || y0 >= m_uiWindowHeight) || (uiIndex2 >= m_PickingResultsDepth.GetCount() || x1 >= m_uiWindowWidth || y1 >= m_uiWindowHeight))
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

void ezPickingRenderPass::ProcessPickingRenderData(ezExtractedRenderData& extractedRenderData)
{
  // copy selection to set for faster checks
  m_SelectionSet.Clear();
  {
    auto renderDataList = extractedRenderData.GetRawRenderDataWithCategory(ezDefaultRenderDataCategories::Selection);
    for (auto& sortableRenderData : renderDataList)
    {
      m_SelectionSet.Insert(sortableRenderData.m_pRenderData->m_hOwner);
    }
  }

  auto Filter = [&](ezRenderData::Category originalCategory, ezRenderData::Category filteredCategory)
  {
    auto renderDataList = extractedRenderData.GetRawRenderDataWithCategory(originalCategory);
    for (auto& sortableRenderData : renderDataList)
    {
      auto pRenderData = sortableRenderData.m_pRenderData;
      if (m_SelectionSet.Contains(pRenderData->m_hOwner) || pRenderData->IsInstanceOf(m_pGridRenderDataType))
        continue;

      extractedRenderData.AddRenderData(pRenderData, filteredCategory);
    }
  };

  Filter(ezDefaultRenderDataCategories::LitOpaqueStatic, s_LitOpaqueWithoutSelection);
  Filter(ezDefaultRenderDataCategories::LitOpaqueDynamic, s_LitOpaqueWithoutSelection);

  Filter(ezDefaultRenderDataCategories::LitMaskedStatic, s_LitMaskedWithoutSelection);
  Filter(ezDefaultRenderDataCategories::LitMaskedDynamic, s_LitOpaqueWithoutSelection);

  if (m_bPickTransparent)
  {
    Filter(ezDefaultRenderDataCategories::LitTransparent, s_LitTransparentWithoutSelection);
    Filter(ezDefaultRenderDataCategories::SimpleTransparent, s_SimpleTransparentWithoutSelection);
  }
}
