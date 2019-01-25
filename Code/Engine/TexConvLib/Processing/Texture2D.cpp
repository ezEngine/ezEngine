#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

ezResult ezTexConvProcessor::Assemble2DTexture()
{
  m_ScratchImage.ResetAndAlloc(m_Descriptor.m_InputImages[0].GetHeader());

  ezHybridArray<const ezColor*, 16> pSource;
  for (ezUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
  {
    pSource.ExpandAndGetRef() = m_Descriptor.m_InputImages[i].GetPixelPointer<ezColor>();
  }

  ezColor* pPixelOut = m_ScratchImage.GetPixelPointer<ezColor>();

  for (ezUInt32 y = 0; y < m_uiTargetResolutionY; ++y)
  {
    for (ezUInt32 x = 0; x < m_uiTargetResolutionX; ++x)
    {
      const ezUInt32 pixelOffset = (y * m_uiTargetResolutionX) + x;
      ezColor* pThisPixelOut = pPixelOut + pixelOffset;

      for (ezUInt32 channel = 0; channel < 4; ++channel)
      {
        float fValue;

        const auto& cm = m_Descriptor.m_Texture2DChannelMapping[channel];
        const ezInt32 inputIndex = cm.m_iInputImageIndex;
        if (inputIndex != -1)
        {
          const ezColor* pSourcePixel = pSource[inputIndex] + pixelOffset;

          switch (cm.m_ChannelValue)
          {
            case ezTexConvChannelValue::Red:
              fValue = pSourcePixel->r;
              break;
            case ezTexConvChannelValue::Green:
              fValue = pSourcePixel->g;
              break;
            case ezTexConvChannelValue::Blue:
              fValue = pSourcePixel->b;
              break;
            case ezTexConvChannelValue::Alpha:
              fValue = pSourcePixel->a;
              break;
            case ezTexConvChannelValue::Black:
              fValue = 0.0f;
              break;
            case ezTexConvChannelValue::White:
              fValue = 1.0f;
              break;
          }
        }

        pThisPixelOut->GetData()[channel] = fValue;
      }
    }
  }

  return EZ_SUCCESS;
}
