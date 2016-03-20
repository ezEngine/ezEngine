#include <RendererCore/PCH.h>
//#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/BlurPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>


#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <CoreUtils/Geometry/GeomUtils.h>
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlurPass, 1, ezRTTIDefaultAllocator<ezBlurPass>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Input", m_PinInput),
EZ_MEMBER_PROPERTY("Output", m_PinOutput)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezBlurPass::ezBlurPass() : ezRenderPipelinePass("BlurPass")
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Blur.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");

  // TODO: Move my lovely quad somewhere else.
  const char* szResourceName = "BlurFullscreenQuad";
  m_hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);
  if (m_hMesh.IsValid())
    return;

  // Build geometry
  ezGeometry geom;

  const ezVec2 halfSize = ezVec2(2.0f, 2.0f) * 0.5f;

  ezUInt32 idx[4];
  ezMat4 mTransform = ezMat4::IdentityMatrix();

  // I just fiddled around with the UV until it was the right way around on the screen...
  //geom.AddRectXY(ezVec2(2.0f, 2.0f), ezColor::CornflowerBlue);
  idx[0] = geom.AddVertex(ezVec3(-halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 1), ezColor::CornflowerBlue, 0, mTransform);
  idx[1] = geom.AddVertex(ezVec3(halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 1), ezColor::CornflowerBlue, 0, mTransform);
  idx[2] = geom.AddVertex(ezVec3(halfSize.x, halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 0), ezColor::CornflowerBlue, 0, mTransform);
  idx[3] = geom.AddVertex(ezVec3(-halfSize.x, halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 0), ezColor::CornflowerBlue, 0, mTransform);
  geom.AddPolygon(idx);

  // Convert to mesh resource. Copied from somewhere in the tests.
  ezDynamicArray<ezUInt16> Indices;
  ezGALPrimitiveTopology::Enum Topology = ezGALPrimitiveTopology::Triangles;

  Indices.Reserve(geom.GetPolygons().GetCount() * 6);

  for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
  {
    for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
    {
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
    }
  }
  
  ezMeshBufferResourceDescriptor desc;
  desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);

  desc.AllocateStreams(geom.GetVertices().GetCount(), Topology, Indices.GetCount() / (Topology + 1));

  for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
  {
    desc.SetVertexData<ezVec3>(0, v, geom.GetVertices()[v].m_vPosition);
    desc.SetVertexData<ezVec2>(1, v, geom.GetVertices()[v].m_vTexCoord);
  }

  for (ezUInt32 t = 0; t < Indices.GetCount(); t += 3)
  {
    desc.SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
  }
  
  m_hMesh = ezResourceManager::CreateResource<ezMeshBufferResource>(szResourceName, desc, szResourceName);
}

ezBlurPass::~ezBlurPass()
{
}

bool ezBlurPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTagetSetup& setup = view.GetRenderTargetSetup();

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

    // Setup render target
    ezGALRenderTagetSetup renderTargetSetup;
    ezGALRenderTargetViewCreationDescription rtvd;
    rtvd.m_hTexture = outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::Color;
    renderTargetSetup.SetRenderTarget(0, pDevice->CreateRenderTargetView(rtvd));

    // Bind render target and viewport
    ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();
    const ezRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
    pGALContext->SetViewport(viewPortRect.x, viewPortRect.y, viewPortRect.width, viewPortRect.height, 0.0f, 1.0f);
    pGALContext->SetRenderTargetSetup(renderTargetSetup);

    // Setup input view and sampler
    ezGALSamplerStateCreationDescription sscd;
    sscd.m_MagFilter = ezGALTextureFilterMode::Linear;
    sscd.m_MinFilter = ezGALTextureFilterMode::Point;
    sscd.m_MipFilter = ezGALTextureFilterMode::Point;
    sscd.m_AddressU = ezGALTextureAddressMode::Clamp;
    sscd.m_AddressV = ezGALTextureAddressMode::Clamp;
    ezGALSamplerStateHandle hSamplerState = pDevice->CreateSamplerState(sscd);

    ezGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinInput.m_uiInputIndex]->m_TextureHandle;
    ezGALResourceViewHandle hResourceView = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(m_hMesh);
   
    // TODO: how to upload constant buffer stuffy?

    // TODO: renderViewContext.m_pRenderContext->BindTexture() needs a texture resource handle that I don't have.
    pGALContext->SetResourceView(ezGALShaderStage::PixelShader, 0, hResourceView);
    pGALContext->SetSamplerState(ezGALShaderStage::PixelShader, 0, hSamplerState);

    // TODO: This spams erros because I use pGALContext to bind the texture, don't have another choice though :-/
    renderViewContext.m_pRenderContext->DrawMeshBuffer();
  }
}
