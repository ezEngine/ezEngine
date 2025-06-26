
inline ezBitflags<ezGALShaderResourceCategory> ezGALShaderResourceCategory::MakeFromShaderDescriptorType(ezGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case ezGALShaderResourceType::Sampler:
      return ezGALShaderResourceCategory::Sampler;
    case ezGALShaderResourceType::ConstantBuffer:
    case ezGALShaderResourceType::PushConstants:
      return ezGALShaderResourceCategory::ConstantBuffer;
    case ezGALShaderResourceType::Texture:
      return ezGALShaderResourceCategory::TextureSRV;
    case ezGALShaderResourceType::TexelBuffer:
    case ezGALShaderResourceType::StructuredBuffer:
    case ezGALShaderResourceType::ByteAddressBuffer:
      return ezGALShaderResourceCategory::BufferSRV;
    case ezGALShaderResourceType::TextureRW:
      return ezGALShaderResourceCategory::TextureUAV;
    case ezGALShaderResourceType::TexelBufferRW:
    case ezGALShaderResourceType::StructuredBufferRW:
    case ezGALShaderResourceType::ByteAddressBufferRW:
      return ezGALShaderResourceCategory::BufferUAV;
    case ezGALShaderResourceType::TextureAndSampler:
      return ezGALShaderResourceCategory::TextureSRV | ezGALShaderResourceCategory::Sampler;
    default:
      EZ_REPORT_FAILURE("Missing enum");
      return {};
  }
}

inline bool ezGALShaderTextureType::IsArray(ezGALShaderTextureType::Enum format)
{
  switch (format)
  {
    case ezGALShaderTextureType::Texture1DArray:
    case ezGALShaderTextureType::Texture2DArray:
    case ezGALShaderTextureType::Texture2DMSArray:
    case ezGALShaderTextureType::TextureCubeArray:
      return true;
    default:
      return false;
  }
}

inline bool ezGALShaderTextureType::IsMSAA(ezGALShaderTextureType::Enum format)
{
  switch (format)
  {
    case ezGALShaderTextureType::Texture2DMS:
    case ezGALShaderTextureType::Texture2DMSArray:
      return true;
    default:
      return false;
  }
}

inline ezGALTextureType::Enum ezGALShaderTextureType::GetTextureType(ezGALShaderTextureType::Enum format)
{
  switch (format)
  {
    case ezGALShaderTextureType::Texture2D:
    case ezGALShaderTextureType::Texture2DMS:
      return ezGALTextureType::Texture2D;
    case ezGALShaderTextureType::Texture2DArray:
    case ezGALShaderTextureType::Texture2DMSArray:
      return ezGALTextureType::Texture2DArray;
    case ezGALShaderTextureType::Texture3D:
      return ezGALTextureType::Texture3D;
    case ezGALShaderTextureType::TextureCube:
      return ezGALTextureType::TextureCube;
    case ezGALShaderTextureType::TextureCubeArray:
      return ezGALTextureType::TextureCubeArray;
    case ezGALShaderTextureType::Unknown:
    case ezGALShaderTextureType::Texture1D:
    case ezGALShaderTextureType::Texture1DArray:
    default:
      EZ_REPORT_FAILURE("Unknown shader texture type");
      return ezGALTextureType::Invalid;
  }
}
