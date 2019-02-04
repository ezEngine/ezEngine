#pragma once

#include <Foundation/Basics/Assert.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Math.h>

#include <Texture/Export.h>
#include <Texture/Image/ImageFormat.h>

/// \brief A class containing image meta data, such as format and dimensions.
///
/// This class has no associated behavior or functionality, and its getters and setters have no effect other than changing
/// the contained value. It is intended as a container to be modified by image utils and loaders.
class EZ_TEXTURE_DLL ezImageHeader
{
public:
  /// \brief Constructs an image using an unknown format and zero size.
  ezImageHeader() { Clear(); }

  /// \brief Constructs an image using an unknown format and zero size.
  void Clear()
  {
    m_uiNumMipLevels = 1;
    m_uiNumFaces = 1;
    m_uiNumArrayIndices = 1;
    m_uiWidth = 0;
    m_uiHeight = 0;
    m_uiDepth = 1;
    m_format = ezImageFormat::UNKNOWN;
  }

  /// \brief Sets the image format.
  void SetImageFormat(const ezImageFormat::Enum& format) { m_format = format; }

  /// \brief Returns the image format.
  ezImageFormat::Enum GetImageFormat() const { return m_format; }

  /// \brief Sets the image width.
  void SetWidth(ezUInt32 uiWidth) { m_uiWidth = uiWidth; }

  /// \brief Returns the image width for a given mip level, clamped to 1.
  ezUInt32 GetWidth(ezUInt32 uiMipLevel = 0) const
  {
    EZ_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return ezMath::Max(m_uiWidth >> uiMipLevel, 1U);
  }

  /// \brief Sets the image height.
  void SetHeight(ezUInt32 uiHeight) { m_uiHeight = uiHeight; }

  /// \brief Returns the image height for a given mip level, clamped to 1.
  ezUInt32 GetHeight(ezUInt32 uiMipLevel = 0) const
  {
    EZ_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return ezMath::Max(m_uiHeight >> uiMipLevel, 1U);
  }

  /// \brief Sets the image depth. The default is 1.
  void SetDepth(ezUInt32 uiDepth) { m_uiDepth = uiDepth; }

  /// \brief Returns the image depth for a given mip level, clamped to 1.
  ezUInt32 GetDepth(ezUInt32 uiMipLevel = 0) const
  {
    EZ_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return ezMath::Max(m_uiDepth >> uiMipLevel, 1U);
  }

  /// \brief Sets the number of mip levels, including the full-size image.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumMipLevels(ezUInt32 uiNumMipLevels) { m_uiNumMipLevels = uiNumMipLevels; }

  /// \brief Returns the number of mip levels, including the full-size image.
  ezUInt32 GetNumMipLevels() const { return m_uiNumMipLevels; }

  /// \brief Sets the number of cubemap faces. Use 1 for a non-cubemap.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumFaces(ezUInt32 uiNumFaces) { m_uiNumFaces = uiNumFaces; }

  /// \brief Returns the number of cubemap faces, or 1 for a non-cubemap.
  ezUInt32 GetNumFaces() const { return m_uiNumFaces; }

  /// \brief Sets the number of array indices.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumArrayIndices(ezUInt32 uiNumArrayIndices) { m_uiNumArrayIndices = uiNumArrayIndices; }

  /// \brief Returns the number of array indices.
  ezUInt32 GetNumArrayIndices() const { return m_uiNumArrayIndices; }

  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  ezUInt32 GetNumBlocksX(ezUInt32 uiMipLevel = 0) const { return ezImageFormat::GetNumBlocksX(m_format, GetWidth(uiMipLevel)); }

  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  ezUInt32 GetNumBlocksY(ezUInt32 uiMipLevel = 0) const { return ezImageFormat::GetNumBlocksY(m_format, GetHeight(uiMipLevel)); }

  /// \brief Returns the number of blocks contained in a given mip level in the depth direction.
  ezUInt32 GetNumBlocksZ(ezUInt32 uiMipLevel = 0) const { return ezImageFormat::GetNumBlocksZ(m_format, GetHeight(uiMipLevel)); }

  /// \brief Returns the offset in bytes between two subsequent rows of the given mip level.
  ezUInt32 GetRowPitch(ezUInt32 uiMipLevel = 0) const { return ezImageFormat::GetRowPitch(GetImageFormat(), GetWidth(uiMipLevel)); }

  /// \brief Returns the offset in bytes between two subsequent depth slices of the given mip level.
  ezUInt32 GetDepthPitch(ezUInt32 uiMipLevel = 0) const
  {
    return ezImageFormat::GetDepthPitch(GetImageFormat(), GetWidth(uiMipLevel), GetHeight(uiMipLevel));
  }

  /// \brief Computes the data size required for an image with the header's format and dimensions.
  ezUInt32 ComputeDataSize() const
  {
    ezUInt32 uiDataSize = 0;

    for (ezUInt32 uiMipLevel = 0; uiMipLevel < GetNumMipLevels(); uiMipLevel++)
    {
      uiDataSize += GetDepthPitch(uiMipLevel) * GetDepth(uiMipLevel);
    }

    return uiDataSize * GetNumArrayIndices() * GetNumFaces();
  }

  /// \brief Computes the number of mip maps in the full mip chain.
  ezUInt32 ComputeNumberOfMipMaps() const
  {
    ezUInt32 numMipMaps = 1;
    ezUInt32 width = GetWidth();
    ezUInt32 height = GetHeight();
    ezUInt32 depth = GetDepth();

    while (width > 1 || height > 1 || depth > 1)
    {
      width = ezMath::Max(1u, width / 2);
      height = ezMath::Max(1u, height / 2);
      depth = ezMath::Max(1u, depth / 2);

      numMipMaps++;
    }

    return numMipMaps;
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

