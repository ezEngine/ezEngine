#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererGL/Basics.h>

#include <Foundation/Containers/DynamicArray.h>

struct ID3D11InputLayout;

class ezGALVertexDeclarationGL : public ezGALVertexDeclaration
{
public:

  struct VertexAttributeDesc
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 uiChannelCount;
    ezUInt16 uiStride;
    ezUInt16 uiBufferIndex;
    ezUInt16 uiOffset;

    bool bNormalized;
    ezUInt8 uiDivisor;

    ezUInt32 uiFormat;
  };

  const ezDynamicArray<VertexAttributeDesc>& GetGLAttributeDesc();

protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALVertexDeclarationGL(const ezGALVertexDeclarationCreationDescription& Description);

  virtual ~ezGALVertexDeclarationGL();

  ezDynamicArray<VertexAttributeDesc> m_AttributesGL;
};

#include <RendererGL/Shader/Implementation/VertexDeclarationGL_inl.h>
