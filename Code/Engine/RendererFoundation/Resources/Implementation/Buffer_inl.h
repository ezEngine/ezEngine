

ezUInt32 ezGALBuffer::GetSize() const
{
  return m_Description.m_uiTotalSize;
}

ezGALBufferRange ezGALBuffer::ClampRange(ezGALBufferRange range) const
{
  const ezUInt32 uiBufferSize = GetDescription().m_uiTotalSize;
  EZ_ASSERT_DEBUG(range.m_uiByteOffset < uiBufferSize, "Invalid ezGALBufferRange: Buffer offset {} is out of bounds of the buffer size {}", range.m_uiByteOffset, uiBufferSize);
  if (range.m_uiByteCount == EZ_GAL_WHOLE_SIZE)
  {
    range.m_uiByteCount = uiBufferSize - range.m_uiByteOffset;
  }
  EZ_ASSERT_DEBUG(range.m_uiByteOffset + range.m_uiByteCount <= uiBufferSize, "Invalid ezGALBufferRange: Buffer offset {} + byte count {} is bigger than buffer size {}", range.m_uiByteOffset, range.m_uiByteCount, uiBufferSize);
  return range;
}
