
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
