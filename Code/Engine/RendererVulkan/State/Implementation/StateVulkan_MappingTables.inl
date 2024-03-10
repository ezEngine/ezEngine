
static vk::CullModeFlagBits GALCullModeToVulkan[ezGALCullMode::ENUM_COUNT] =
  {
    vk::CullModeFlagBits::eNone,
    vk::CullModeFlagBits::eFront,
    vk::CullModeFlagBits::eBack};

static const vk::SamplerAddressMode GALTextureAddressModeToVulkan[ezImageAddressMode::ENUM_COUNT] =
  {
    vk::SamplerAddressMode::eRepeat,
    vk::SamplerAddressMode::eClampToEdge,
    vk::SamplerAddressMode::eClampToBorder,
    vk::SamplerAddressMode::eMirroredRepeat,
};

static const vk::CompareOp GALCompareFuncToVulkan[ezGALCompareFunc::ENUM_COUNT] =
  {
    vk::CompareOp::eNever,
    vk::CompareOp::eLess,
    vk::CompareOp::eEqual,
    vk::CompareOp::eLessOrEqual,
    vk::CompareOp::eGreater,
    vk::CompareOp::eNotEqual,
    vk::CompareOp::eGreaterOrEqual,
    vk::CompareOp::eAlways};

static const vk::Filter GALFilterToVulkanFilter[3] =
  {
    vk::Filter::eNearest,
    vk::Filter::eLinear,
    vk::Filter::eLinear};

static const vk::SamplerMipmapMode GALFilterToVulkanMipmapMode[3] =
  {
    vk::SamplerMipmapMode::eNearest,
    vk::SamplerMipmapMode::eLinear,
    vk::SamplerMipmapMode::eLinear};

// TODO this isn't available directly in vulkan
#if 0
static const D3D11_FILTER GALFilterTableIndexToVulkan[16] =
{
  D3D11_FILTER_MIN_MAG_MIP_POINT,
  D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
  D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
  D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
  D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
  D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
  D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
  D3D11_FILTER_MIN_MAG_MIP_LINEAR,
  D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
  D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
  D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
  D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
  D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
  D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
  D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
  D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR
};
#endif

static const vk::StencilOp GALStencilOpTableIndexToVulkan[8] =
  {
    vk::StencilOp::eKeep,
    vk::StencilOp::eZero,
    vk::StencilOp::eReplace,
    vk::StencilOp::eIncrementAndClamp,
    vk::StencilOp::eDecrementAndClamp,
    vk::StencilOp::eInvert,
    vk::StencilOp::eIncrementAndWrap,
    vk::StencilOp::eDecrementAndWrap};
