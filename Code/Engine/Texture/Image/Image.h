#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>

#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/ImageHeader.h>

/// \brief A lightweight view to image data without owning the memory.
///
/// ezImageView provides read-only access to image data along with the metadata needed to interpret it.
/// It does not own the image data, so the underlying memory must remain valid for the lifetime of the view.
/// This class is ideal for passing image data around without unnecessary copying.
///
/// Use cases:
/// - Passing images to functions that only read data
/// - Creating temporary views to sub-regions of larger images
/// - Interfacing with external image processing libraries
/// - Converting between different image representations
class EZ_TEXTURE_DLL ezImageView : protected ezImageHeader
{
public:
  /// \brief Constructs an empty image view.
  ezImageView();

  /// \brief Constructs an image view with the given header and image data.
  ezImageView(const ezImageHeader& header, ezConstByteBlobPtr imageData);

  /// \brief Resets to an empty state, releasing the reference to external data.
  void Clear();

  /// \brief Returns false if the image view does not reference any data yet.
  bool IsValid() const;

  /// \brief Resets the view to reference new external image data.
  ///
  /// Any previous data reference is released. The new data must remain valid
  /// for the lifetime of this view.
  void ResetAndViewExternalStorage(const ezImageHeader& header, ezConstByteBlobPtr imageData);

  /// \brief Convenience function to save the image to the given file.
  ezResult SaveTo(ezStringView sFileName) const;

  /// \brief Returns the header this image was constructed from.
  const ezImageHeader& GetHeader() const;

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  ezBlobPtr<const T> GetBlobPtr() const;

  ezConstByteBlobPtr GetByteBlobPtr() const;

  /// \brief Returns a view to the given sub-image.
  ezImageView GetSubImageView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0) const;

  /// \brief Returns a view to a sub-plane.
  ezImageView GetPlaneView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to z slice of the image.
  ezImageView GetSliceView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 z = 0, ezUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to a row of pixels resp. blocks.
  ezImageView GetRowView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 y = 0, ezUInt32 z = 0, ezUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  const T* GetPixelPointer(
    ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 x = 0, ezUInt32 y = 0, ezUInt32 z = 0, ezUInt32 uiPlaneIndex = 0) const;

  /// \brief Reinterprets the image with a given format; the format must have the same size in bits per pixel as the current one.
  void ReinterpretAs(ezImageFormat::Enum format);

public:
  using ezImageHeader::GetDepth;
  using ezImageHeader::GetHeight;
  using ezImageHeader::GetWidth;

  using ezImageHeader::GetNumArrayIndices;
  using ezImageHeader::GetNumFaces;
  using ezImageHeader::GetNumMipLevels;
  using ezImageHeader::GetPlaneCount;

  using ezImageHeader::GetImageFormat;

  using ezImageHeader::GetNumBlocksX;
  using ezImageHeader::GetNumBlocksY;
  using ezImageHeader::GetNumBlocksZ;

  using ezImageHeader::GetDepthPitch;
  using ezImageHeader::GetRowPitch;

protected:
  ezUInt64 ComputeLayout();

  void ValidateSubImageIndices(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiPlaneIndex) const;
  template <typename T>
  void ValidateDataTypeAccessor(ezUInt32 uiPlaneIndex) const;

  const ezUInt64& GetSubImageOffset(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiPlaneIndex) const;

  ezHybridArray<ezUInt64, 16> m_SubImageOffsets;
  ezBlobPtr<ezUInt8> m_DataPtr;
};

/// \brief Container for image data with automatic memory management.
///
/// ezImage extends ezImageView by owning the image data it references. It can use either internal storage
/// or attach to external memory. This class handles allocation, deallocation, and provides convenient
/// methods for loading, saving, and converting images.
///
/// Memory management:
/// - Internal storage: ezImage allocates and manages its own memory
/// - External storage: ezImage references user-provided memory (user manages lifetime)
/// - Storage can be switched between internal and external as needed
///
/// The sub-images are stored in a predefined order compatible with DDS files:
/// For each array slice: mip level 0, mip level 1, ..., mip level N
/// For cubemaps: +X, -X, +Y, -Y, +Z, -Z faces in that order
/// For texture arrays: array slice 0, array slice 1, ..., array slice N
///
/// Common usage patterns:
/// ```cpp
/// // Load from file
/// ezImage image;
/// image.LoadFrom("texture.png");
///
/// // Create with specific format
/// ezImageHeader header;
/// header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
/// header.SetWidth(256); header.SetHeight(256);
/// ezImage image(header);
///
/// // Convert format
/// image.Convert(ezImageFormat::BC1_UNORM);
/// ```
class EZ_TEXTURE_DLL ezImage : public ezImageView
{
  /// Use Reset() instead
  void operator=(const ezImage& rhs) = delete;

  /// Use Reset() instead
  void operator=(const ezImageView& rhs) = delete;

  /// \brief Constructs an image with the given header; allocating internal storage for it.
  explicit ezImage(const ezImageHeader& header);

  /// \brief Constructs an image with the given header backed by user-supplied external storage.
  explicit ezImage(const ezImageHeader& header, ezByteBlobPtr externalData);

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

  /// \brief Allocates storage for an image with the given header.
  ///
  /// If currently using external storage and it's large enough, that storage will be reused.
  /// Otherwise, the image will detach from external storage and allocate internal storage.
  /// Any existing data is discarded.
  void ResetAndAlloc(const ezImageHeader& header);

  /// \brief Attaches the image to external storage provided by the user.
  ///
  /// The external storage must remain valid for the lifetime of this ezImage.
  /// The storage must be large enough to hold the image data described by the header.
  /// Use this when you want to avoid memory allocation or work with memory-mapped files.
  void ResetAndUseExternalStorage(const ezImageHeader& header, ezByteBlobPtr externalData);

  /// \brief Takes ownership of another image's data via move semantics.
  ///
  /// The other image is left in an empty state. If the other image uses external storage,
  /// this image will also reference that storage and inherit the lifetime requirements.
  void ResetAndMove(ezImage&& other);

  /// \brief Copies data from an image view into internal storage.
  ///
  /// If currently attached to external storage, the attachment is discarded and internal
  /// storage is allocated. The source view's data is copied completely.
  void ResetAndCopy(const ezImageView& other);

  /// \brief Convenience function to load the image from the given file.
  ezResult LoadFrom(ezStringView sFileName);

  /// \brief Convenience function to convert the image to the given format.
  ezResult Convert(ezImageFormat::Enum targetFormat);

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  ezBlobPtr<T> GetBlobPtr();

  ezByteBlobPtr GetByteBlobPtr();

  using ezImageView::GetBlobPtr;
  using ezImageView::GetByteBlobPtr;

  /// \brief Returns a view to the given sub-image.
  ezImage GetSubImageView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0);

  using ezImageView::GetSubImageView;

  /// \brief Returns a view to a sub-plane.
  ezImage GetPlaneView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 uiPlaneIndex = 0);

  using ezImageView::GetPlaneView;

  /// \brief Returns a view to z slice of the image.
  ezImage GetSliceView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 z = 0, ezUInt32 uiPlaneIndex = 0);

  using ezImageView::GetSliceView;

  /// \brief Returns a view to a row of pixels resp. blocks.
  ezImage GetRowView(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 y = 0, ezUInt32 z = 0, ezUInt32 uiPlaneIndex = 0);

  using ezImageView::GetRowView;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  T* GetPixelPointer(ezUInt32 uiMipLevel = 0, ezUInt32 uiFace = 0, ezUInt32 uiArrayIndex = 0, ezUInt32 x = 0, ezUInt32 y = 0, ezUInt32 z = 0, ezUInt32 uiPlaneIndex = 0);

  using ezImageView::GetPixelPointer;

private:
  bool UsesExternalStorage() const;

  ezBlob m_InternalStorage;
};

#include <Texture/Image/Implementation/Image_inl.h>
