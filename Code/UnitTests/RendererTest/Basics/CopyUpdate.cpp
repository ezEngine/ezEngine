#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererTest/Basics/CopyUpdate.h>
#include <RendererTest/Basics/RendererTestUtils.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

namespace
{
  constexpr ezUInt32 s_uiCopyTexturePaddingBytes = 0;
  constexpr ezUInt32 s_uiCopyTextureNpotPaddingBytes = 4;
  constexpr ezUInt32 s_uiCopyTextureArrayPaddingBytes = 8;
  constexpr ezUInt32 s_uiCopyTextureCubePaddingBytes = 12;
  constexpr ezUInt32 s_uiCopyTextureCubeArrayPaddingBytes = 16;
  constexpr ezUInt32 s_uiCopyTextureBC1PaddingBytes = 8;

  void FillBufferPattern(ezDynamicArray<ezUInt8>& out_buffer, ezUInt32 uiSize, bool bReverse)
  {
    out_buffer.SetCountUninitialized(uiSize);
    for (ezUInt32 i = 0; i < uiSize; ++i)
    {
      out_buffer[i] = static_cast<ezUInt8>(bReverse ? 255 - i : i);
    }
  }

  /// Creates a buffer suitable for the buffer copy/update tests.
  ///
  /// We use a structured buffer because that combination of flags supports both copy operations and AheadOfTime updates,
  /// and the readback path doesn't depend on the binding type.
  ezGALBufferHandle CreateTestBuffer(ezGALDevice* pDevice, ezUInt32 uiTotalSize, ezArrayPtr<const ezUInt8> initialData)
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = uiTotalSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = false;
    return pDevice->CreateBuffer(desc, initialData);
  }

  ezGALTextureCreationDescription CreateTextureDesc(ezGALTextureType::Enum type, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiMipLevels, ezUInt32 uiArraySize = 1, ezGALResourceFormat::Enum format = ezGALResourceFormat::BGRAUByteNormalizedsRGB)
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_uiMipLevelCount = static_cast<ezUInt8>(uiMipLevels);
    desc.m_uiArraySize = uiArraySize;
    desc.m_Type = type;
    desc.m_Format = format;
    desc.m_ResourceAccess.m_bImmutable = false;
    return desc;
  }

  /// Returns the total number of array layers a texture description has, expanding cube maps to 6 faces per cube.
  ezUInt32 GetTotalSlices(const ezGALTextureCreationDescription& desc)
  {
    const bool bIsCube = (desc.m_Type == ezGALTextureType::TextureCube || desc.m_Type == ezGALTextureType::TextureCubeArray);
    return bIsCube ? desc.m_uiArraySize * 6 : desc.m_uiArraySize;
  }

  ezUInt32 GetExtraPaddingRows(ezUInt32 uiPaddingBytes, ezUInt32 uiBytesPerBlock)
  {
    return uiPaddingBytes / uiBytesPerBlock;
  }

  void CreateBC1Image(ezImage& ref_image, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiMipLevelCount, ezUInt8 uiSeed)
  {
    ezImage sourceImage;
    ezRendererTestUtils::CreateImage(sourceImage, uiWidth, uiHeight, uiMipLevelCount, false, uiSeed);
    EZ_TEST_BOOL(ezImageConversion::Convert(sourceImage, ref_image, ezImageFormat::BC1_UNORM).Succeeded());
  }

  void CreateTestImage(ezImage& ref_image, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiMipLevelCount, ezGALResourceFormat::Enum format, ezUInt8 uiSeed)
  {
    if (format == ezGALResourceFormat::BC1)
    {
      CreateBC1Image(ref_image, uiWidth, uiHeight, uiMipLevelCount, uiSeed);
      return;
    }

    ezRendererTestUtils::CreateImage(ref_image, uiWidth, uiHeight, uiMipLevelCount, false, uiSeed);
  }

  void CopyImageRegionBytes(const ezImage& sourceImage, ezUInt32 uiSourceMipLevel, const ezRectU32& sourceRect, ezImage& ref_destinationImage, ezUInt32 uiDestinationMipLevel, ezVec3U32 vDestinationOffset)
  {
    const ezImageFormat::Enum format = sourceImage.GetImageFormat();
    EZ_ASSERT_DEV(format == ref_destinationImage.GetImageFormat(), "Source and destination image formats must match.");

    const ezUInt32 uiBlockWidth = ezImageFormat::GetBlockWidth(format);
    const ezUInt32 uiBlockHeight = ezImageFormat::GetBlockHeight(format);
    EZ_ASSERT_DEV((sourceRect.x % uiBlockWidth) == 0 && (sourceRect.y % uiBlockHeight) == 0, "Source rectangle must be block-aligned.");
    EZ_ASSERT_DEV((vDestinationOffset.x % uiBlockWidth) == 0 && (vDestinationOffset.y % uiBlockHeight) == 0, "Destination offset must be block-aligned.");

    const ezUInt32 uiSourceBlockX = sourceRect.x / uiBlockWidth;
    const ezUInt32 uiSourceBlockY = sourceRect.y / uiBlockHeight;
    const ezUInt32 uiDestinationBlockX = vDestinationOffset.x / uiBlockWidth;
    const ezUInt32 uiDestinationBlockY = vDestinationOffset.y / uiBlockHeight;
    const ezUInt32 uiBlockRows = ezImageFormat::GetNumBlocksY(format, sourceRect.height);
    const ezUInt32 uiRowSize = static_cast<ezUInt32>(ezImageFormat::GetRowPitch(format, sourceRect.width));

    for (ezUInt32 y = 0; y < uiBlockRows; ++y)
    {
      const ezUInt8* pSrc = sourceImage.GetPixelPointer<ezUInt8>(uiSourceMipLevel, 0, 0, uiSourceBlockX, uiSourceBlockY + y);
      ezUInt8* pDst = ref_destinationImage.GetPixelPointer<ezUInt8>(uiDestinationMipLevel, 0, 0, uiDestinationBlockX, uiDestinationBlockY + y);
      ezMemoryUtils::Copy(pDst, pSrc, uiRowSize);
    }
  }

  ezGALSystemMemoryDescription BuildStridedData(const ezImage& image, ezUInt32 uiMipLevel, const ezRectU32& sourceRect, ezUInt32 uiPaddingBytes, ezDynamicArray<ezUInt8>& out_storage)
  {
    const ezImageFormat::Enum format = image.GetImageFormat();
    const ezUInt32 uiBlockWidth = ezImageFormat::GetBlockWidth(format);
    const ezUInt32 uiBlockHeight = ezImageFormat::GetBlockHeight(format);
    EZ_ASSERT_DEV((sourceRect.x % uiBlockWidth) == 0 && (sourceRect.y % uiBlockHeight) == 0, "Source rectangle must be block-aligned.");

    const ezUInt32 uiSourceBlockX = sourceRect.x / uiBlockWidth;
    const ezUInt32 uiSourceBlockY = sourceRect.y / uiBlockHeight;
    const ezUInt32 uiBlockRows = ezImageFormat::GetNumBlocksY(format, sourceRect.height);
    const ezUInt32 uiBlockColumns = ezImageFormat::GetNumBlocksX(format, sourceRect.width);
    const ezUInt32 uiTightRowPitch = static_cast<ezUInt32>(ezImageFormat::GetRowPitch(format, sourceRect.width));
    const ezUInt32 uiBytesPerBlock = uiTightRowPitch / uiBlockColumns;
    const ezUInt32 uiRowPitch = uiTightRowPitch + uiPaddingBytes;
    const ezUInt32 uiSlicePitch = uiRowPitch * (uiBlockRows + GetExtraPaddingRows(uiPaddingBytes, uiBytesPerBlock));

    out_storage.SetCount(uiSlicePitch, 0xCD);
    for (ezUInt32 y = 0; y < uiBlockRows; ++y)
    {
      const ezUInt8* pSrc = image.GetPixelPointer<ezUInt8>(uiMipLevel, 0, 0, uiSourceBlockX, uiSourceBlockY + y);
      ezUInt8* pDst = out_storage.GetData() + y * uiRowPitch;
      ezMemoryUtils::Copy(pDst, pSrc, uiTightRowPitch);
    }

    ezGALSystemMemoryDescription memory;
    memory.m_pData = out_storage.GetByteArrayPtr();
    memory.m_uiRowPitch = uiRowPitch;
    memory.m_uiSlicePitch = uiSlicePitch;
    return memory;
  }

  ezGALSystemMemoryDescription BuildStridedData(const ezImage& image, ezUInt32 uiMipLevel, ezUInt32 uiPaddingBytes, ezDynamicArray<ezUInt8>& out_storage)
  {
    return BuildStridedData(image, uiMipLevel, ezRectU32(0, 0, image.GetWidth(uiMipLevel), image.GetHeight(uiMipLevel)), uiPaddingBytes, out_storage);
  }

  void BuildInitialData(ezArrayPtr<const ezImage> images, ezUInt32 uiPaddingBytes, ezDynamicArray<ezGALSystemMemoryDescription>& out_initialData,
    ezDynamicArray<ezDynamicArray<ezUInt8>>& out_initialDataStorage)
  {
    out_initialData.Clear();
    out_initialDataStorage.Clear();
    if (images.IsEmpty())
      return;

    const ezUInt32 uiMipLevels = images[0].GetNumMipLevels();
    out_initialData.Reserve(images.GetCount() * uiMipLevels);
    out_initialDataStorage.SetCount(images.GetCount() * uiMipLevels);
    ezUInt32 uiSubresourceIndex = 0;
    for (ezUInt32 s = 0; s < images.GetCount(); ++s)
    {
      EZ_ASSERT_DEV(images[s].GetNumMipLevels() == uiMipLevels, "All layered test images must have the same mip count");
      for (ezUInt32 m = 0; m < uiMipLevels; ++m)
      {
        out_initialData.PushBack(BuildStridedData(images[s], m, uiPaddingBytes, out_initialDataStorage[uiSubresourceIndex]));
        ++uiSubresourceIndex;
      }
    }
  }

  ezGALTextureHandle CreateTexture(ezGALDevice* pDevice, const ezGALTextureCreationDescription& desc, ezArrayPtr<const ezImage> initialImages, ezUInt32 uiPaddingBytes)
  {
    if (initialImages.IsEmpty())
      return pDevice->CreateTexture(desc, {});

    ezDynamicArray<ezGALSystemMemoryDescription> initialData;
    ezDynamicArray<ezDynamicArray<ezUInt8>> initialDataStorage;
    BuildInitialData(initialImages, uiPaddingBytes, initialData, initialDataStorage);
    return pDevice->CreateTexture(desc, initialData.GetArrayPtr());
  }

} // namespace


void ezRendererTestCopyUpdate::SetupSubTests()
{
  const ezGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("01 - CopyUpdateBuffer", SubTests::ST_CopyBuffer);
  AddSubTest("02 - CopyUpdateTexture", SubTests::ST_CopyTexture);
  AddSubTest("03 - CopyUpdateTexture2DArray", SubTests::ST_CopyTextureArray);
  AddSubTest("04 - CopyUpdateTextureCube", SubTests::ST_CopyTextureCube);
  AddSubTest("05 - CopyUpdateTextureCubeArray", SubTests::ST_CopyTextureCubeArray);
  AddSubTest("06 - CopyUpdateTextureNPOT", SubTests::ST_CopyTextureNpot);

  const auto bc1Support = caps.m_FormatSupport[ezGALResourceFormat::BC1];
  const bool bCanCreateBC1 = ezImageConversion::IsConvertible(ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageFormat::BC1_UNORM);
  if (bc1Support.IsSet(ezGALResourceFormatSupport::Texture) && bCanCreateBC1)
  {
    AddSubTest("07 - CopyUpdateTextureBC1", SubTests::ST_CopyTextureBC1);
  }
}

ezResult ezRendererTestCopyUpdate::InitializeTest()
{
  ezStartup::StartupCoreSystems();

  if (SetupRenderer().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezRendererTestCopyUpdate::DeInitializeTest()
{
  ShutdownRenderer();
  ezStartup::ShutdownCoreSystems();
  ezMemoryTracker::DumpMemoryLeaks();
  return EZ_SUCCESS;
}

ezResult ezRendererTestCopyUpdate::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;
  m_BufferReadback.Reset();
  m_TextureReadback.Reset();

  switch (iIdentifier)
  {
    case ST_CopyBuffer:
    {
      // Source: counter pattern, destination: all 0xCC so we can tell which bytes survived.
      FillBufferPattern(m_BufferSourceData, s_uiBufferSize, false);

      ezDynamicArray<ezUInt8> destInitial;
      destInitial.SetCount(s_uiBufferSize, 0xCC);

      m_hBufferSource = CreateTestBuffer(m_pDevice, s_uiBufferSize, m_BufferSourceData.GetByteArrayPtr());
      m_hBufferDest = CreateTestBuffer(m_pDevice, s_uiBufferSize, destInitial.GetByteArrayPtr());
      EZ_TEST_BOOL(!m_hBufferSource.IsInvalidated());
      EZ_TEST_BOOL(!m_hBufferDest.IsInvalidated());
    }
    break;
    case ST_CopyTexture:
      SetupTextureCopyUpdatePair(ezGALTextureType::Texture2D, 1, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTexturePaddingBytes);
      break;
    case ST_CopyTextureNpot:
      SetupTextureCopyUpdatePair(ezGALTextureType::Texture2D, 1, s_uiNpotTextureWidth, s_uiNpotTextureHeight, s_uiTextureMips, s_uiCopyTextureNpotPaddingBytes);
      break;
    case ST_CopyTextureArray:
      SetupTextureCopyUpdatePair(ezGALTextureType::Texture2DArray, 4, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTextureArrayPaddingBytes);
      break;
    case ST_CopyTextureCube:
      SetupTextureCopyUpdatePair(ezGALTextureType::TextureCube, 1, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTextureCubePaddingBytes);
      break;
    case ST_CopyTextureCubeArray:
      SetupTextureCopyUpdatePair(
        ezGALTextureType::TextureCubeArray, 2, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTextureCubeArrayPaddingBytes);
      break;
    case ST_CopyTextureBC1:
      SetupTextureCopyUpdatePair(ezGALTextureType::Texture2D, 1, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTextureBC1PaddingBytes, ezGALResourceFormat::BC1);
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestCopyUpdate::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_BufferReadback.Reset();
  m_TextureReadback.Reset();

  m_pDevice->DestroyBuffer(m_hBufferSource);
  m_pDevice->DestroyBuffer(m_hBufferDest);
  m_pDevice->DestroyTexture(m_hTextureSource);
  m_pDevice->DestroyTexture(m_hTextureDest);

  m_BufferSourceData.Clear();
  m_TextureSourceImages.Clear();
  m_TextureDestImages.Clear();
  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestCopyUpdate::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;

  if (m_iFrame > 1)
    return ezTestAppRun::Quit;

  switch (iIdentifier)
  {
    case ST_CopyBuffer:
      BeginFrame();
      RunCopyBuffer();
      EndFrame();
      break;
    case ST_CopyTexture:
    case ST_CopyTextureNpot:
    case ST_CopyTextureArray:
    case ST_CopyTextureCube:
    case ST_CopyTextureCubeArray:
    case ST_CopyTextureBC1:
      BeginFrame();
      RunCopyTextureRegion(iIdentifier);
      RunCopyTexture();
      RunUpdateTexture(iIdentifier);
      EndFrame();

      RunUpdateTextureForNextFrame(iIdentifier);

      BeginFrame();
      VerifyTextureSliceReadback(m_hTextureDest, m_TextureDestImages.GetArrayPtr());
      EndFrame();
      break;
  }

  return ezTestAppRun::Quit;
}

void ezRendererTestCopyUpdate::RunCopyBuffer()
{
  constexpr ezUInt32 uiRegionSrcOffset = 32;
  constexpr ezUInt32 uiRegionDstOffset = 96;
  constexpr ezUInt32 uiRegionByteCount = 64;

  BeginCommands("CopyBufferRegion");
  TransitionBuffer(m_hBufferSource, ezGALResourceState::CopySource);
  TransitionBuffer(m_hBufferDest, ezGALResourceState::CopyDestination);
  m_pEncoder->CopyBufferRegion(m_hBufferDest, uiRegionDstOffset, m_hBufferSource, uiRegionSrcOffset, uiRegionByteCount);
  EndCommands();

  ezDynamicArray<ezUInt8> expected;
  expected.SetCount(s_uiBufferSize, 0xCC);
  ezMemoryUtils::Copy(expected.GetData() + uiRegionDstOffset, m_BufferSourceData.GetData() + uiRegionSrcOffset, uiRegionByteCount);

  VerifyBufferReadback(m_hBufferDest, expected.GetByteArrayPtr());

  BeginCommands("CopyBuffer");
  TransitionBuffer(m_hBufferSource, ezGALResourceState::CopySource);
  TransitionBuffer(m_hBufferDest, ezGALResourceState::CopyDestination);
  m_pEncoder->CopyBuffer(m_hBufferDest, m_hBufferSource);
  EndCommands();

  VerifyBufferReadback(m_hBufferDest, m_BufferSourceData.GetByteArrayPtr());

  ezDynamicArray<ezUInt8> updateData;
  FillBufferPattern(updateData, s_uiBufferSize, true);
  constexpr ezUInt32 uiUpdateSplitOffset = s_uiBufferSize / 2;

  BeginCommands("UpdateBuffer");
  // AheadOfTime updates must not overlap within the same frame. Two chunks cover the full buffer and exercise a non-zero offset.
  m_pEncoder->UpdateBuffer(m_hBufferDest, 0, updateData.GetArrayPtr().GetSubArray(0, uiUpdateSplitOffset), ezGALUpdateMode::AheadOfTime);
  m_pEncoder->UpdateBuffer(m_hBufferDest, uiUpdateSplitOffset,
    updateData.GetArrayPtr().GetSubArray(uiUpdateSplitOffset, s_uiBufferSize - uiUpdateSplitOffset), ezGALUpdateMode::AheadOfTime);
  EndCommands();

  VerifyBufferReadback(m_hBufferDest, updateData.GetByteArrayPtr());
}

void ezRendererTestCopyUpdate::RunCopyTextureRegion(ezInt32 iIdentifier)
{
  ezUInt32 uiSourceOffset = 1;
  ezUInt32 uiDestinationOffset = 2;
  ezUInt32 uiCopySize = 4;

  ezGALTextureSubresource srcSub{0, 0};
  ezGALTextureSubresource dstSub{0, 0};
  switch (iIdentifier)
  {
    case ST_CopyBuffer:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return;
    case ST_CopyTexture:
    case ST_CopyTextureNpot:
      break;
    case ST_CopyTextureBC1:
      uiSourceOffset = 0;
      uiDestinationOffset = 4;
      break;
    case ST_CopyTextureArray:
      srcSub.m_uiArraySlice = 1;
      dstSub.m_uiArraySlice = 2;
      break;
    case ST_CopyTextureCube:
      dstSub.m_uiArraySlice = 5;
      break;
    case ST_CopyTextureCubeArray:
      srcSub.m_uiArraySlice = 2;
      dstSub.m_uiArraySlice = 9;
      break;
  }

  ezBoundingBoxu32 box;
  box.m_vMin = ezVec3U32(uiSourceOffset, uiSourceOffset, 0);
  box.m_vMax = ezVec3U32(uiSourceOffset + uiCopySize, uiSourceOffset + uiCopySize, 1);
  ezVec3U32 dstPoint(uiDestinationOffset, uiDestinationOffset, 0);

  BeginCommands("CopyTextureRegion");
  TransitionTexture(m_hTextureSource, ezGALResourceState::CopySource);
  TransitionTexture(m_hTextureDest, ezGALResourceState::CopyDestination);
  m_pEncoder->CopyTextureRegion(m_hTextureDest, dstSub, dstPoint, m_hTextureSource, srcSub, box);
  EndCommands();

  const ezRectU32 sourceRect(box.m_vMin.x, box.m_vMin.y, box.m_vMax.x - box.m_vMin.x, box.m_vMax.y - box.m_vMin.y);
  CopyImageRegionBytes(m_TextureSourceImages[srcSub.m_uiArraySlice], srcSub.m_uiMipLevel, sourceRect, m_TextureDestImages[dstSub.m_uiArraySlice], dstSub.m_uiMipLevel, dstPoint);

  VerifyTextureSliceReadback(m_hTextureDest, m_TextureDestImages.GetArrayPtr());
}

void ezRendererTestCopyUpdate::RunCopyTexture()
{
  BeginCommands("CopyTexture");
  TransitionTexture(m_hTextureSource, ezGALResourceState::CopySource);
  TransitionTexture(m_hTextureDest, ezGALResourceState::CopyDestination);
  m_pEncoder->CopyTexture(m_hTextureDest, m_hTextureSource);
  EndCommands();

  for (ezUInt32 s = 0; s < m_TextureDestImages.GetCount(); ++s)
  {
    for (ezUInt32 m = 0; m < m_TextureSourceImages[s].GetNumMipLevels(); ++m)
    {
      const ezRectU32 sourceRect(0, 0, m_TextureSourceImages[s].GetWidth(m), m_TextureSourceImages[s].GetHeight(m));
      CopyImageRegionBytes(m_TextureSourceImages[s], m, sourceRect, m_TextureDestImages[s], m, ezVec3U32(0));
    }
  }

  VerifyTextureSliceReadback(m_hTextureDest, m_TextureDestImages.GetArrayPtr());
}

void ezRendererTestCopyUpdate::RunUpdateTexture(ezInt32 iIdentifier)
{
  ezUInt32 uiMipLevel = 1;
  ezUInt32 uiSourceOffset = 1;
  ezUInt32 uiOffset = 1;
  ezUInt32 uiSize = 2;
  ezUInt32 uiArraySlice = 0;
  ezUInt32 uiPaddingBytes = s_uiCopyTexturePaddingBytes;
  switch (iIdentifier)
  {
    case ST_CopyBuffer:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return;
    case ST_CopyTexture:
      break;
    case ST_CopyTextureNpot:
      uiPaddingBytes = s_uiCopyTextureNpotPaddingBytes;
      break;
    case ST_CopyTextureArray:
      uiArraySlice = 2;
      uiPaddingBytes = s_uiCopyTextureArrayPaddingBytes;
      break;
    case ST_CopyTextureCube:
      uiArraySlice = 5;
      uiPaddingBytes = s_uiCopyTextureCubePaddingBytes;
      break;
    case ST_CopyTextureCubeArray:
      uiArraySlice = 9;
      uiPaddingBytes = s_uiCopyTextureCubeArrayPaddingBytes;
      break;
    case ST_CopyTextureBC1:
      uiMipLevel = 0;
      uiSourceOffset = 0;
      uiOffset = 4;
      uiSize = 4;
      uiPaddingBytes = s_uiCopyTextureBC1PaddingBytes;
      break;
  }

  const ezGALTexture* pTexture = m_pDevice->GetTexture(m_hTextureDest);
  const ezGALTextureCreationDescription& desc = pTexture->GetDescription();

  ezImage updateImage;
  CreateTestImage(updateImage, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, desc.m_Format, static_cast<ezUInt8>(200u + uiArraySlice));

  const ezRectU32 sourceRect(uiSourceOffset, uiSourceOffset, uiSize, uiSize);
  ezDynamicArray<ezUInt8> updateDataStorage;
  ezGALSystemMemoryDescription srcMem = BuildStridedData(updateImage, uiMipLevel, sourceRect, uiPaddingBytes, updateDataStorage);

  ezBoundingBoxu32 destBox;
  destBox.m_vMin = ezVec3U32(uiOffset, uiOffset, 0);
  destBox.m_vMax = ezVec3U32(uiOffset + uiSize, uiOffset + uiSize, 1);

  BeginCommands("UpdateTexture");
  // UpdateTexture goes through the InitContext which manages its own barriers around the upload.
  ezGALTextureSubresource subResource;
  subResource.m_uiMipLevel = uiMipLevel;
  subResource.m_uiArraySlice = uiArraySlice;
  m_pEncoder->UpdateTexture(m_hTextureDest, subResource, destBox, srcMem);
  EndCommands();

  const ezVec3U32 destinationOffset(uiOffset, uiOffset, 0);
  CopyImageRegionBytes(updateImage, uiMipLevel, sourceRect, m_TextureDestImages[uiArraySlice], uiMipLevel, destinationOffset);

  VerifyTextureSliceReadback(m_hTextureDest, m_TextureDestImages.GetArrayPtr());
}

void ezRendererTestCopyUpdate::RunUpdateTextureForNextFrame(ezInt32 iIdentifier)
{
  ezUInt32 uiMipLevel = 1;
  ezUInt32 uiSourceOffset = 0;
  ezUInt32 uiOffset = 0;
  ezUInt32 uiSize = 2;
  ezUInt32 uiArraySlice = 0;
  ezUInt32 uiPaddingBytes = s_uiCopyTexturePaddingBytes;
  switch (iIdentifier)
  {
    case ST_CopyBuffer:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return;
    case ST_CopyTexture:
      break;
    case ST_CopyTextureNpot:
      uiPaddingBytes = s_uiCopyTextureNpotPaddingBytes;
      break;
    case ST_CopyTextureArray:
      uiArraySlice = 2;
      uiPaddingBytes = s_uiCopyTextureArrayPaddingBytes;
      break;
    case ST_CopyTextureCube:
      uiArraySlice = 5;
      uiPaddingBytes = s_uiCopyTextureCubePaddingBytes;
      break;
    case ST_CopyTextureCubeArray:
      uiArraySlice = 9;
      uiPaddingBytes = s_uiCopyTextureCubeArrayPaddingBytes;
      break;
    case ST_CopyTextureBC1:
      uiMipLevel = 0;
      uiSourceOffset = 0;
      uiOffset = 0;
      uiSize = 4;
      uiPaddingBytes = s_uiCopyTextureBC1PaddingBytes;
      break;
  }

  const ezGALTexture* pTexture = m_pDevice->GetTexture(m_hTextureDest);
  const ezGALTextureCreationDescription& desc = pTexture->GetDescription();

  ezImage updateImage;
  CreateTestImage(updateImage, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, desc.m_Format, static_cast<ezUInt8>(220u + uiArraySlice));

  const ezRectU32 sourceRect(uiSourceOffset, uiSourceOffset, uiSize, uiSize);
  ezDynamicArray<ezUInt8> updateDataStorage;
  ezGALSystemMemoryDescription srcMem = BuildStridedData(updateImage, uiMipLevel, sourceRect, uiPaddingBytes, updateDataStorage);

  ezBoundingBoxu32 destBox;
  destBox.m_vMin = ezVec3U32(uiOffset, uiOffset, 0);
  destBox.m_vMax = ezVec3U32(uiOffset + uiSize, uiOffset + uiSize, 1);

  ezGALTextureSubresource subResource;
  subResource.m_uiMipLevel = uiMipLevel;
  subResource.m_uiArraySlice = uiArraySlice;
  m_pDevice->UpdateTextureForNextFrame(m_hTextureDest, srcMem, subResource, destBox);

  const ezVec3U32 destinationOffset(uiOffset, uiOffset, 0);
  CopyImageRegionBytes(updateImage, uiMipLevel, sourceRect, m_TextureDestImages[uiArraySlice], uiMipLevel, destinationOffset);
}

void ezRendererTestCopyUpdate::VerifyBufferReadback(ezGALBufferHandle hBuffer, ezArrayPtr<const ezUInt8> expected)
{
  BeginCommands("ReadbackBuffer");
  TransitionBuffer(hBuffer, ezGALResourceState::CopySource);
  m_BufferReadback.ReadbackBuffer(*m_pEncoder, hBuffer);
  EndCommands();

  ezEnum<ezGALAsyncResult> res = m_BufferReadback.GetReadbackResult(ezTime::MakeFromHours(1));
  if (!EZ_TEST_BOOL_MSG(res == ezGALAsyncResult::Ready, "Buffer readback timed out"))
    return;

  ezArrayPtr<const ezUInt8> memory;
  ezReadbackBufferLock lock = m_BufferReadback.LockBuffer(memory);
  EZ_ASSERT_ALWAYS(lock, "Failed to lock readback buffer");

  if (EZ_TEST_INT(memory.GetCount(), expected.GetCount()))
  {
    EZ_TEST_INT(ezMemoryUtils::Compare(memory.GetPtr(), expected.GetPtr(), memory.GetCount()), 0);
  }
}

void ezRendererTestCopyUpdate::SetupTextureCopyUpdatePair(
  ezGALTextureType::Enum type, ezUInt32 uiArraySize, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiMipLevels, ezUInt32 uiPaddingBytes, ezGALResourceFormat::Enum format)
{
  ezGALTextureCreationDescription desc = CreateTextureDesc(type, uiWidth, uiHeight, uiMipLevels, uiArraySize, format);
  const ezUInt32 uiTotalSlices = GetTotalSlices(desc);

  m_TextureSourceImages.SetCount(uiTotalSlices);
  m_TextureDestImages.SetCount(uiTotalSlices);

  for (ezUInt32 s = 0; s < uiTotalSlices; ++s)
  {
    CreateTestImage(m_TextureSourceImages[s], uiWidth, uiHeight, uiMipLevels, format, static_cast<ezUInt8>(s));
    CreateTestImage(m_TextureDestImages[s], uiWidth, uiHeight, uiMipLevels, format, static_cast<ezUInt8>(100u + s));
  }

  m_hTextureSource = CreateTexture(m_pDevice, desc, m_TextureSourceImages.GetArrayPtr(), uiPaddingBytes);
  m_hTextureDest = CreateTexture(m_pDevice, desc, m_TextureDestImages.GetArrayPtr(), uiPaddingBytes);
  EZ_TEST_BOOL(!m_hTextureSource.IsInvalidated());
  EZ_TEST_BOOL(!m_hTextureDest.IsInvalidated());

  if (!m_hTextureSource.IsInvalidated() && !m_hTextureDest.IsInvalidated())
  {
    const ezGALTexture* pSourceTexture = m_pDevice->GetTexture(m_hTextureSource);
    const ezGALTexture* pDestTexture = m_pDevice->GetTexture(m_hTextureDest);

    for (ezUInt32 uiMipLevel = 0; uiMipLevel < uiMipLevels; ++uiMipLevel)
    {
      const ezVec3U32 vSourceImageSize(m_TextureSourceImages[0].GetWidth(uiMipLevel), m_TextureSourceImages[0].GetHeight(uiMipLevel), 1);
      const ezVec3U32 vDestImageSize(m_TextureDestImages[0].GetWidth(uiMipLevel), m_TextureDestImages[0].GetHeight(uiMipLevel), 1);

      EZ_TEST_BOOL(pSourceTexture->GetMipMapSize(uiMipLevel) == vSourceImageSize);
      EZ_TEST_BOOL(pDestTexture->GetMipMapSize(uiMipLevel) == vDestImageSize);
    }
  }
}

void ezRendererTestCopyUpdate::VerifyTextureSliceReadback(ezGALTextureHandle hTexture, ezArrayPtr<const ezImage> expectedLayers)
{
  BeginCommands("ReadbackTextureLayered");
  TransitionTexture(hTexture, ezGALResourceState::CopySource);
  m_TextureReadback.ReadbackTexture(*m_pEncoder, hTexture);
  EndCommands();

  ezEnum<ezGALAsyncResult> res = m_TextureReadback.GetReadbackResult(ezTime::MakeFromHours(1));
  if (!EZ_TEST_BOOL_MSG(res == ezGALAsyncResult::Ready, "Layered texture readback timed out"))
    return;

  const ezGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  const ezGALTextureCreationDescription& desc = pTexture->GetDescription();
  const ezUInt32 uiTotalSlices = expectedLayers.GetCount();
  EZ_ASSERT_DEV(uiTotalSlices == GetTotalSlices(desc), "Mismatch between expected layer count and texture slice count");
  for (ezUInt32 s = 0; s < uiTotalSlices; ++s)
  {
    EZ_ASSERT_DEV(expectedLayers[s].GetNumMipLevels() == desc.m_uiMipLevelCount, "Mismatch between expected image mip count and texture mip count");
  }

  ezDynamicArray<ezGALTextureSubresource> subs;
  subs.SetCount(uiTotalSlices * desc.m_uiMipLevelCount);
  ezUInt32 uiSubresourceIndex = 0;
  for (ezUInt32 s = 0; s < uiTotalSlices; ++s)
  {
    for (ezUInt32 m = 0; m < desc.m_uiMipLevelCount; ++m)
    {
      subs[uiSubresourceIndex].m_uiMipLevel = m;
      subs[uiSubresourceIndex].m_uiArraySlice = s;
      ++uiSubresourceIndex;
    }
  }

  ezDynamicArray<ezGALSystemMemoryDescription> memory;
  ezReadbackTextureLock lock = m_TextureReadback.LockTexture(subs.GetArrayPtr(), memory);
  EZ_ASSERT_ALWAYS(lock, "Failed to lock layered readback texture");

  uiSubresourceIndex = 0;
  for (ezUInt32 s = 0; s < uiTotalSlices; ++s)
  {
    for (ezUInt32 m = 0; m < desc.m_uiMipLevelCount; ++m)
    {
      const ezUInt32 uiCurrentSubresourceIndex = uiSubresourceIndex++;
      ezImage actual;
      ezTextureUtils::CopySubResourceToImage(desc, subs[uiCurrentSubresourceIndex], memory[uiCurrentSubresourceIndex], actual, false);

      const auto expectedView = expectedLayers[s].GetSubImageView(m);
      if (!EZ_TEST_INT(actual.GetByteBlobPtr().GetCount(), expectedView.GetByteBlobPtr().GetCount()))
        continue;

      const int cmp = ezMemoryUtils::Compare(actual.GetByteBlobPtr().GetPtr(), expectedView.GetByteBlobPtr().GetPtr(), actual.GetByteBlobPtr().GetCount());
      EZ_TEST_INT_MSG(cmp, 0, "Slice {} mismatch after copy", s);
    }
  }
}

static ezRendererTestCopyUpdate g_CopyUpdateTest;
