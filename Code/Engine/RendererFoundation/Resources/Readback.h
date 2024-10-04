#pragma once

#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/Buffer.h>

class ezGALDevice;

class EZ_RENDERERFOUNDATION_DLL ezGALReadback
{
public:
  EZ_FORCE_INLINE ezGALFenceHandle GetCurrentFence() const { return m_hFence; }
  [[nodiscard]] ezEnum<ezGALAsyncResult> GetReadbackResult(ezTime timeout) const;

protected:
  ezGALDevice* m_pDevice = nullptr;
  ezGALFenceHandle m_hFence = 0;
};

class EZ_RENDERERFOUNDATION_DLL ezGALReadbackBuffer : public ezGALReadback
{
public:
  ~ezGALReadbackBuffer();
  ezGALFenceHandle ReadbackBuffer(ezGALCommandEncoder& encoder, ezGALBufferHandle hBuffer);
  ezResult LockBuffer(ezArrayPtr<const ezUInt8>& out_Memory);
  ezResult UnlockBuffer();

private:
  ezGALBufferHandle m_hReadbackBuffer;
};

class  EZ_RENDERERFOUNDATION_DLL ezGALReadbackTexture : public ezGALReadback
{
public:
  ~ezGALReadbackTexture();
  ezGALFenceHandle ReadbackTexture(ezGALCommandEncoder& encoder, ezGALTextureHandle hTexture);
  ezResult LockTexture(const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_Memory);
  ezResult UnlockTexture(const ezArrayPtr<const ezGALTextureSubresource>& subResources);

private:
  ezGALTextureHandle m_hReadbackTexture;
};



