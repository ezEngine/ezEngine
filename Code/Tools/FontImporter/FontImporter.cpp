#include <FontImporterPCH.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <RendererCore/Font/FontResource.h>
#include <FontImporter.h>
#include <TextureAtlasLayout.h>

void ezFontImporter::Startup()
{
  FT_Error error = FT_Init_FreeType(&m_Library);
  EZ_ASSERT_ALWAYS(!error, "Error occurred during FreeType library initialization.");
}

ezResult ezFontImporter::Import(const ezString& inputFile, const ezFontImportOptions& importOptions, ezRawFont& outFont)
{
  ezOSFile file;
  if (file.Open(inputFile, ezFileOpenMode::Read).Failed())
  {
    ezLog::Error("No file found at path: {}", inputFile);

    return EZ_FAILURE;
  }

  ezDynamicArray<ezUInt8> fileBuffer;

  const ezUInt64 fileSize = file.ReadAll(fileBuffer);

  ezFileStats fontFileInfo;

  ezFileSystem::GetFileStats(inputFile, fontFileInfo);

  ezStringBuilder fontFileName(ezPathUtils::GetFileName(inputFile));

  outFont.m_Name = fontFileName;

  file.Close();

  FT_Face face;

  FT_Error error = FT_New_Memory_Face(m_Library, fileBuffer.GetData(), fileSize, 0, &face);

  if (error == FT_Err_Unknown_File_Format)
  {
    ezLog::Error("Failed to load font file: {}. Unsupported file format.", inputFile);
    return EZ_FAILURE;
  }
  else if (error)
  {
    ezLog::Error("Failed to load font file: {}. Unknown error.", inputFile);
    return EZ_FAILURE;
  }

  outFont.m_FamilyName = face->family_name;

  error = FT_Select_Charmap(face, ft_encoding_unicode);
  if (error)
  {
    ezLog::Error("Failed to set character map");
    return EZ_FAILURE;
  }

  FT_Int32 loadFlags;
  switch (importOptions.m_RenderMode)
  {
    case ezFontRenderMode::Smooth:
      loadFlags = FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_HINTING;
      break;
    case ezFontRenderMode::Raster:
      loadFlags = FT_LOAD_TARGET_MONO | FT_LOAD_NO_HINTING;
      break;
    case ezFontRenderMode::HintedSmooth:
      loadFlags = FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_AUTOHINT;
      break;
    case ezFontRenderMode::HintedRaster:
      loadFlags = FT_LOAD_TARGET_MONO | FT_LOAD_NO_AUTOHINT;
      break;
    default:
      loadFlags = FT_LOAD_TARGET_NORMAL;
      break;
  }

  FT_Render_Mode renderMode = FT_LOAD_TARGET_MODE(loadFlags);

  for (ezUInt32 fontSize : importOptions.m_FontSizes)
  {
    FT_F26Dot6 ftSize = (FT_F26Dot6)(fontSize * (1 << 6));

    error = FT_Set_Char_Size(face, ftSize, 0, importOptions.m_Dpi, importOptions.m_Dpi);
    if (error)
    {
      ezLog::Error("Failed to set character size");
      return EZ_FAILURE;
    }

    ezRawFontBitmap rawFontBitmap;

    // Get all char sizes so we can generate texture layout
    ezDynamicArray<ezTextureAtlasElement> atlasElements;
    ezMap<ezUInt32, ezUInt32> seqIdxToCharIdx;

    for (ezCharacterRange range : importOptions.m_charIndexRanges)
    {
      for (ezUInt32 charIdx = range.m_Start; charIdx <= range.m_End; charIdx++)
      {
        error = FT_Load_Char(face, (FT_ULong)charIdx, loadFlags);
        if (error)
        {
          ezLog::Error("Failed to load a character");
          return EZ_FAILURE;
        }

        error = FT_Render_Glyph(face->glyph, renderMode);
        if (error)
        {
          ezLog::Error("Failed to render a character");
          return EZ_FAILURE;
        }

        FT_GlyphSlot slot = face->glyph;

        ezTextureAtlasElement atlasElement;
        atlasElement.Input.Width = slot->bitmap.width;
        atlasElement.Input.Height = slot->bitmap.rows;

        atlasElements.PushBack(atlasElement);

        seqIdxToCharIdx[atlasElements.GetCount() - 1] = charIdx;
      }
    }

    // Add missing glyph
    {
      error = FT_Load_Glyph(face, (FT_ULong)0, loadFlags);
      if (error)
      {
        ezLog::Error("Failed to load a character");
        return EZ_FAILURE;
      }

      error = FT_Render_Glyph(face->glyph, renderMode);
      if (error)
      {
        ezLog::Error("Failed to render a character");
        return EZ_FAILURE;
      }

      FT_GlyphSlot slot = face->glyph;

      ezTextureAtlasElement atlasElement;
      atlasElement.Input.Width = slot->bitmap.width;
      atlasElement.Input.Height = slot->bitmap.rows;

      atlasElements.PushBack(atlasElement);
    }

    ezInt32 baselineOffset = 0;
    ezUInt32 lineHeight = 0;
    ezUInt32 pageIndex = 0;

    // Create an optimal layout for character bitmaps
    ezDynamicArray<ezTextureAtlasPage> pages = ezTextureAtlasUtility::CreateTextureAtlasLayout(atlasElements, 64, 64, sMaximumTextureSize, sMaximumTextureSize, true);

    // Create char bitmap atlas textures and load character information
    for (ezTextureAtlasPage& page : pages)
    {
      ezUInt32 bufferSize = page.Width * page.Height;

      ezBlob pixelStore;
      pixelStore.SetCountUninitialized(bufferSize);
      pixelStore.ZeroFill();

      ezUInt8* pixelBuffer = pixelStore.GetBlobPtr<ezUInt8>().GetPtr();

      for (ezUInt32 elementIterIndex = 0; elementIterIndex < atlasElements.GetCount(); elementIterIndex++)
      {
        if (atlasElements[elementIterIndex].Output.Page != pageIndex)
          continue;
        ezTextureAtlasElement currentElement = atlasElements[elementIterIndex];
        ezUInt32 currentElementIndex = currentElement.Output.Index;

        bool isMissingGlyph = currentElementIndex == (atlasElements.GetCount() - 1); // It's always the last element

        ezUInt32 characterIndex = 0;

        if (!isMissingGlyph)
        {
          characterIndex = seqIdxToCharIdx[currentElementIndex];

          error = FT_Load_Char(face, characterIndex, loadFlags);
        }
        else
        {
          error = FT_Load_Glyph(face, 0, loadFlags);
        }
        if (error)
        {
          ezLog::Error("Failed to load a character");
          return EZ_FAILURE;
        }

        error = FT_Render_Glyph(face->glyph, renderMode);
        if (error)
        {
          ezLog::Error("Failed to render a character");
          return EZ_FAILURE;
        }

        FT_GlyphSlot slot = face->glyph;

        if (slot->bitmap.buffer == nullptr && slot->bitmap.rows > 0 && slot->bitmap.width > 0)
        {
          ezLog::Error("Failed to render glyph bitmap");
          return EZ_FAILURE;
        }

        ezUInt8* sourceBuffer = slot->bitmap.buffer;
        ezUInt8* destBuffer = pixelBuffer + (currentElement.Output.y * page.Width) + currentElement.Output.x;

        if (slot->bitmap.pixel_mode == ft_pixel_mode_grays)
        {
          for (ezUInt32 bitmapRow = 0; bitmapRow < slot->bitmap.rows; bitmapRow++)
          {
            for (ezUInt32 bitmapColumn = 0; bitmapColumn < slot->bitmap.width; bitmapColumn++)
            {
              destBuffer[bitmapColumn] = sourceBuffer[bitmapColumn];
              //destBuffer[bitmapColumn + 1] = sourceBuffer[bitmapColumn];
            }

            destBuffer += page.Width;
            sourceBuffer += slot->bitmap.pitch;
          }
        }
        else if (slot->bitmap.pixel_mode == ft_pixel_mode_mono)
        {
          // 8 pixels are packed into a byte, so do some unpacking
          for (ezUInt32 bitmapRow = 0; bitmapRow < slot->bitmap.rows; bitmapRow++)
          {
            for (ezUInt32 bitmapColumn = 0; bitmapColumn < slot->bitmap.width; bitmapColumn++)
            {
              ezUInt8 srcValue = sourceBuffer[bitmapColumn >> 3];
              ezUInt8 dstValue = (srcValue & (128 >> (bitmapColumn & 7))) != 0 ? 255 : 0;

              destBuffer[bitmapColumn] = dstValue;
              //destBuffer[bitmapColumn + 1] = dstValue;
            }

            destBuffer += page.Width;
            sourceBuffer += slot->bitmap.pitch;
          }
        }
        else
        {
          ezLog::Error("Unsupported pixel mode for a FreeType bitmap.");
          return EZ_FAILURE;
        }
        // Store character information
        ezFontGlyph fontGlyph;

        float invTexWidth = 1.0f / page.Width;
        float invTexHeight = 1.0f / page.Height;

        fontGlyph.m_CharacterId = characterIndex;
        fontGlyph.m_Width = currentElement.Input.Width;
        fontGlyph.m_Height = currentElement.Input.Height;
        fontGlyph.m_Page = currentElement.Output.Page;
        fontGlyph.m_UVWidth = invTexWidth * currentElement.Input.Width;
        fontGlyph.m_UVHeight = invTexHeight * currentElement.Input.Height;
        fontGlyph.m_UVX = invTexWidth * currentElement.Output.x;
        fontGlyph.m_UVY = invTexHeight * currentElement.Output.y;
        fontGlyph.m_XOffset = slot->bitmap_left;
        fontGlyph.m_YOffset = slot->bitmap_top;
        fontGlyph.m_XAdvance = slot->advance.x >> 6;
        fontGlyph.m_YAdvance = slot->advance.y >> 6;

        baselineOffset = ezMath::Max(baselineOffset, (ezInt32)(slot->metrics.horiBearingY >> 6));
        lineHeight = ezMath::Max(lineHeight, fontGlyph.m_Height);

        // Load kerning and store char
        if (!isMissingGlyph)
        {
          FT_Vector resultKerning;
          for (ezUInt32 charIndexRangeIter = 0; charIndexRangeIter < importOptions.m_charIndexRanges.GetCount(); charIndexRangeIter++)
          {
            auto kerningIter = importOptions.m_charIndexRanges[charIndexRangeIter];
            for (ezUInt32 kerningCharIdx = kerningIter.m_Start; kerningCharIdx <= kerningIter.m_End; kerningCharIdx++)
            {
              if (kerningCharIdx == characterIndex)
                continue;

              error = FT_Get_Kerning(face, characterIndex, kerningCharIdx, FT_KERNING_DEFAULT, &resultKerning);
              if (error)
              {
                ezLog::Error("Failed to get kerning information for character: {0}", characterIndex);
                return EZ_FAILURE;
              }

              ezInt32 kerningX = (ezInt32)(resultKerning.x >> 6); // Y kerning is ignored because it is so rare
              if (kerningX == 0)                                  // We don't store 0 kerning, this is assumed default
                continue;

              ezKerningPair pair;
              pair.m_Amount = kerningX;
              pair.m_OtherCharacterId = kerningCharIdx;

              fontGlyph.m_KerningPairs.PushBack(pair);
            }
          }

          rawFontBitmap.m_Characters[characterIndex] = fontGlyph;
        }
        else
        {
          rawFontBitmap.m_MissingGlyph = fontGlyph;
        }
      }

      ezImageHeader imgHeader;
      imgHeader.SetWidth(page.Width);
      imgHeader.SetHeight(page.Height);
      imgHeader.SetImageFormat(ezImageFormat::Enum::R8_UNORM);

      ezImage& outImg = rawFontBitmap.m_Textures.ExpandAndGetRef();
      outImg.ResetAndAlloc(imgHeader);

      auto pixelData = outImg.GetBlobPtr<ezUInt8>();
      ezMemoryUtils::ZeroFill(pixelData.GetPtr(), pixelData.GetCount());
      ezMemoryUtils::Copy(pixelData.GetPtr(), pixelBuffer, bufferSize);

      {
        ezStringBuilder fontPageName;
        fontPageName.Format("{0}_{1}_{2}", fontFileName, fontSize, pageIndex);

        ezStringBuilder fontPagePath(inputFile);
        fontPagePath.Append(fontPageName.GetData());
        ezStringBuilder fontImagePath(fontPagePath);
        fontImagePath.Append(".png");

        ezStringBuilder fontImageAbsPath;

        ezResult resolved = ezFileSystem::ResolvePath(fontImagePath, &fontImageAbsPath, nullptr);

        ezFileWriter file;

        file.Open(fontImageAbsPath);

        auto ezResult = ezImageFileFormat::GetWriterFormat("png")->WriteImage(file, outImg, ezLog::GetThreadLocalLogSystem(), "png");

        EZ_ASSERT_ALWAYS(ezResult == EZ_SUCCESS, "Failed to write file.");

        file.Close();
      }

      pageIndex++;
    }

    rawFontBitmap.m_Size = fontSize;
    rawFontBitmap.m_BaselineOffset = baselineOffset;
    rawFontBitmap.m_LineHeight = lineHeight;

    // Get space size
    error = FT_Load_Char(face, 32, loadFlags);
    if (error)
    {
      ezLog::Error("Failed to load a character");
      return EZ_FAILURE;
    }

    rawFontBitmap.m_SpaceWidth = face->glyph->advance.x >> 6;

    outFont.m_BitmapPerSize.Insert(rawFontBitmap.m_Size, std::move(rawFontBitmap));
  }

  return EZ_SUCCESS;
}

void ezFontImporter::Shutdown()
{
  FT_Done_FreeType(m_Library);
}
