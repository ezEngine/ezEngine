
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
    case ezGALShaderResourceType::TexelBuffer:
    case ezGALShaderResourceType::StructuredBuffer:
      return ezGALShaderResourceCategory::SRV;
    case ezGALShaderResourceType::TextureRW:
    case ezGALShaderResourceType::TexelBufferRW:
    case ezGALShaderResourceType::StructuredBufferRW:
      return ezGALShaderResourceCategory::UAV;
    case ezGALShaderResourceType::TextureAndSampler:
      return ezGALShaderResourceCategory::SRV | ezGALShaderResourceCategory::Sampler;
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