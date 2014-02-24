
#pragma once

#include <RendererFoundation/Basics.h>
#include <Foundation/Basics/Types/RefCounted.h>

class EZ_RENDERERFOUNDATION_DLL ezGALShaderByteCode : public ezRefCounted
{
public:

  ezGALShaderByteCode();

  ezGALShaderByteCode(const ezArrayPtr<ezUInt8>& pByteCode);

  ezGALShaderByteCode(void* pSource, ezUInt32 uiSize);

  ~ezGALShaderByteCode();

  inline const void* GetByteCode() const;

  inline ezUInt32 GetSize() const;

  inline bool IsValid() const;

protected:

  void CopyFrom(const ezArrayPtr<ezUInt8>& pByteCode);

  void Free();

  ezArrayPtr<ezUInt8> m_pSource;
};

#include <RendererFoundation/Shader/Implementation/ShaderBytecode_inl.h>