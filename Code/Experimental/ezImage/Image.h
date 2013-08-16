#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>

#include <ImageHeader.h>

/// \brief A class containing image data and associated meta data.
///
/// This class is a lightweight container for image data and the description required for interpreting the data,
/// such as the image format, its dimensions, number of subimages (i.e. cubemap faces, mip levels and array subimages).
/// However, it does not provide any methods for interpreting or  modifying of the image data.
/// 
/// The subimages are stored in a predefined order compatible with the layout of DDS files, that is, it first stores
/// the mip chain for each image, then all faces in a case of a cubemap, then the individual images of an image array.
class ezImage : public ezImageHeader
{
public:
  /// \brief Constructs an empty image.
  inline ezImage();

  /// \brief Convenience function to load the image from the given file.
  ezResult LoadFrom(const char* szFileName);

  /// \brief Convenience function to save the image to the given file.
  ezResult SaveTo(const char* szFileName);

  /// \brief Sets the alignment to use for each pixel row. The default is 1.
  inline void SetRowAlignment(ezUInt32 uiRowAlignment);

  /// \brief Returns the alignment to use for each pixel row. The default is 1.
  inline ezUInt32 GetRowAlignment() const;

  /// \brief Sets the alignment to use for each depth slice. The default is 1.
  inline void SetDepthAlignment(ezUInt32 uiDepthAlignment);

  /// \brief Returns the alignment to use for each depth slice. The default is 1.
  inline ezUInt32 GetDepthAlignment() const;

  /// \brief Sets the alignment to use for each subimage. The default is 1.
  inline void SetSubImageAlignment(ezUInt32 uiSubImageAlignment);

  /// \brief Returns the alignment to use for each subimage. The default is 1.
  inline ezUInt32 GetSubImageAlignment() const;


  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  ///
  /// This method is only valid to call for block-compressed formats.
  inline ezUInt32 GetNumBlocksX(ezUInt32 uiMipLevel) const;

  /// \brief Returns the number of blocks contained in a given mip level in the vertical direction.
  ///
  /// This method is only valid to call for block-compressed formats.
  inline ezUInt32 GetNumBlocksY(ezUInt32 uiMipLevel) const;


  /// \brief Returns the size of the allocated image data.
  ///
  /// In addition to the returned size, 16 additional bytes will be allocated, which can be exploited
  /// to simplify copying or modification at aligned boundaries.
  inline ezUInt32 GetDataSize() const;

  /// \brief Returns a pointer to the beginning of the data contained in this image.
  template<typename T>
  T* GetDataPointer();

  /// \brief Returns a pointer to the beginning of the data contained in this image.
  template<typename T>
  const T* GetDataPointer() const;

  /// \brief Returns a pointer to the beginning of a given subimage.
  template<typename T>
  const T* GetSubImagePointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;

  /// \brief Returns a pointer to the beginning of a given subimage.
  template<typename T>
  T* GetSubImagePointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex);

  /// \brief Returns a pointer to a given pixel contained in a subimage.
  ///
  /// This method is only valid to use when the image format is a linear pixel format.
  template<typename T>
  const T* GetPixelPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z) const;

  /// \brief Returns a pointer to a given pixel contained in a subimage.
  ///
  /// This method is only valid to use when the image format is a linear pixel format.
  template<typename T>
  T* GetPixelPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z);

  /// \brief Returns a pointer to a given block contained in a subimage.
  ///
  /// This method is only valid to use when the image format is a block compressed format.
  template<typename T>
  const T* GetBlockPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 z) const;

  /// \brief Returns a pointer to a given block contained in a subimage.
  ///
  /// This method is only valid to use when the image format is a block compressed format.
  template<typename T>
  T* GetBlockPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 z);

  /// \brief Allocates the storage space required for the configured number of subimages.
  ///
  /// When creating an image, call this method after setting the dimensions and number of mip levels, faces and array indices.
  /// Changing the image dimensions, alignment or number of subimages will not automatically reallocate the data.
  void AllocateImageData();

  /// \brief Returns the offset in bytes between two subsequent rows of the given mip level.
  ///
  /// This method is only valid to use when the image format is a linear pixel format.
  inline ezUInt32 GetRowPitch(ezUInt32 uiMipLevel) const;

  /// \brief Returns the offset in bytes between two subsequent depth slices of the given mip level.
  inline ezUInt32 GetDepthPitch(ezUInt32 uiMipLevel) const;

  /// \brief Returns the position in bytes in the data array of the given subimage.
  inline ezUInt32 GetDataOffSet(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;
 
private:
  struct SubImage
  {
    EZ_DECLARE_POD_TYPE();

    int m_uiRowPitch;
    int m_uiDepthPitch;
    int m_uiDataOffset;
  };

  inline void ValidateSubImageIndices(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;

  inline SubImage& GetSubImage(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex);

  inline const SubImage& GetSubImage(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;

  ezUInt32 m_uiRowAlignment;
  ezUInt32 m_uiDepthAlignment;
  ezUInt32 m_uiSubImageAlignment;

  ezHybridArray<SubImage, 16> m_subImages;
  ezDynamicArray<ezUInt8> m_data;
};

#include <Implementation/Image_inl.h>