#include <RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/LSAOPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezLSAODepthCompareFunction, 1)
  EZ_ENUM_CONSTANT(ezLSAODepthCompareFunction::Depth),
  EZ_ENUM_CONSTANT(ezLSAODepthCompareFunction::Normal),
  EZ_ENUM_CONSTANT(ezLSAODepthCompareFunction::NormalAndSampleDistance),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLSAOPass, 1, ezRTTIDefaultAllocator<ezLSAOPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    EZ_MEMBER_PROPERTY("AmbientObscurance", m_PinOutput),
    EZ_ACCESSOR_PROPERTY("LineToLineDistance", GetLineToLinePixelOffset, SetLineToLinePixelOffset)->AddAttributes(new ezDefaultValueAttribute(2), new ezClampValueAttribute(1, 20)),
    EZ_ACCESSOR_PROPERTY("LineSampleDistanceFactor", GetLineSamplePixelOffset, SetLineSamplePixelOffset)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 10)),
    EZ_ACCESSOR_PROPERTY("OcclusionFalloff", GetOcclusionFalloff, SetOcclusionFalloff)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.01f, 2.0f)),
    EZ_ENUM_MEMBER_PROPERTY("DepthCompareFunction", ezLSAODepthCompareFunction, m_DepthCompareFunction),
    EZ_ACCESSOR_PROPERTY("DepthCutoffDistance", GetDepthCutoffDistance, SetDepthCutoffDistance)->AddAttributes(new ezDefaultValueAttribute(4.0f), new ezClampValueAttribute(0.1f, 100.0f)),
    EZ_MEMBER_PROPERTY("DistributedGathering", m_bDistributedGathering)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  float HaltonSequence(int base, int j)
  {
    static int primes[61] = {
      2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283};

    EZ_ASSERT_DEV(base < 61, "Don't have prime number for this base.");

    // Halton sequence with reverse permutation
    const int p = primes[base];
    float h = 0.0f;
    float f = 1.0f / static_cast<float>(p);
    float fct = f;
    while (j > 0)
    {
      int i = j % p;
      h += (i == 0 ? i : p - i) * fct;
      j /= p;
      fct *= f;
    }
    return h;
  }
} // namespace

ezLSAOPass::ezLSAOPass()
  : ezRenderPipelinePass("LSAOPass")
  , m_uiLineToLinePixelOffset(2)
  , m_uiLineSamplePixelOffsetFactor(1)
  , m_bSweepDataDirty(true)
  , m_bDistributedGathering(true)
{
  {
    // Load shader.
    m_hShaderLineSweep = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/LSAOSweep.ezShader");
    EZ_ASSERT_DEV(m_hShaderLineSweep.IsValid(), "Could not lsao sweep shader!");
    m_hShaderGather = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/LSAOGather.ezShader");
    EZ_ASSERT_DEV(m_hShaderGather.IsValid(), "Could not lsao gather shader!");
    m_hShaderAverage = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/LSAOAverage.ezShader");
    EZ_ASSERT_DEV(m_hShaderGather.IsValid(), "Could not lsao average shader!");
  }

  {
    m_hLineSweepCB = ezRenderContext::CreateConstantBufferStorage<ezLSAOConstants>();
    ezLSAOConstants* cb = ezRenderContext::GetConstantBufferData<ezLSAOConstants>(m_hLineSweepCB);
    cb->DepthCutoffDistance = 8.0f;
    cb->OcclusionFalloff = 0.25f;
  }
}

ezLSAOPass::~ezLSAOPass()
{
  DestroyLineSweepData();

  ezRenderContext::DeleteConstantBufferStorage(m_hLineSweepCB);
  m_hLineSweepCB.Invalidate();
}

bool ezLSAOPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  EZ_ASSERT_DEBUG(inputs.GetCount() == 1, "Unexpected number of inputs for ezScreenSpaceAmbientOcclusionPass.");

  // Depth
  if (!inputs[m_PinDepthInput.m_uiInputIndex])
  {
    ezLog::Error("No depth input connected to ssao pass!");
    return false;
  }
  if (!inputs[m_PinDepthInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    ezLog::Error("All ssao pass inputs must allow shader resoure view.");
    return false;
  }

  // Output format matches input format but is f16.
  outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinDepthInput.m_uiInputIndex];
  outputs[m_PinOutput.m_uiOutputIndex].m_Format = ezGALResourceFormat::RGHalf;

  return true;
}

void ezLSAOPass::InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  // Todo: Support half resolution.
  SetupLineSweepData(ezVec2I32(inputs[m_PinDepthInput.m_uiInputIndex]->m_Desc.m_uiWidth, inputs[m_PinDepthInput.m_uiInputIndex]->m_Desc.m_uiHeight));
}

void ezLSAOPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (m_bSweepDataDirty)
    SetupLineSweepData(ezVec2I32(inputs[m_PinDepthInput.m_uiInputIndex]->m_Desc.m_uiWidth, inputs[m_PinDepthInput.m_uiInputIndex]->m_Desc.m_uiHeight));

  if (outputs[m_PinOutput.m_uiOutputIndex] == nullptr)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALPass* pGALPass = pDevice->BeginPass(GetName());
  EZ_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  EZ_ASSERT_NOT_IMPLEMENTED;
#if 0
  // Set rendertarget immediately to ensure that depth buffer is no longer bound (we need it right away in the sweeping part)
  ezGALRenderingSetup renderingSetup;
  ezGALTextureHandle tempTexture;
  if (m_bDistributedGathering)
  {
    ezGALTextureCreationDescription tempTextureDesc = outputs[m_PinOutput.m_uiOutputIndex]->m_Desc;
    tempTextureDesc.m_bAllowShaderResourceView = true;
    tempTextureDesc.m_bCreateRenderTarget = true;
    tempTexture = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempTextureDesc);
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempTexture));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
  }
  renderViewContext.m_pRenderContext->SetViewportAndRenderTargetSetup(renderViewContext.m_pViewData->m_ViewPortRect, renderTargetSetup);


  // Constant buffer for both passes.
  renderViewContext.m_pRenderContext->BindConstantBuffer("ezLSAOConstants", m_hLineSweepCB);
  // Depth buffer for both passes.
  {
    ezGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle;
    ezGALResourceViewHandle hDepthInputView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", hDepthInputView);
  }


  // Line Sweep part (compute)
  {
    EZ_PROFILE_AND_MARKER(pGALContext, "Line Sweep");

    renderViewContext.m_pRenderContext->BindShader(m_hShaderLineSweep);
    renderViewContext.m_pRenderContext->BindBuffer("LineInstructions", m_hLineSweepInfoSRV);
    renderViewContext.m_pRenderContext->BindUAV("LineSweepOutputBuffer", m_hLineSweepOutputUAV);
    renderViewContext.m_pRenderContext->Dispatch(m_numSweepLines / SSAO_LINESWEEP_THREAD_GROUP + (m_numSweepLines % SSAO_LINESWEEP_THREAD_GROUP != 0 ? 1 : 0)).IgnoreResult();
  }

  // Gather samples.
  {
    EZ_PROFILE_AND_MARKER(pGALContext, "Gather");

    if (m_bDistributedGathering)
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DISTRIBUTED_SSAO_GATHERING", "TRUE");
    else
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DISTRIBUTED_SSAO_GATHERING", "FALSE");

    switch (m_DepthCompareFunction)
    {
      case ezLSAODepthCompareFunction::Depth:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_DEPTH");
        break;
      case ezLSAODepthCompareFunction::Normal:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL");
        break;
      case ezLSAODepthCompareFunction::NormalAndSampleDistance:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL_AND_SAMPLE_DISTANCE");
        break;
    }

    // Manually unbind UAV. TODO, this should be done automatically.
    pGALContext->SetUnorderedAccessView(0, ezGALUnorderedAccessViewHandle());

    renderViewContext.m_pRenderContext->BindShader(m_hShaderGather);

    renderViewContext.m_pRenderContext->BindBuffer("LineInstructions", m_hLineSweepInfoSRV);
    renderViewContext.m_pRenderContext->BindBuffer("LineSweepOutputBuffer", m_hLineSweepOutputSRV);

    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // If enabled, average distributed gather samples and write to output.
  if (m_bDistributedGathering)
  {
    EZ_PROFILE_AND_MARKER(pGALContext, "Averaging");

    switch (m_DepthCompareFunction)
    {
      case ezLSAODepthCompareFunction::Depth:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_DEPTH");
        break;
      case ezLSAODepthCompareFunction::Normal:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL");
        break;
      case ezLSAODepthCompareFunction::NormalAndSampleDistance:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL_AND_SAMPLE_DISTANCE");
        break;
    }

    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
    renderViewContext.m_pRenderContext->SetViewportAndRenderTargetSetup(renderViewContext.m_pViewData->m_ViewPortRect, renderTargetSetup);

    renderViewContext.m_pRenderContext->BindShader(m_hShaderAverage);

    {
      ezGALResourceViewCreationDescription rvcd;
      rvcd.m_hTexture = tempTexture;
      ezGALResourceViewHandle hTempTextureRView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);
      renderViewContext.m_pRenderContext->BindTexture2D("SSAOGatherOutput", hTempTextureRView);
    }

    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    // Give back temp texture.
    ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempTexture);
  }
#endif
}

void ezLSAOPass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor = ezColor::White;

  auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, "Clear");
}

void ezLSAOPass::SetLineToLinePixelOffset(ezUInt32 uiPixelOffset)
{
  m_uiLineToLinePixelOffset = uiPixelOffset;
  m_bSweepDataDirty = true;
}

void ezLSAOPass::SetLineSamplePixelOffset(ezUInt32 uiPixelOffset)
{
  m_uiLineSamplePixelOffsetFactor = uiPixelOffset;
  m_bSweepDataDirty = true;
}

float ezLSAOPass::GetDepthCutoffDistance() const
{
  ezLSAOConstants* cb = ezRenderContext::GetConstantBufferData<ezLSAOConstants>(m_hLineSweepCB);
  return cb->DepthCutoffDistance;
}

void ezLSAOPass::SetDepthCutoffDistance(float fDepthCutoffDistance)
{
  ezLSAOConstants* cb = ezRenderContext::GetConstantBufferData<ezLSAOConstants>(m_hLineSweepCB);
  cb->DepthCutoffDistance = fDepthCutoffDistance;
}

float ezLSAOPass::GetOcclusionFalloff() const
{
  ezLSAOConstants* cb = ezRenderContext::GetConstantBufferData<ezLSAOConstants>(m_hLineSweepCB);
  return cb->OcclusionFalloff;
}

void ezLSAOPass::SetOcclusionFalloff(float fFalloff)
{
  ezLSAOConstants* cb = ezRenderContext::GetConstantBufferData<ezLSAOConstants>(m_hLineSweepCB);
  cb->OcclusionFalloff = fFalloff;
}

void ezLSAOPass::DestroyLineSweepData()
{
  ezGALDevice* device = ezGALDevice::GetDefaultDevice();

  if (!m_hLineSweepOutputUAV.IsInvalidated())
    device->DestroyUnorderedAccessView(m_hLineSweepOutputUAV);
  m_hLineSweepOutputUAV.Invalidate();

  if (!m_hLineSweepOutputSRV.IsInvalidated())
    device->DestroyResourceView(m_hLineSweepOutputSRV);
  m_hLineSweepOutputSRV.Invalidate();

  if (!m_hLineSweepOutputBuffer.IsInvalidated())
    device->DestroyBuffer(m_hLineSweepOutputBuffer);
  m_hLineSweepOutputBuffer.Invalidate();

  if (!m_hLineInfoBuffer.IsInvalidated())
    device->DestroyBuffer(m_hLineInfoBuffer);
  m_hLineInfoBuffer.Invalidate();
}

void ezLSAOPass::SetupLineSweepData(const ezVec2I32& imageResolution)
{
  DestroyLineSweepData();

  ezDynamicArray<LineInstruction> lineInstructions;
  ezUInt32 totalNumberOfSamples = 0;
  ezLSAOConstants* cb = ezRenderContext::GetConstantBufferData<ezLSAOConstants>(m_hLineSweepCB);
  cb->LineToLinePixelOffset = m_uiLineToLinePixelOffset;

  // Compute general information per direction and create line instructions.

  // As long as we don't span out different line samplings across multiple frames, the number of prepared directions here is always equal to
  // the number of directions per frame. Note that if we were to do temporal sampling with a different line set every frame, we would need
  // to precompute all *possible* sampling directions still as a whole here!
  ezVec2I32 samplingDir[NUM_SWEEP_DIRECTIONS_PER_FRAME];
  {
    constexpr int numSweepDirs = NUM_SWEEP_DIRECTIONS_PER_FRAME;

    // As described in the paper, all directions are aligned so that we always hit  pixels on a square.
    static_assert(numSweepDirs % 4 == 0, "Invalid number of sweep directions for LSAO!");
    // static_assert((numSweepDirs * NUM_SWEEP_DIRECTIONS_PER_PIXEL) % 9 == 0, "Invalid number of sweep directions for LSAO!");
    const int perSide = (numSweepDirs + 4) / 4 - 1; // side length of the square on which all directions lie -1
    const int halfPerSide = perSide / 2 + (perSide % 2);
    for (int i = 0; i < perSide; ++i)
    {
      // Put opposing directions next to each other, so that a gather pass that doesn't sample all directions, only needs to sample an even
      // number of directions to end up with non-negative occlusion.
      samplingDir[i * 4 + 0] = ezVec2I32(i - halfPerSide, halfPerSide) * m_uiLineSamplePixelOffsetFactor; // Top
      samplingDir[i * 4 + 1] = -samplingDir[i * 4 + 0];                                                   // Bottom
      samplingDir[i * 4 + 2] = ezVec2I32(halfPerSide, halfPerSide - i) * m_uiLineSamplePixelOffsetFactor; // Right
      samplingDir[i * 4 + 3] = -samplingDir[i * 4 + 2];                                                   // Left
    }

    // todo: Ddd debug test to check whether any direction is duplicated. Mistakes in the equations above can easily happen!
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    for (int i = 0; i < numSweepDirs - 1; ++i)
    {
      for (int j = i + 1; j < numSweepDirs; ++j)
        EZ_ASSERT_DEBUG(samplingDir[i] != samplingDir[j], "Two SSAO sampling directions are equal. Implementation for direction determination is broken.");
    }
#  endif
  }

  for (int dirIndex = 0; dirIndex < EZ_ARRAY_SIZE(samplingDir); ++dirIndex)
  {
    ezUInt32 totalLineCountBefore = lineInstructions.GetCount();
    AddLinesForDirection(imageResolution, samplingDir[dirIndex], dirIndex, lineInstructions, totalNumberOfSamples);
    EZ_ASSERT_DEBUG(totalNumberOfSamples % 2 == 0, "Only even number of line samples are allowed");

    cb->Directions[dirIndex].Direction = ezVec2(static_cast<float>(samplingDir[dirIndex].x), static_cast<float>(samplingDir[dirIndex].y));
    cb->Directions[dirIndex].NumLines = lineInstructions.GetCount() - totalLineCountBefore;
    cb->Directions[dirIndex].LineInstructionOffset = totalLineCountBefore;
  }
  m_numSweepLines = lineInstructions.GetCount();
  cb->TotalLineNumber = m_numSweepLines;

  // Allocate and upload data structures to GPU
  {
    ezGALDevice* device = ezGALDevice::GetDefaultDevice();
    DestroyLineSweepData();

    // Output UAV for line sweep pass.
    // DX11 allows only float and int for writing RWBuffer, so we need to do manual packing.
    {
      ezGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = 4;
      bufferDesc.m_uiTotalSize = 2 * totalNumberOfSamples;
      bufferDesc.m_BufferType = ezGALBufferType::Generic;
      bufferDesc.m_bUseForIndirectArguments = false;
      bufferDesc.m_bUseAsStructuredBuffer = false;
      bufferDesc.m_bAllowRawViews = false;
      bufferDesc.m_bStreamOutputTarget = false;
      bufferDesc.m_bAllowShaderResourceView = true;
      bufferDesc.m_bAllowUAV = true;
      bufferDesc.m_ResourceAccess.m_bReadBack = false;
      bufferDesc.m_ResourceAccess.m_bImmutable = false;

      m_hLineSweepOutputBuffer = device->CreateBuffer(bufferDesc);

      ezGALUnorderedAccessViewCreationDescription uavDesc;
      uavDesc.m_hBuffer = m_hLineSweepOutputBuffer;
      uavDesc.m_OverrideViewFormat = ezGALResourceFormat::RUInt;
      uavDesc.m_uiFirstElement = 0;
      uavDesc.m_uiNumElements = totalNumberOfSamples / 2;
      uavDesc.m_bRawView = false;
      uavDesc.m_bAppend = false;
      m_hLineSweepOutputUAV = device->CreateUnorderedAccessView(uavDesc);

      ezGALResourceViewCreationDescription srvDesc;
      srvDesc.m_hBuffer = m_hLineSweepOutputBuffer;
      srvDesc.m_OverrideViewFormat = ezGALResourceFormat::RUInt;
      srvDesc.m_uiFirstElement = 0;
      srvDesc.m_uiNumElements = totalNumberOfSamples / 2;
      srvDesc.m_bRawView = false;
      m_hLineSweepOutputSRV = device->CreateResourceView(srvDesc);
    }

    // Structured buffer per line.
    {
      ezGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = sizeof(LineInstruction);
      bufferDesc.m_uiTotalSize = sizeof(LineInstruction) * m_numSweepLines;
      bufferDesc.m_BufferType = ezGALBufferType::Generic;
      bufferDesc.m_bUseForIndirectArguments = false;
      bufferDesc.m_bUseAsStructuredBuffer = true;
      bufferDesc.m_bAllowRawViews = false;
      bufferDesc.m_bStreamOutputTarget = false;
      bufferDesc.m_bAllowShaderResourceView = true;
      bufferDesc.m_bAllowUAV = false;
      bufferDesc.m_ResourceAccess.m_bReadBack = false;
      bufferDesc.m_ResourceAccess.m_bImmutable = true;

      m_hLineInfoBuffer = device->CreateBuffer(bufferDesc, ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(lineInstructions.GetData()), lineInstructions.GetCount() * sizeof(LineInstruction)));

      m_hLineSweepInfoSRV = device->GetDefaultResourceView(m_hLineInfoBuffer);
    }
  }

  m_bSweepDataDirty = false;
}

void ezLSAOPass::AddLinesForDirection(const ezVec2I32& imageResolution, const ezVec2I32& sampleDir, ezUInt32 lineIndex, ezDynamicArray<LineInstruction>& outinLineInstructions, ezUInt32& outinTotalNumberOfSamples)
{
  EZ_ASSERT_DEBUG(sampleDir.x != 0 || sampleDir.y != 0, "Sample direction is null (not pointing anywhere)");

  ezUInt32 firstNewLineInstructionIndex = outinLineInstructions.GetCount();

  // Always walk positive and flip if necessary later.
  ezVec2I32 walkDir(ezMath::Abs(sampleDir.x), ezMath::Abs(sampleDir.y));
  ezVec2 walkDirF(static_cast<float>(walkDir.x), static_cast<float>(walkDir.y));

  // Line "creation" always starts from 0,0 and walks along EITHER x or y depending which one is the less dominant axis.

  // Helper to avoid duplication for dominant x/y
  int domDir = walkDir.x > walkDir.y ? 0 : 1;
  int secDir = 1 - domDir;
#  define DOM GetData()[domDir]
#  define SEC GetData()[secDir]

  // Walk along secondary axis backwards.
  for (ezInt32 sec = imageResolution.SEC - 1; true; sec -= m_uiLineToLinePixelOffset)
  {
    LineInstruction& newLine = outinLineInstructions.ExpandAndGetRef();
    newLine.FirstSamplePos.DOM = 0.0f;
    newLine.FirstSamplePos.SEC = static_cast<float>(sec);

    // If we are already outside of the screen with sec, this is not a point inside the screen!
    if (sec < 0)
    {
      // If we don't walk in the secondary direction at all this means that we're done.
      if (walkDir.SEC == 0)
      {
        outinLineInstructions.PopBack();
        break;
      }
      // Otherwise we just need to walk long enough to hit the screen again.
      else
      {
        // Find new start on the sec axis. (dom axis is fine)
        ezVec2 minimalStepToBorder = walkDirF * ezMath::Ceil(static_cast<float>(-sec) / walkDirF.SEC); // Remember: Only walk discrete steps!
        newLine.FirstSamplePos.DOM += minimalStepToBorder.DOM;
        newLine.FirstSamplePos.SEC += minimalStepToBorder.SEC;

        // Outside, we're done.
        if (newLine.FirstSamplePos.DOM >= imageResolution.DOM - walkDir.DOM * 2)
        {
          outinLineInstructions.PopBack();
          break;
        }
      }
    }

    // Add a pseudo random offset to distributed the samples a bit.
    // We still want to go from discrete pixel to discrete pixel so we have to round which can mess up our line placement.
    // So this is introducing some error. Visual comparision clearly shows that it's worth it though.
    float offset = HaltonSequence(lineIndex, sec + lineIndex);
    newLine.FirstSamplePos.DOM += ezMath::Round(offset * walkDir.DOM);
    newLine.FirstSamplePos.SEC += ezMath::Round(offset * walkDir.SEC);

    // Clamp back to possible area.
    // Due to the way we jump from pixels to line in the gather shader, we can't just discard lines.
    newLine.FirstSamplePos.x = ezMath::Clamp<float>(newLine.FirstSamplePos.x, 0.0f, imageResolution.x - 1.0f);
    newLine.FirstSamplePos.y = ezMath::Clamp<float>(newLine.FirstSamplePos.y, 0.0f, imageResolution.y - 1.0f);

    // Compute how many samples this line will consume.
    unsigned int stepsToDOMBorder = static_cast<unsigned int>((imageResolution.DOM - newLine.FirstSamplePos.DOM) / walkDir.DOM + 1);
    unsigned int numSamples = 0;
    if (walkDir.SEC > 0)
    {
      unsigned int stepsToSECBorder = static_cast<unsigned int>((imageResolution.SEC - newLine.FirstSamplePos.SEC) / walkDir.SEC + 1);
      numSamples = ezMath::Min(stepsToSECBorder, stepsToDOMBorder);
    }
    else
      numSamples = stepsToDOMBorder;

    // Due to output packing restrictions only even number of samples are allowed. Remove one if necessary.
    if (numSamples % 2 != 0)
      --numSamples;

    newLine.LineSweepOutputBufferOffset = outinTotalNumberOfSamples;
    outinTotalNumberOfSamples += numSamples;
    newLine.LineDirIndex_NumSamples = lineIndex | (numSamples << 16);
  }

#  undef SEC
#  undef DOM

  // Now consider x/y being negative.
  for (int c = 0; c < 2; ++c)
  {
    if (sampleDir.GetData()[c] < 0)
    {
      for (ezUInt32 i = firstNewLineInstructionIndex; i < outinLineInstructions.GetCount(); ++i)
      {
        outinLineInstructions[i].FirstSamplePos.GetData()[c] = imageResolution.GetData()[c] - 1 - outinLineInstructions[i].FirstSamplePos.GetData()[c];
      }
    }
  }

  // Validation.
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  for (ezUInt32 i = firstNewLineInstructionIndex; i < outinLineInstructions.GetCount(); ++i)
  {
    auto p = outinLineInstructions[i].FirstSamplePos;
    EZ_ASSERT_DEV(p.x >= 0 && p.y >= 0 && p.x < imageResolution.x && p.y < imageResolution.y, "First sweep line sample pos is invalid. Something is wrong with the sweep line generation algorithm.");
  }
#  endif
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LSAOPass);
