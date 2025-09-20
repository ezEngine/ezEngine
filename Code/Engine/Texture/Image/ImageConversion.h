#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>

#include <Texture/Image/Image.h>

EZ_DECLARE_FLAGS(ezUInt8, ezImageConversionFlags, InPlace);

/// \brief Describes a single conversion step between two image formats.
///
/// Used by conversion step implementations to advertise which format pairs they can handle.
/// The conversion system uses this information to build optimal conversion paths.
struct ezImageConversionEntry
{
  ezImageConversionEntry(ezImageFormat::Enum source, ezImageFormat::Enum target, ezImageConversionFlags::Enum flags, float fAdditionalPenalty = 0)
    : m_sourceFormat(source)
    , m_targetFormat(target)
    , m_flags(flags)
    , m_fAdditionalPenalty(fAdditionalPenalty)
  {
  }

  const ezImageFormat::Enum m_sourceFormat;
  const ezImageFormat::Enum m_targetFormat;
  const ezBitflags<ezImageConversionFlags> m_flags;

  /// Additional cost penalty for this conversion step.
  ///
  /// Used to bias the pathfinding algorithm when multiple conversion routes are available.
  /// Higher penalties make this step less likely to be chosen in the optimal path.
  float m_fAdditionalPenalty = 0.0f;
};

/// \brief Interface for a single image conversion step.
///
/// The actual functionality is implemented as either ezImageConversionStepLinear or ezImageConversionStepDecompressBlocks.
/// Depending on the types on conversion advertised by GetSupportedConversions(), users of this class need to cast it to a derived type
/// first to access the desired functionality.
class EZ_TEXTURE_DLL ezImageConversionStep : public ezEnumerable<ezImageConversionStep>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezImageConversionStep);

protected:
  ezImageConversionStep();
  virtual ~ezImageConversionStep();

public:
  /// \brief Returns an array pointer of supported conversions.
  ///
  /// \note The returned array must have the same entries each time this method is called.
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const = 0;
};

/// \brief Interface for a single image conversion step where both the source and target format are uncompressed.
class EZ_TEXTURE_DLL ezImageConversionStepLinear : public ezImageConversionStep
{
public:
  /// \brief Converts a batch of pixels.
  virtual ezResult ConvertPixels(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt64 uiNumElements, ezImageFormat::Enum sourceFormat,
    ezImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is compressed and the target format is uncompressed.
class EZ_TEXTURE_DLL ezImageConversionStepDecompressBlocks : public ezImageConversionStep
{
public:
  /// \brief Decompresses the given number of blocks.
  virtual ezResult DecompressBlocks(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 uiNumBlocks, ezImageFormat::Enum sourceFormat,
    ezImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is uncompressed and the target format is compressed.
class EZ_TEXTURE_DLL ezImageConversionStepCompressBlocks : public ezImageConversionStep
{
public:
  /// \brief Compresses the given number of blocks.
  virtual ezResult CompressBlocks(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 uiNumBlocksX, ezUInt32 uiNumBlocksY,
    ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a linear to a planar format.
class EZ_TEXTURE_DLL ezImageConversionStepPlanarize : public ezImageConversionStep
{
public:
  /// \brief Converts a batch of pixels into the given target planes.
  virtual ezResult ConvertPixels(const ezImageView& source, ezArrayPtr<ezImage> target, ezUInt32 uiNumPixelsX, ezUInt32 uiNumPixelsY, ezImageFormat::Enum sourceFormat,
    ezImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a planar to a linear format.
class EZ_TEXTURE_DLL ezImageConversionStepDeplanarize : public ezImageConversionStep
{
public:
  /// \brief Converts a batch of pixels from the given source planes.
  virtual ezResult ConvertPixels(ezArrayPtr<ezImageView> source, ezImage target, ezUInt32 uiNumPixelsX, ezUInt32 uiNumPixelsY, ezImageFormat::Enum sourceFormat,
    ezImageFormat::Enum targetFormat) const = 0;
};


/// \brief High-level image format conversion system with automatic path finding.
///
/// This class provides a complete image conversion system that can automatically find
/// optimal conversion paths between any two supported formats. It uses a plugin-based
/// architecture where conversion steps register themselves at startup.
///
/// **Basic Usage:**
/// ```cpp
/// // Simple format conversion
/// ezImage sourceImage;
/// sourceImage.LoadFrom("texture.png");
/// ezImage targetImage;
/// ezImageConversion::Convert(sourceImage, targetImage, ezImageFormat::BC1_UNORM);
/// ```
///
/// **Advanced Usage with Path Caching:**
/// ```cpp
/// // Build reusable conversion path
/// ezHybridArray<ezImageConversion::ConversionPathNode, 16> path;
/// ezUInt32 numScratchBuffers;
/// ezImageConversion::BuildPath(sourceFormat, targetFormat, false, path, numScratchBuffers);
///
/// // Use cached path for multiple conversions
/// for (auto& image : images)
/// {
///   ezImageConversion::Convert(image, convertedImage, path, numScratchBuffers);
/// }
/// ```
///
/// The conversion system automatically handles:
/// - Multi-step conversions (e.g., BC1 -> RGBA8 -> BC7)
/// - Memory layout differences (linear, block-compressed, planar)
/// - Optimal path selection based on quality and performance
/// - In-place conversions when possible
class EZ_TEXTURE_DLL ezImageConversion
{
public:
  /// \brief Checks if a conversion path exists between two formats.
  ///
  /// This is a fast query that doesn't build the actual conversion path.
  /// Use this to validate format compatibility before attempting conversion.
  static bool IsConvertible(ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat);

  /// \brief Finds the format requiring the least conversion cost from a list of candidates.
  ///
  /// Useful when you have multiple acceptable target formats and want to choose
  /// the one that preserves the most quality or requires the least processing.
  static ezImageFormat::Enum FindClosestCompatibleFormat(ezImageFormat::Enum format, ezArrayPtr<const ezImageFormat::Enum> compatibleFormats);

  /// \brief A single node along a computed conversion path.
  struct ConversionPathNode
  {
    EZ_DECLARE_POD_TYPE();

    const ezImageConversionStep* m_step;
    ezImageFormat::Enum m_sourceFormat;
    ezImageFormat::Enum m_targetFormat;
    ezUInt32 m_sourceBufferIndex;
    ezUInt32 m_targetBufferIndex;
    bool m_inPlace;
  };

  /// \brief Precomputes an optimal conversion path between two formats and the minimal number of required scratch buffers.
  ///
  /// The generated path can be cached by the user if the same conversion is performed multiple times. The path must not be reused if the
  /// set of supported conversions changes, e.g. when plugins are loaded or unloaded.
  ///
  /// \param sourceFormat           The source format.
  /// \param targetFormat           The target format.
  /// \param sourceEqualsTarget     If true, the generated path is applicable if source and target memory regions are equal, and may contain
  /// additional copy-steps if the conversion can't be performed in-place.
  ///                               A path generated with sourceEqualsTarget == true will work correctly even if source and target are not
  ///                               the same, but may not be optimal. A path generated with sourceEqualsTarget == false will not work
  ///                               correctly when source and target are the same.
  /// \param out_path               The generated path.
  /// \param out_numScratchBuffers The number of scratch buffers required for the conversion path.
  /// \returns                      ez_SUCCESS if a path was found, ez_FAILURE otherwise.
  static ezResult BuildPath(ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat, bool bSourceEqualsTarget,
    ezHybridArray<ConversionPathNode, 16>& out_path, ezUInt32& out_uiNumScratchBuffers);

  /// \brief  Converts the source image into a target image with the given format. Source and target may be the same.
  static ezResult Convert(const ezImageView& source, ezImage& ref_target, ezImageFormat::Enum targetFormat);

  /// \brief Converts the source image into a target image using a precomputed conversion path.
  static ezResult Convert(const ezImageView& source, ezImage& ref_target, ezArrayPtr<ConversionPathNode> path, ezUInt32 uiNumScratchBuffers);

  /// \brief Converts the raw source data into a target data buffer with the given format. Source and target may be the same.
  static ezResult ConvertRaw(
    ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 uiNumElements, ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat);

  /// \brief Converts the raw source data into a target data buffer using a precomputed conversion path.
  static ezResult ConvertRaw(
    ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 uiNumElements, ezArrayPtr<ConversionPathNode> path, ezUInt32 uiNumScratchBuffers);

private:
  ezImageConversion();
  ezImageConversion(const ezImageConversion&);

  static ezResult ConvertSingleStep(const ezImageConversionStep* pStep, const ezImageView& source, ezImage& target, ezImageFormat::Enum targetFormat);

  static ezResult ConvertSingleStepDecompress(const ezImageView& source, ezImage& target, ezImageFormat::Enum sourceFormat,
    ezImageFormat::Enum targetFormat, const ezImageConversionStep* pStep);

  static ezResult ConvertSingleStepCompress(const ezImageView& source, ezImage& target, ezImageFormat::Enum sourceFormat,
    ezImageFormat::Enum targetFormat, const ezImageConversionStep* pStep);

  static ezResult ConvertSingleStepDeplanarize(const ezImageView& source, ezImage& target, ezImageFormat::Enum sourceFormat,
    ezImageFormat::Enum targetFormat, const ezImageConversionStep* pStep);

  static ezResult ConvertSingleStepPlanarize(const ezImageView& source, ezImage& target, ezImageFormat::Enum sourceFormat,
    ezImageFormat::Enum targetFormat, const ezImageConversionStep* pStep);

  static void RebuildConversionTable();
};
