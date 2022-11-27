ezInt32 ezImageFilterWeights::GetFirstSourceSampleIndex(ezUInt32 uiDstSampleIndex) const
{
  ezSimdFloat dstSampleInSourceSpace = (ezSimdFloat(uiDstSampleIndex) + ezSimdFloat(0.5f)) * m_fDestToSourceScale;

  return ezInt32(ezMath::Floor(dstSampleInSourceSpace - m_fWidthInSourceSpace));
}

inline ezArrayPtr<const float> ezImageFilterWeights::ViewWeights() const
{
  return m_Weights;
}
