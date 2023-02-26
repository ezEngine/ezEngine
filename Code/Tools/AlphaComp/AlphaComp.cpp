#include <AlphaComp/AlphaCompPCH.h>

#include <AlphaComp/AlphaComp.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/ezTexFormat/ezTexFormat.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Texture/Image/Image.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/IO/MemoryStream.h>

//#define DO_SANITY_CHECKS

ezCommandLineOptionPath opt_InputFile("_AlphaComp", "-in", "Input path", "", "");
ezCommandLineOptionPath opt_OutputFile("_AlphaComp", "-out", "OutputPath", "", "");

ezAlphaComp::ezAlphaComp()
  : ezApplication("TexConv")
{
}

ezResult ezAlphaComp::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("texconv");

  return SUPER::BeforeCoreSystemsStartup();
}

void ezAlphaComp::AfterCoreSystemsStartup()
{
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites).IgnoreResult();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

void ezAlphaComp::BeforeCoreSystemsShutdown()
{
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

struct BitWriter
{
  ezDynamicArray<ezUInt32> m_data;
  ezUInt32 m_bitsUnused = 0;

  void Write(ezUInt32 value, ezUInt32 bits)
  {
    EZ_ASSERT_DEBUG(bits == 1, "currently only one bit is supported");
    if(m_bitsUnused == 0)
    {
      m_data.PushBack(0);
      m_bitsUnused = 32;
    }

    value = value << (32 - m_bitsUnused);
    m_data.PeekBack() |= value;
    m_bitsUnused -= 1;

    /*EZ_ASSERT_DEBUG(bits >= 0, "Need to write at least one bit");
    ezUInt32 numAdditionalBytesRequired = (m_bitsUnused >= bits) ? 0 : (bits - m_bitsUnused - 1) / 8 + 1;
    ezUInt32 startingSize = m_data.GetCount();
    if (numAdditionalBytesRequired > 0)
    {
      m_data.SetCount(startingSize + numAdditionalBytesRequired);
    }

    if(m_bitsUnused == 0)
    {
      ezUInt32 numBytesToCopy = (bits - 1) / 8 + 1;
      memcpy(m_data.GetData() + startingSize, &value, numBytesToCopy);
      m_bitsUnused = 8 - (bits % 8);
    }
    else
    {
      ezUInt8* outPtr = m_data.GetData() + (startingSize - 1);
      while(bits > 0)
      {
        if(m_bitsUnused == 0)
        {
          m_bitsUnused = 8;
        }
        ezUInt8 valueToAdd = ((1 << m_bitsUnused) - 1) & value;
        *outPtr |= (valueToAdd << (m_bitsUnused - 1));
        value = value >> m_bitsUnused;
        outPtr++;
        if(m_bitsUnused > bits)
        {
          m_bitsUnused -= bits;
          bits = 0;
        }
        else
        {
          bits -= m_bitsUnused;
          m_bitsUnused = 0;
        }
      }
    }*/
  }
};

ezApplication::Execution ezAlphaComp::Run()
{
  SetReturnCode(-1);

  ezString input = opt_InputFile.GetOptionValue(ezCommandLineOption::LogMode::Always);
  ezString output = opt_OutputFile.GetOptionValue(ezCommandLineOption::LogMode::Always);
  ezStringBuilder binFile = output;
  binFile.ChangeFileExtension("bin");


  ezImage inputImage;
  if(inputImage.LoadFrom(input.GetData()).Failed())
  {
    ezLog::Error("Failed to load input image form {}", input);
    SetReturnCode(-1);
    return ezApplication::Execution::Quit;
  }

  /*ezImageHeader debugHeader;
  debugHeader.SetWidth(471);
  debugHeader.SetHeight(471);
  debugHeader.SetImageFormat(ezImageFormat::R32_FLOAT);

  inputImage.ResetAndAlloc(debugHeader);
  float* debugValue = inputImage.GetPixelPointer<float>(0, 0, 0, 0, 0, 0);
  for(ezUInt32 y =0; y < debugHeader.GetHeight(); ++y)
  {
    for(ezUInt32 x = 0; x < debugHeader.GetWidth(); ++x)
    {
      //if((x >= 32 && y < 32) || (x < 32 && y >= 32))
      //{
      //  *debugValue = 1.0f;
      //}
      float normalizedX = (static_cast<float>(x) / debugHeader.GetWidth()) * 2.0f - 1.0f;
      float normalizedY = (static_cast<float>(y) / debugHeader.GetHeight()) * 2.0f - 1.0f;
      if(ezMath::Sqrt(normalizedX * normalizedX + normalizedY * normalizedY) < 1.0f)
      {
        *debugValue = 1.0f;
      }
      ++debugValue;
    }
  }

  ezStringBuilder debugImg = input;
  debugImg.RemoveFileExtension();
  debugImg.Append("_debug.dds");
  inputImage.SaveTo(debugImg.GetData()).IgnoreResult();*/

  ezImageHeader inputHeader = inputImage.GetHeader();

  /*
  // TO RGB png image
  ezImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(ezImageFormat::R8G8B8_UNORM);
  outputHeader.SetWidth(inputHeader.GetWidth() / 3);

  ezImage outputImage;
  outputImage.ResetAndAlloc(outputHeader);


  for(ezUInt32 y = 0; y < inputHeader.GetHeight(); ++y)
  {
    const float* inputRow = inputImage.GetPixelPointer<float>(0, 0, 0, 0, y, 0);
    ezUInt8* outputRow = outputImage.GetPixelPointer<ezUInt8>(0, 0, 0, 0, y, 0);
    for(ezUInt32 x=0; x < inputHeader.GetWidth(); ++x, ++inputRow, ++outputRow)
    {
      if(*inputRow > 0.0f)
      {
        *outputRow = 0x00;
      }
      else
      {
        *outputRow = 0xFF;
      }
    }
  }

  if(outputImage.SaveTo(output.GetData()).Failed())
  {
    ezLog::Error("Failed to write output image to {}", output);
    SetReturnCode(-1);
    return ezApplication::Execution::Quit;
  }*/

  constexpr ezUInt32 blockSize = 32;

  ezStopwatch watch;

  ezUInt32 numFullBlocksX = inputHeader.GetWidth() / blockSize;
  ezUInt32 numFullBlocksY = inputHeader.GetHeight() / blockSize;
  bool partialBlockX = (inputHeader.GetWidth() % blockSize) != 0;
  bool partialBlockY = (inputHeader.GetHeight() % blockSize) != 0;

  BitWriter writer;

#ifdef DO_SANITY_CHECKS
  ezUInt32 numOnPixels = 0;
#endif
  for (ezUInt32 blockY = 0; blockY < numFullBlocksY; blockY++)
  {
    for (ezUInt32 blockX = 0; blockX < numFullBlocksX; blockX++)
    {
      const ezUInt32 blockStartX = blockX * blockSize;
      const ezUInt32 blockStartY = blockY * blockSize;
      bool blockHasEdge = false;
      for(ezUInt32 y=0; y < blockSize; y++)
      {
        const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
        for(ezUInt32 x=0; x < blockSize; ++x, ++inputValue)
        {
          if(*inputValue > 0.0f)
          {
            blockHasEdge = true;
            goto hasEdgeEnd;
          }
        }
      }
      hasEdgeEnd:
      writer.Write(blockHasEdge ? 1u : 0u, 1);

      if(blockHasEdge)
      {
        for (ezUInt32 y = 0; y < blockSize; y++)
        {
          const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
          for (ezUInt32 x = 0; x < blockSize; ++x, ++inputValue)
          {
            writer.Write(*inputValue > 0.0f ? 1u : 0u, 1);
#ifdef DO_SANITY_CHECKS
            if(*inputValue > 0.0f)
            {
              numOnPixels += 1;
            }
#endif
          }
        }
      }
    }
  }

  if(partialBlockX)
  {
    const ezUInt32 blockStartX = numFullBlocksX * blockSize;
    const ezUInt32 remainingBlockSizeX = inputImage.GetWidth() - blockStartX;
    for(ezUInt32 blockY = 0; blockY < numFullBlocksY; blockY++)
    {
      const ezUInt32 blockStartY = blockY * blockSize;
      bool blockHasEdge = false;
      for (ezUInt32 y = 0; y < blockSize; y++)
      {
        const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
        for (ezUInt32 x = 0; x < remainingBlockSizeX; ++x, ++inputValue)
        {
          if (*inputValue > 0.0f)
          {
            blockHasEdge = true;
            goto hasEdgeEnd2;
          }
        }
      }
    hasEdgeEnd2:
      writer.Write(blockHasEdge ? 1u : 0u, 1);

      if (blockHasEdge)
      {
        for (ezUInt32 y = 0; y < blockSize; y++)
        {
          const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
          for (ezUInt32 x = 0; x < remainingBlockSizeX; ++x, ++inputValue)
          {
            writer.Write(*inputValue > 0.0f ? 1u : 0u, 1);
#ifdef DO_SANITY_CHECKS
            if (*inputValue > 0.0f)
            {
              numOnPixels += 1;
            }
#endif
          }
        }
      }
    }
  }

  if (partialBlockY)
  {
    const ezUInt32 blockStartY = numFullBlocksY * blockSize;
    const ezUInt32 remainingBlockSizeY = inputImage.GetHeight() - blockStartY;
    for (ezUInt32 blockX = 0; blockX < numFullBlocksX; blockX++)
    {
      const ezUInt32 blockStartX = blockX * blockSize;
      bool blockHasEdge = false;
      for (ezUInt32 y = 0; y < remainingBlockSizeY; y++)
      {
        const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
        for (ezUInt32 x = 0; x < blockSize; ++x, ++inputValue)
        {
          if (*inputValue > 0.0f)
          {
            blockHasEdge = true;
            goto hasEdgeEnd3;
          }
        }
      }
    hasEdgeEnd3:
      writer.Write(blockHasEdge ? 1u : 0u, 1);

      if (blockHasEdge)
      {
        for (ezUInt32 y = 0; y < remainingBlockSizeY; y++)
        {
          const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
          for (ezUInt32 x = 0; x < blockSize; ++x, ++inputValue)
          {
            writer.Write(*inputValue > 0.0f ? 1u : 0u, 1);
#ifdef DO_SANITY_CHECKS
            if (*inputValue > 0.0f)
            {
              numOnPixels += 1;
            }
#endif
          }
        }
      }
    }
  }

  if(partialBlockX && partialBlockY)
  {
    const ezUInt32 blockStartX = numFullBlocksX * blockSize;
    const ezUInt32 remainingBlockSizeX = inputImage.GetWidth() - blockStartX;
    const ezUInt32 blockStartY = numFullBlocksY * blockSize;
    const ezUInt32 remainingBlockSizeY = inputImage.GetHeight() - blockStartY;

    bool blockHasEdge = false;
    for (ezUInt32 y = 0; y < remainingBlockSizeY; y++)
    {
      const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
      for (ezUInt32 x = 0; x < remainingBlockSizeX; ++x, ++inputValue)
      {
        if (*inputValue > 0.0f)
        {
          blockHasEdge = true;
          goto hasEdgeEnd4;
        }
      }
    }
  hasEdgeEnd4:
    writer.Write(blockHasEdge ? 1u : 0u, 1);

    if (blockHasEdge)
    {
      for (ezUInt32 y = 0; y < remainingBlockSizeY; y++)
      {
        const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
        for (ezUInt32 x = 0; x < remainingBlockSizeX; ++x, ++inputValue)
        {
          writer.Write(*inputValue > 0.0f ? 1u : 0u, 1);
#ifdef DO_SANITY_CHECKS
          if (*inputValue > 0.0f)
          {
            numOnPixels += 1;
          }
#endif
        }
      }
    }
  }

  auto step1 = watch.GetRunningTotal();

#ifdef DO_SANITY_CHECKS
  ezUInt32 numOnBits = 0;
  for(auto& b : writer.m_data)
  {
    for(ezUInt8 v = 1, x = 0; x < 8; x++, v <<= 1)
    {
      if(b & v)
      {
        numOnBits += 1;
      }
    }
  }

  ezLog::Info("NumOnPixels {} NumOnBits {}", numOnPixels, numOnBits);
  if(numOnBits < numOnPixels)
  {
    ezLog::Error("numOnBits is incorrect!");
  }
#endif

  ezDynamicArray<ezUInt8> compressedMemory;
  compressedMemory.SetCountUninitialized(writer.m_data.GetCount() * sizeof(ezUInt32));
  ezRawMemoryStreamWriter memoryStreamWriter(compressedMemory.GetData(), compressedMemory.GetCount());

  ezCompressedStreamWriterZstd compressedWriter;
  compressedWriter.SetOutputStream(&memoryStreamWriter, ezCompressedStreamWriterZstd::Default);

  if(compressedWriter.WriteBytes(writer.m_data.GetData(), writer.m_data.GetCount() * sizeof(ezUInt32)).Failed())
  {
    ezLog::Error("Failed to write {} worth of data", writer.m_data.GetCount() * sizeof(ezUInt32));
    SetReturnCode(-1);
    return ezApplication::Execution::Quit;
  }
  compressedWriter.FinishCompressedStream().IgnoreResult();

  auto step2 = watch.GetRunningTotal();

  ezLog::Info("Pre zstd size {}", writer.m_data.GetCount() * sizeof(ezUInt32));
  ezLog::Info("Post zstd size {}", memoryStreamWriter.GetNumWrittenBytes());

  ezLog::Info("Time for compression {} ({} {})", ezArgF(step1.GetMilliseconds() + step2.GetMilliseconds(), 4), ezArgF(step1.GetMilliseconds(), 4), ezArgF(step2.GetMilliseconds(), 4));

  ezFileWriter fileWriter;

  if (fileWriter.Open(binFile.GetData()).Failed())
  {
    ezLog::Error("Failed to open output file {}", output);
    SetReturnCode(-1);
    return ezApplication::Execution::Quit;
  }
  EZ_ASSERT_DEV(compressedMemory.GetCount() >= memoryStreamWriter.GetNumWrittenBytes(), "out of bounds write");
  fileWriter.WriteBytes(compressedMemory.GetData(), memoryStreamWriter.GetNumWrittenBytes()).IgnoreResult();
  fileWriter.Close();

  ezDynamicArray<ezUInt8> uncompressedMemory;
  uncompressedMemory.SetCountUninitialized(writer.m_data.GetCount() * sizeof(ezUInt32));

  watch.StopAndReset();
  watch.Resume();

  ezRawMemoryStreamReader rawReader(compressedMemory.GetData(), memoryStreamWriter.GetNumWrittenBytes());
  ezCompressedStreamReaderZstd decompressor(&rawReader);
  auto bytesRead = decompressor.ReadBytes(uncompressedMemory.GetData(), uncompressedMemory.GetCount());
  EZ_ASSERT_DEV(bytesRead == uncompressedMemory.GetCount(), "decompression failed");

  ezUInt32* pCur = (ezUInt32*)uncompressedMemory.GetData();
  const ezUInt32* pStart = pCur;
  const ezUInt32* pEnd = (ezUInt32*)(uncompressedMemory.GetData() + uncompressedMemory.GetCount());
  ezUInt32 curValue = 0;
  ezUInt32 bitsLeft = 0;

  ezDynamicArray<ezUInt32> decompressedImage;
  ezUInt32 decompressedImageRowPitch = (inputHeader.GetWidth() - 1) / 32 + 1;
  decompressedImage.SetCount(decompressedImageRowPitch * inputHeader.GetHeight());

  for(ezUInt32 blockY = 0;blockY < numFullBlocksY; blockY++)
  {
    for(ezUInt32 blockX = 0; blockX < numFullBlocksX; blockX++)
    {
      if (bitsLeft == 0)
      {
        bitsLeft = 32;
        curValue = *pCur;
        ++pCur;
      }
      bool blockHasEdge = curValue & 1;
      bitsLeft--;
      curValue >>= 1;
      if(blockHasEdge)
      {
        if(bitsLeft == 32)
        {
          for (ezUInt32 y = 0; y < blockSize; y++)
          {
            ezUInt32* dst = decompressedImage.GetData() + (blockY * blockSize + y) * decompressedImageRowPitch + blockX * (blockSize / 32);
            for (ezUInt32 x = 0; x < blockSize; x += 32, ++dst, ++pCur)
            {
              *dst = curValue;
              curValue = *pCur;
            }
          }
        }
        else
        {
          for (ezUInt32 y = 0; y < blockSize; y++)
          {
            ezUInt32* dst = decompressedImage.GetData() + (blockY * blockSize + y) * decompressedImageRowPitch + blockX * (blockSize / 32);
            for (ezUInt32 x = 0; x < blockSize; x += 32, ++dst, ++pCur)
            {
              ezUInt32 nextValue = *pCur;
              *dst = curValue | (nextValue << bitsLeft);
              curValue = nextValue >> (32 - bitsLeft);
            }
          }
        }
      }
    }
  }

  if (partialBlockX)
  {
    const ezUInt32 blockStartX = numFullBlocksX * blockSize;
    const ezUInt32 remainingBlockSizeX = inputImage.GetWidth() - blockStartX;
    const ezUInt32 tailReads = remainingBlockSizeX % 32;
    const ezUInt32 remainingFullReads = remainingBlockSizeX - tailReads;

    for (ezUInt32 blockY = 0; blockY < numFullBlocksY; blockY++)
    {
      if (bitsLeft == 0)
      {
        bitsLeft = 32;
        curValue = *pCur;
        ++pCur;
      }
      bool blockHasEdge = curValue & 1;
      bitsLeft--;
      curValue >>= 1;
      if (blockHasEdge)
      {
        for (ezUInt32 y = 0; y < blockSize; y++)
        {
          if(bitsLeft == 0)
          {
            curValue = *pCur;
            ++pCur;
            bitsLeft = 32;
          }
          ezUInt32* dst = decompressedImage.GetData() + (blockY * blockSize + y) * decompressedImageRowPitch + numFullBlocksX * (blockSize / 32);
          if(bitsLeft == 32)
          {
            for (ezUInt32 x = 0; x < remainingFullReads; x += 32, ++dst, ++pCur)
            {
              *dst = curValue;
              curValue = *pCur;
            }
          }
          else
          {
            for (ezUInt32 x = 0; x < remainingFullReads; x += 32, ++dst, ++pCur)
            {
              ezUInt32 nextValue = *pCur;
              *dst = curValue | (nextValue << bitsLeft);
              curValue = nextValue >> (32 - bitsLeft);
            }
          }
          if (tailReads > 0)
          {
            if (tailReads > bitsLeft)
            {
              ezUInt32 nextValue = *pCur;
              ezUInt32 nextMask = ((1 << (tailReads - bitsLeft)) - 1);
              *dst = curValue | ((nextValue & nextMask) << bitsLeft);
              curValue = nextValue >> (tailReads - bitsLeft);
              bitsLeft = bitsLeft + 32 - tailReads;
              ++pCur;
            }
            else
            {
              *dst = curValue & ((1 << tailReads) - 1);
              curValue >>= tailReads;
              bitsLeft -= tailReads;
            }
            ++dst;
          }
        }
      }
    }
  }

  if (partialBlockY)
  {
    const ezUInt32 blockStartY = numFullBlocksY * blockSize;
    const ezUInt32 remainingBlockSizeY = inputImage.GetHeight() - blockStartY;

    for (ezUInt32 blockX = 0; blockX < numFullBlocksX; blockX++)
    {
      if (bitsLeft == 0)
      {
        bitsLeft = 32;
        curValue = *pCur;
        ++pCur;
      }
      bool blockHasEdge = curValue & 1;
      bitsLeft--;
      curValue >>= 1;
      if (blockHasEdge)
      {
        if(bitsLeft == 32)
        {
          for (ezUInt32 y = 0; y < remainingBlockSizeY; y++)
          {
            ezUInt32* dst = decompressedImage.GetData() + (numFullBlocksY * blockSize + y) * decompressedImageRowPitch + blockX * (blockSize / 32);
            for (ezUInt32 x = 0; x < blockSize; x += 32, ++dst, ++pCur)
            {
              *dst = curValue;
              curValue = *pCur;
            }
          }
        }
        else
        {
          for (ezUInt32 y = 0; y < remainingBlockSizeY; y++)
          {
            ezUInt32* dst = decompressedImage.GetData() + (numFullBlocksY * blockSize + y) * decompressedImageRowPitch + blockX * (blockSize / 32);
            for (ezUInt32 x = 0; x < blockSize; x += 32, ++dst, ++pCur)
            {
              ezUInt32 nextValue = *pCur;
              *dst = curValue | (nextValue << bitsLeft);
              curValue = nextValue >> (32 - bitsLeft);
            }
          }
        }
      }
    }
  }

  if(partialBlockX && partialBlockY)
  {
    const ezUInt32 blockStartX = numFullBlocksX * blockSize;
    const ezUInt32 remainingBlockSizeX = inputImage.GetWidth() - blockStartX;
    const ezUInt32 tailReadsX = remainingBlockSizeX % 32;
    const ezUInt32 remainingFullReadsX = remainingBlockSizeX - tailReadsX;

    const ezUInt32 blockStartY = numFullBlocksY * blockSize;
    const ezUInt32 remainingBlockSizeY = inputImage.GetHeight() - blockStartY;

    if (bitsLeft == 0)
    {
      bitsLeft = 32;
      curValue = *pCur;
      ++pCur;
    }
    bool blockHasEdge = curValue & 1;
    bitsLeft--;
    curValue >>= 1;
    if (blockHasEdge)
    {
      for (ezUInt32 y = 0; y < remainingBlockSizeY; y++)
      {
        if (bitsLeft == 0)
        {
          curValue = *pCur;
          ++pCur;
          bitsLeft = 32;
        }
        ezUInt32* dst = decompressedImage.GetData() + (blockStartY + y) * decompressedImageRowPitch + numFullBlocksX * (blockSize / 32);
        if(bitsLeft == 32)
        {
          for (ezUInt32 x = 0; x < remainingFullReadsX; x += 32, ++dst, ++pCur)
          {
            *dst = curValue;
            curValue = *pCur;
          }
        }
        else
        {
          for (ezUInt32 x = 0; x < remainingFullReadsX; x += 32, ++dst, ++pCur)
          {
            ezUInt32 nextValue = *pCur;
            *dst = curValue | (nextValue << bitsLeft);
            curValue = nextValue >> (32 - bitsLeft);
          }
        }
        if (tailReadsX > 0)
        {
          if (tailReadsX > bitsLeft)
          {
            ezUInt32 nextValue = *pCur;
            ezUInt32 nextMask = ((1 << (tailReadsX - bitsLeft)) - 1);
            *dst = curValue | ((nextValue & nextMask) << bitsLeft);
            curValue = nextValue >> (tailReadsX - bitsLeft);
            bitsLeft = bitsLeft + 32 - tailReadsX;
            ++pCur;
          }
          else
          {
            *dst = curValue & ((1 << tailReadsX) - 1);
            curValue >>= tailReadsX;
            bitsLeft -= tailReadsX;
          }
          ++dst;
        }
      }
    }
  }

  auto decompressEnd = watch.GetRunningTotal();
  ezLog::Info("Decompression took {}", ezArgF(decompressEnd.GetMilliseconds(), 4));

  ezImageHeader testHeader = inputHeader;
  testHeader.SetImageFormat(ezImageFormat::R8G8B8_UNORM);

  ezImage testImage;
  testImage.ResetAndAlloc(testHeader);

  for(ezUInt32 y=0; y < testHeader.GetHeight(); ++y)
  {
    const ezUInt32* src = decompressedImage.GetData() + (y * decompressedImageRowPitch);
    ezUInt8* outputRow = testImage.GetPixelPointer<ezUInt8>(0, 0, 0, 0, y, 0);
    for(ezUInt32 x=0; x < testHeader.GetWidth(); ++x, outputRow += 3)
    {
      ezUInt32 value = src[x / 32];
      if (value & (1 << (x % 32)))
      {
        outputRow[0] = 0xFF;
        outputRow[1] = 0xFF;
        outputRow[2] = 0xFF;
      }
      else
      {
        outputRow[0] = 0x0;
        outputRow[1] = 0x0;
        outputRow[2] = 0x0;
      }
    }
  }

  testImage.SaveTo(output.GetData()).IgnoreResult();

  SetReturnCode(0);
  return ezApplication::Execution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezAlphaComp);
