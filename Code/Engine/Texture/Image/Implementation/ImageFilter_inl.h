ezInt32 ezImageFilterWeights::GetFirstSourceSampleIndex(ezUInt32 dstSampleIndex) const
{
  ezSimdFloat dstSampleInSourceSpace = (ezSimdFloat(dstSampleIndex) + ezSimdFloat(0.5f)) * m_fDestToSourceScale;

  return ezInt32(ezMath::Floor(dstSampleInSourceSpace - m_fWidthInSourceSpace));
}

inline ezArrayPtr<const float> ezImageFilterWeights::ViewWeights() const
{
  return m_Weights;
}
