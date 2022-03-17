#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>

#include <d3d11.h>

ezGALVertexDeclarationVulkan::ezGALVertexDeclarationVulkan(const ezGALVertexDeclarationCreationDescription& Description)
  : ezGALVertexDeclaration(Description)
{
}

ezGALVertexDeclarationVulkan::~ezGALVertexDeclarationVulkan() = default;

static const char* GALSemanticToVulkan[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD",
  "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES", "BONEINDICES", "BONEWEIGHTS",
  "BONEWEIGHTS"};

static UINT GALSemanticToIndexVulkan[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 1, 0, 1};

EZ_CHECK_AT_COMPILETIME_MSG(EZ_ARRAY_SIZE(GALSemanticToVulkan) == ezGALVertexAttributeSemantic::ENUM_COUNT,
  "GALSemanticToVulkan array size does not match vertex attribute semantic count");
EZ_CHECK_AT_COMPILETIME_MSG(EZ_ARRAY_SIZE(GALSemanticToIndexVulkan) == ezGALVertexAttributeSemantic::ENUM_COUNT,
  "GALSemanticToIndexVulkan array size does not match vertex attribute semantic count");

EZ_DEFINE_AS_POD_TYPE(D3D11_INPUT_ELEMENT_DESC);

ezResult ezGALVertexDeclarationVulkan::InitPlatform(ezGALDevice* pDevice)
{
  // TODO
#if 0
  ezHybridArray<D3D11_INPUT_ELEMENT_DESC, 8> DXInputElementDescs;

  ezGALDeviceVulkan* pDXDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  const ezGALShader* pShader = pDevice->GetShader(m_Description.m_hShader);

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(ezGALShaderStage::VertexShader))
  {
    return EZ_FAILURE;
  }

  // Copy attribute descriptions
  for (ezUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const ezGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    D3D11_INPUT_ELEMENT_DESC DXDesc;
    DXDesc.AlignedByteOffset = Current.m_uiOffset;
    DXDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;

    if (DXDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
      ezLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, i);
      return EZ_FAILURE;
    }

    DXDesc.InputSlot = Current.m_uiVertexBufferSlot;
    DXDesc.InputSlotClass = Current.m_bInstanceData ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
    DXDesc.InstanceDataStepRate = Current.m_bInstanceData ? 1 : 0; /// \todo Expose step rate?
    DXDesc.SemanticIndex = GALSemanticToIndexVulkan[Current.m_eSemantic];
    DXDesc.SemanticName = GALSemanticToVulkan[Current.m_eSemantic];

    DXInputElementDescs.PushBack(DXDesc);
  }


  const ezScopedRefPointer<ezGALShaderByteCode>& pByteCode = pShader->GetDescription().m_ByteCodes[ezGALShaderStage::VertexShader];

  if (FAILED(pDXDevice->GetDXDevice()->CreateInputLayout(
        &DXInputElementDescs[0], DXInputElementDescs.GetCount(), pByteCode->GetByteCode(), pByteCode->GetSize(), &m_pDXInputLayout)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
#endif

  return EZ_SUCCESS;
}

ezResult ezGALVertexDeclarationVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  // TODO
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_VertexDeclarationVulkan);
