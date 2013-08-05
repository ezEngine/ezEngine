#pragma once

#include <Foundation\Basics\Assert.h>
#include <Foundation/Math/Math.h>

#include "ezImageDefinitions.h"
#include "ezImageFormat.h"

class ezImageHeader
{
public:
  ezImageHeader() :
    m_uiWidth(0), m_uiHeight(0), m_uiDepth(1),
    m_uiNumMipLevels(1), m_uiNumFaces(1), m_uiNumArrayIndices(1),
    m_format(ezImageFormat::UNKNOWN)
  {
  }

  void SetImageFormat(const ezImageFormat::Enum& format)
  {
    m_format = format;
  }

  const ezImageFormat::Enum& GetImageFormat() const
  {
    return m_format;
  }

  void SetWidth(ezUInt32 uiWidth)
  {
    m_uiWidth = uiWidth;
  }

  ezUInt32 GetWidth(ezUInt32 uiMipLevel = 0) const
  {
    EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return ezMath::Max(m_uiWidth >> uiMipLevel, 1U);
  }

  void SetHeight(ezUInt32 uiHeight)
  {
    m_uiHeight = uiHeight;
  }

  ezUInt32 GetHeight(ezUInt32 uiMipLevel = 0) const
  {
    EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return ezMath::Max(m_uiHeight >> uiMipLevel, 1U);
  }

  void SetDepth(ezUInt32 uiDepth)
  {
    m_uiDepth = uiDepth;
  }

  ezUInt32 GetDepth(ezUInt32 uiMipLevel = 0) const
  {
    EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return ezMath::Max(m_uiDepth >> uiMipLevel, 1U);
  }

  void SetNumMipLevels(ezUInt32 uiNumMipLevels)
  {
    m_uiNumMipLevels = uiNumMipLevels;
  }

  ezUInt32 GetNumMipLevels() const
  {
    return m_uiNumMipLevels;
  }

  void SetNumFaces(ezUInt32 uiNumFaces)
  {
    m_uiNumFaces = uiNumFaces;
  }

  ezUInt32 GetNumFaces() const
  {
    return m_uiNumFaces;
  }

  void SetNumArrayIndices(ezUInt32 uiNumArrayIndices)
  {
    m_uiNumArrayIndices = uiNumArrayIndices;
  }

  ezUInt32 GetNumArrayIndices() const
  {
    return m_uiNumArrayIndices;
  }

protected:
  ezUInt32 m_uiNumMipLevels;
  ezUInt32 m_uiNumFaces;
  ezUInt32 m_uiNumArrayIndices;

  ezUInt32 m_uiWidth;
  ezUInt32 m_uiHeight;
  ezUInt32 m_uiDepth;

  ezImageFormat::Enum m_format;
};

