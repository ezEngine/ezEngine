#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>

#include <Foundation/Image/Implementation/ImageHeader.h>

/// \brief A class referencing image data and holding metadata about the image.
class EZ_FOUNDATION_DLL ezImageView : protected ezImageHeader
{
public:
  /// \brief Constructs an empty image view.
  ezImageView();

  /// \brief Constructs an image view with the given header and image data.
  ezImageView(const ezImageHeader& header, ezArrayPtr<const void> imageData);

  /// \brief Constructs an empty image view.
  void Clear();

  /// \brief Constructs an image view with the given header and image data.
  void ResetAndViewExternalStorage(const ezImageHeader& header, ezArrayPtr<const void> imageData);

  /// \brief Convenience function to save the image to the given file.
  ezResult SaveTo(const char* szFileName, ezLogInterface* pLog = ezLog::GetThreadLocalLogSystem()) const;

  /// \brief Returns the header this image was constructed from.
  const ezImageHeader& GetHeader() const;

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  ezArrayPtr<const T> GetArrayPtr() const;

  /// \brief Returns a view to the given sub-image.
  ezImageView GetSubImageView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0) const;

  /// \brief Returns a view to z slice of the image.
  ezImageView GetSliceView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 z = 0) const;

  /// \brief Returns a view to a row of pixels resp. blocks.
  ezImageView GetRowView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 y = 0, ezUInt32 z = 0) const;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  const T* GetPixelPointer(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 x = 0, ezUInt32 y = 0,
                           ezUInt32 z = 0) const;

  void ForceSRGBFormat();

public:
  using ezImageHeader::GetDepth;
  using ezImageHeader::GetHeight;
  using ezImageHeader::GetWidth;

  using ezImageHeader::GetNumArrayIndices;
  using ezImageHeader::GetNumFaces;
  using ezImageHeader::GetNumMipLevels;

  using ezImageHeader::GetImageFormat;

  using ezImageHeader::GetNumBlocksX;
  using ezImageHeader::GetNumBlocksY;
  using ezImageHeader::GetNumBlocksZ;

  using ezImageHeader::GetDepthPitch;
  using ezImageHeader::GetRowPitch;

protected:
  ezUInt32 ComputeLayout();

  void ValidateSubImageIndices(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;
  template <typename T>
  void ValidateDataTypeAccessor() const;

  const ezUInt32& GetSubImageOffset(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;

  ezHybridArray<ezUInt32, 16> m_subImageOffsets;
  ezArrayPtr<ezUInt8> m_dataPtr;
};

/// \brief A class containing image data and associated meta data.
///
/// This class is a lightweight container for image data and the description required for interpreting the data,
/// such as the image format, its dimensions, number of sub-images (i.e. cubemap faces, mip levels and array sub-images).
/// However, it does not provide any methods for interpreting or  modifying of the image data.
///
/// The sub-images are stored in a predefined order compatible with the layout of DDS files, that is, it first stores
/// the mip chain for each image, then all faces in a case of a cubemap, then the individual images of an image array.
class EZ_FOUNDATION_DLL ezImage : public ezImageView
{
  /// Use Reset() instead
  void operator=(const ezImage& rhs) = delete;

  /// Use Reset() instead
  void operator=(const ezImageView& rhs) = delete;

  /// \brief Constructs an image with the given header; allocating internal storage for it.
  explicit ezImage(const ezImageHeader& header);

  /// \brief Constructs an image with the given header backed by user-supplied external storage.
  explicit ezImage(const ezImageHeader& header, ezArrayPtr<void> externalData);
  
  /// \brief Constructor from image view (copies the image data to internal storage)
  explicit ezImage(const ezImageView& other);

public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Constructs an empty image.
  ezImage();

  /// \brief Move constructor
  ezImage(ezImage&& other);

  void operator=(ezImage&& rhs);

  /// \brief Constructs an empty image. If the image is attached to an external storage, the attachment is discarded.
  void Clear();

  /// \brief Constructs an image with the given header and ensures sufficient storage is allocated.
  ///
  /// \note If this ezImage was previously attached to external storage, this will reuse that storage.
  /// However, if the external storage is not sufficiently large, ResetAndAlloc() will detach from it and allocate internal storage.
  void ResetAndAlloc(const ezImageHeader& header);

  /// \brief Constructs an image with the given header and attaches to the user-supplied external storage.
  ///
  /// The user is responsible to keep the external storage alive as long as this ezImage is alive.
  void ResetAndUseExternalStorage(const ezImageHeader& header, ezArrayPtr<void> externalData);

  /// \brief Moves the given data into this object.
  ///
  /// If \a other is attached to an external storage, this object will also be attached to it,
  /// so life-time requirements for the external storage are now bound to this instance.
  void ResetAndMove(ezImage&& other);

  /// \brief Constructs from an image view. Copies the image data to internal storage.
  ///
  /// If the image is currently attached to external storage, the attachment is discarded.
  void ResetAndCopy(const ezImageView& other);

  /// \brief Convenience function to load the image from the given file.
  ezResult LoadFrom(const char* szFileName, ezLogInterface* pLog = ezLog::GetThreadLocalLogSystem());

  /// \brief Convenience function to convert the image to the given format.
  ezResult Convert(ezImageFormat::Enum targetFormat);

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  ezArrayPtr<T> GetArrayPtr();

  using ezImageView::GetArrayPtr;

  /// \brief Returns a view to the given sub-image.
  ezImage GetSubImageView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0);

  using ezImageView::GetSubImageView;

  /// \brief Returns a view to z slice of the image.
  ezImage GetSliceView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 z = 0);

  using ezImageView::GetSliceView;

  /// \brief Returns a view to a row of pixels resp. blocks.
  ezImage GetRowView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 y = 0, ezUInt32 z = 0);

  using ezImageView::GetRowView;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  T* GetPixelPointer(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 x = 0, ezUInt32 y = 0,
                     ezUInt32 z = 0);

  using ezImageView::GetPixelPointer;

private:
  bool UsesExternalStorage() const;

  ezDynamicArray<ezUInt8> m_internalStorage;
};

#include <Foundation/Image/Implementation/Image_inl.h>
