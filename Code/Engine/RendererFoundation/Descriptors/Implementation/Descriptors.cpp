#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

ezUInt32 ezGALBindGroupLayoutCreationDescription::CalculateHash() const
{
  ezHashStreamWriter32 writer;
  for (const ezShaderResourceBinding& binding : m_ResourceBindings)
  {
    writer << binding.m_ResourceType.GetValue();
    writer << binding.m_TextureType.GetValue();
    writer << binding.m_Stages.GetValue();
    writer << binding.m_iSet;
    writer << binding.m_iSlot;
    writer << binding.m_uiArraySize;
    writer << binding.m_sName;
    if (binding.m_pLayout != nullptr)
    {
      const ezShaderConstantBufferLayout* pLayout = binding.m_pLayout;
      writer << pLayout->m_uiTotalSize;
      for (const ezShaderConstant& constant : pLayout->m_Constants)
      {
        writer << constant.m_sName;
        writer << constant.m_Type.GetValue();
        writer << constant.m_uiArrayElements;
        writer << constant.m_uiOffset;
      }
    }
  }
  return writer.GetHashValue();
}
