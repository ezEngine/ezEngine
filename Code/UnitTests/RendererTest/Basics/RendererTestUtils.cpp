#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/Basics/RendererTestUtils.h>

ezTransform ezRendererTestUtils::CreateTransform(const ezUInt32 uiColumns, const ezUInt32 uiRows, ezUInt32 x, ezUInt32 y)
{
  ezTransform t = ezTransform::MakeIdentity();
  t.m_vScale = ezVec3(1.0f / float(uiColumns), 1.0f / float(uiRows), 1);
  t.m_vPosition = ezVec3(ezMath::Lerp(-1.f, 1.f, (float(x) + 0.5f) / float(uiColumns)), ezMath::Lerp(1.f, -1.f, (float(y) + 0.5f) / float(uiRows)), 0);
  if (ezClipSpaceYMode::RenderToTextureDefault == ezClipSpaceYMode::Flipped)
  {
    ezTransform flipY = ezTransform::MakeIdentity();
    flipY.m_vScale.y *= -1.0f;
    t = flipY * t;
  }
  return t;
}
void ezRendererTestUtils::FillStructuredBuffer(ezHybridArray<ezTestShaderData, 16>& ref_instanceData, ezUInt32 uiColorOffset, ezUInt32 uiSlotOffset)
{
  ref_instanceData.SetCount(16);
  const ezUInt32 uiColumns = 4;
  const ezUInt32 uiRows = 2;

  for (ezUInt32 x = 0; x < uiColumns; ++x)
  {
    for (ezUInt32 y = 0; y < uiRows; ++y)
    {
      ezTestShaderData& instance = ref_instanceData[uiSlotOffset + x * uiRows + y];
      const float fColorIndex = float(uiColorOffset + x * uiRows + y) / 32.0f;
      instance.InstanceColor = ezColorScheme::LightUI(fColorIndex).GetAsVec4();
      ezTransform t = CreateTransform(uiColumns, uiRows, x, y);
      instance.InstanceTransform = t;
    }
  }
}
void ezRendererTestUtils::CreateImage(ezImage& ref_image, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiMipLevelCount, bool bMipLevelIsBlue, ezUInt8 uiFixedBlue)
{
  ezImageHeader header;
  header.SetImageFormat(ezImageFormat::B8G8R8A8_UNORM_SRGB);
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetNumMipLevels(uiMipLevelCount);

  ref_image.ResetAndAlloc(header);
  for (ezUInt32 m = 0; m < uiMipLevelCount; m++)
  {
    const ezUInt32 uiHeight = ref_image.GetHeight(m);
    const ezUInt32 uiWidth = ref_image.GetWidth(m);

    const ezUInt8 uiBlue = bMipLevelIsBlue ? static_cast<ezUInt8>(255.0f * float(m) / (uiMipLevelCount - 1)) : uiFixedBlue;
    for (ezUInt32 y = 0; y < uiHeight; y++)
    {
      const ezUInt8 uiGreen = static_cast<ezUInt8>(255.0f * float(y) / (uiHeight - 1));
      for (ezUInt32 x = 0; x < uiWidth; x++)
      {
        ImgColor* pColor = ref_image.GetPixelPointer<ImgColor>(m, 0u, 0u, x, y);
        pColor->a = 255;
        pColor->b = uiBlue;
        pColor->g = uiGreen;
        pColor->r = static_cast<ezUInt8>(255.0f * float(x) / (uiWidth - 1));
      }
    }
  }
}
