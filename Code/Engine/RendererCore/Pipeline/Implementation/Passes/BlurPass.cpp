#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Passes/BlurPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <CoreUtils/Geometry/GeomUtils.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlurConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlurPass, 1, ezRTTIDefaultAllocator<ezBlurPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(15)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezBlurPass::ezBlurPass() : ezRenderPipelinePass("BlurPass"), m_iRadius(15)
{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Blur.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBlurCB = ezRenderContext::CreateConstantBufferStorage<BlurConstants>();
  }

  {
    // TODO: Move my lovely quad somewhere else.
    const char* szResourceName = "BlurFullscreenQuad";
    m_hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);
    if (m_hMesh.IsValid())
      return;

    // Build geometry
    ezGeometry geom;

    ezUInt32 idx[4];
    const ezVec2 halfSize = ezVec2(1.0f, 1.0f);
    ezMat4 mTransform = ezMat4::IdentityMatrix();
    idx[0] = geom.AddVertex(ezVec3(-halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 1), ezColor::CornflowerBlue, 0, mTransform);
    idx[1] = geom.AddVertex(ezVec3(halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 1), ezColor::CornflowerBlue, 0, mTransform);
    idx[2] = geom.AddVertex(ezVec3(halfSize.x, halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 0), ezColor::CornflowerBlue, 0, mTransform);
    idx[3] = geom.AddVertex(ezVec3(-halfSize.x, halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 0), ezColor::CornflowerBlue, 0, mTransform);
    geom.AddPolygon(idx, false);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    m_hMesh = ezResourceManager::CreateResource<ezMeshBufferResource>(szResourceName, desc, szResourceName);
  }
}

ezBlurPass::~ezBlurPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hBlurCB);
  m_hBlurCB.Invalidate();
}

bool ezBlurPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinInput.m_uiInputIndex];
  }
  else
  {
    ezLog::Error("No input connected to blur pass!");
    return false;
  }

  return true;
}

void ezBlurPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

    // Setup render target
    ezGALRenderTagetSetup renderTargetSetup;
    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
        
    // Bind render target and viewport
    pGALContext->SetRenderTargetSetup(renderTargetSetup);
    pGALContext->SetViewport(renderViewContext.m_pViewData->m_ViewPortRect);
   
    pGALContext->Clear(ezColor(1.0f, 0.0f, 0.0f));
    // Setup input view and sampler
    ezGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinInput.m_uiInputIndex]->m_TextureHandle;
    ezGALResourceViewHandle hResourceView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(m_hMesh);  
    renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "Input", hResourceView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("BlurConstants", m_hBlurCB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer();
  }
}

void ezBlurPass::SetRadius(ezInt32 iRadius)
{
  m_iRadius = iRadius;
  
  BlurConstants* cb = ezRenderContext::GetConstantBufferData<BlurConstants>(m_hBlurCB);
  cb->BlurRadius = m_iRadius;
}

ezInt32 ezBlurPass::GetRadius() const
{
  return m_iRadius;
}
