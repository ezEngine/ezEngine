#pragma once

#include <RendererFoundation/Resources/Texture.h>

struct ID3D11Resource;
struct D3D11_TEXTURE2D_DESC;
struct D3D11_TEXTURE3D_DESC;
struct D3D11_SUBRESOURCE_DATA;
class ezGALDeviceDX11;

EZ_DEFINE_AS_POD_TYPE(D3D11_SUBRESOURCE_DATA);

class ezGALTextureDX11 : public ezGALTexture
{
public:
  static ezResult Create2DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX11* pDXDevice, D3D11_TEXTURE2D_DESC& out_tex2DDesc);
  static ezResult Create3DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX11* pDXDevice, D3D11_TEXTURE3D_DESC& out_tex3DDesc);
  static void ConvertInitialData(const ezGALTextureCreationDescription& description, ezArrayPtr<ezGALSystemMemoryDescription> initialData, ezHybridArray<D3D11_SUBRESOURCE_DATA, 16>& out_initialData);

public:
  EZ_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const;
  ID3D11ShaderResourceView* GetSRV(ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat) const;
  ID3D11UnorderedAccessView* GetUAV(ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat) const;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;
  friend class ezGALSharedTextureDX11;

  ezGALTextureDX11(const ezGALTextureCreationDescription& Description);
  ~ezGALTextureDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> initialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezResult InitFromNativeObject(ezGALDeviceDX11* pDXDevice);

protected:
  ezGALDeviceDX11* m_pDevice = nullptr;
  ID3D11Resource* m_pDXTexture = nullptr;

  struct View : ezHashableStruct<View>
  {
    ezGALTextureRange m_TextureRange;
    ezEnum<ezGALResourceFormat> m_OverrideViewFormat;

    EZ_ALWAYS_INLINE static ezUInt32 Hash(const View& value) { return value.CalculateHash(); }
    EZ_ALWAYS_INLINE static bool Equal(const View& a, const View& b) { return a == b; }
  };
  mutable ezHashTable<View, ID3D11ShaderResourceView*, View> m_SRVs;
  mutable ezHashTable<View, ID3D11UnorderedAccessView*, View> m_UAVs;
};



#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>
