#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>

#include <Texture/Image/Image.h>

EZ_DECLARE_FLAGS(ezUInt8, ezImageConversionFlags, InPlace);

/// A structure describing the pairs of source/target format that may be converted using the conversion routine.
struct ezImageConversionEntry
{
  ezImageConversionEntry(ezImageFormat::Enum source, ezImageFormat::Enum target, ezImageConversionFlags::Enum flags)
      : m_sourceFormat(source)
      , m_targetFormat(target)
      , m_flags(flags)
  {
  }

  const ezImageFormat::Enum m_sourceFormat;
  const ezImageFormat::Enum m_targetFormat;
  const ezBitflags<ezImageConversionFlags> m_flags;
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
  virtual ezResult ConvertPixels(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt64 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is compressed and the target format is uncompressed.
class EZ_TEXTURE_DLL ezImageConversionStepDecompressBlocks : public ezImageConversionStep
{
public:
  /// \brief Decompresses the given number of blocks.
  virtual ezResult DecompressBlocks(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 numBlocks,
                                    ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is uncompressed and the target format is compressed.
class EZ_TEXTURE_DLL ezImageConversionStepCompressBlocks : public ezImageConversionStep
{
public:
  /// \brief Compresses the given number of blocks.
  virtual ezResult CompressBlocks(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 numBlocksX, ezUInt32 numBlocksY,
                                  ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const = 0;
};


/// \brief Helper class containing utilities to convert between different image formats and layouts.
class EZ_TEXTURE_DLL ezImageConversion
{
public:
  /// \brief Checks if there is a known conversion path between the two formats
  static bool IsConvertible(ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat);

  /// \brief Finds the image format from a given list of formats which is the cheapest to convert to.
  static ezImageFormat::Enum FindClosestCompatibleFormat(ezImageFormat::Enum format,
                                                         ezArrayPtr<const ezImageFormat::Enum> compatibleFormats);

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
  /// \param path_out               The generated path.
  /// \param numScratchBuffers_out The number of scratch buffers required for the conversion path.
  /// \returns                      ez_SUCCESS if a path was found, ez_FAILURE otherwise.
  static ezResult BuildPath(ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat, bool sourceEqualsTarget,
                            ezHybridArray<ConversionPathNode, 16>& path_out, ezUInt32& numScratchBuffers_out);

  /// \brief  Converts the source image into a target image with the given format. Source and target may be the same.
  static ezResult Convert(const ezImageView& source, ezImage& target, ezImageFormat::Enum targetFormat);

  /// \brief Converts the source image into a target image using a precomputed conversion path.
  static ezResult Convert(const ezImageView& source, ezImage& target, ezArrayPtr<ConversionPathNode> path, ezUInt32 numScratchBuffers);

  /// \brief Converts the raw source data into a target data buffer with the given format. Source and target may be the same.
  static ezResult ConvertRaw(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 numElements, ezImageFormat::Enum sourceFormat,
                             ezImageFormat::Enum targetFormat);

  /// \brief Converts the raw source data into a target data buffer using a precomputed conversion path.
  static ezResult ConvertRaw(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 numElements,
                             ezArrayPtr<ConversionPathNode> path, ezUInt32 numScratchBuffers);

private:
  ezImageConversion();
  ezImageConversion(const ezImageConversion&);

  static ezResult ConvertSingleStep(const ezImageConversionStep* pStep, const ezImageView& source, ezImage& target,
                                    ezImageFormat::Enum targetFormat);

  static ezResult ConvertSingleStepDecompress(const ezImageView& source, ezImage& target, ezImageFormat::Enum sourceFormat,
                                              ezImageFormat::Enum targetFormat, const ezImageConversionStep* pStep);

  static ezResult ConvertSingleStepCompress(const ezImageView& source, ezImage& target, ezImageFormat::Enum sourceFormat,
                                            ezImageFormat::Enum targetFormat, const ezImageConversionStep* pStep);

  static void RebuildConversionTable();
};

