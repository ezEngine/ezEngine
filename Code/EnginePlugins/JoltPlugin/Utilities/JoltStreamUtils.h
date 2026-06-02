#pragma once

#include <Foundation/IO/Stream.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Core/StreamOut.h>
#include <Jolt/Jolt.h>

/// Adapts ezStreamReader to JPH::StreamIn.
class ezJoltStreamIn : public JPH::StreamIn
{
public:
  explicit ezJoltStreamIn(ezStreamReader* pReader)
    : m_pReader(pReader)
  {
  }

  virtual void ReadBytes(void* pData, size_t uiNumBytes) override
  {
    if (m_pReader->ReadBytes(pData, uiNumBytes) < uiNumBytes)
      m_bEOF = true;
  }

  virtual bool IsEOF() const override { return m_bEOF; }
  virtual bool IsFailed() const override { return false; }

private:
  ezStreamReader* m_pReader = nullptr;
  bool m_bEOF = false;
};

/// Adapts ezStreamWriter to JPH::StreamOut.
class ezJoltStreamOut : public JPH::StreamOut
{
public:
  explicit ezJoltStreamOut(ezStreamWriter* pWriter)
    : m_pWriter(pWriter)
  {
  }

  virtual void WriteBytes(const void* pData, size_t uiNumBytes) override
  {
    if (m_pWriter->WriteBytes(pData, uiNumBytes).Failed())
      m_bFailed = true;
  }

  virtual bool IsFailed() const override { return m_bFailed; }

private:
  ezStreamWriter* m_pWriter = nullptr;
  bool m_bFailed = false;
};
