#include <RendererCorePCH.h>
#include <RendererCore/Font/TextData.h>

const int EZ_SPACE_CHAR = 32;
const int EZ_TAB_CHAR = 9;

void ezTextData::ezTextWord::Initialize(bool spacer)
{
  m_Width = m_Height = 0;
  m_Spacer = spacer;
  m_SpaceWidth = 0;
  m_CharsStart = 0;
  m_CharsEnd = 0;
  m_LastChar = nullptr;
}

ezUInt32 ezTextData::ezTextWord::AddCharacter(ezUInt32 charIndex, const ezFontGlyph& desc)
{
  ezUInt32 charWidth = CalculateCharacterWidth(m_LastChar, desc);

  m_Width += charWidth;
  m_Height = ezMath::Max(m_Height, desc.m_Height);

  if (m_LastChar == nullptr) // First char
    m_CharsStart = m_CharsEnd = charIndex;
  else
    m_CharsEnd = charIndex;

  m_LastChar = &desc;

  return charWidth;
}

void ezTextData::ezTextWord::AddSpace(ezUInt32 spaceWidth)
{
  m_SpaceWidth += spaceWidth;
  m_Width = m_SpaceWidth;
  m_Height = 0;
}

ezUInt32 ezTextData::ezTextWord::CalculateWidthWithCharacter(const ezFontGlyph& desc)
{
  return m_Width + CalculateCharacterWidth(m_LastChar, desc);
}

ezUInt32 ezTextData::ezTextWord::CalculateCharacterWidth(const ezFontGlyph* prevDesc, const ezFontGlyph& desc)
{
  ezUInt32 charWidth = desc.m_XAdvance;
  if (prevDesc != nullptr)
  {
    ezUInt32 kerning = 0;
    for (ezUInt32 j = 0; j < prevDesc->m_KerningPairs.GetCount(); j++)
    {
      if (prevDesc->m_KerningPairs[j].m_OtherCharacterId == desc.m_CharacterId)
      {
        kerning = prevDesc->m_KerningPairs[j].m_Amount;
        break;
      }
    }

    charWidth += kerning;
  }

  return charWidth;
}

////////////////////////////////////////////////////////////////////////////////

ezUInt32 ezTextData::ezTextLine::CalculateWidthWithCharacter(const ezFontGlyph& desc)
{
  ezUInt32 charWidth = 0;

  if (!m_IsEmpty)
  {
    ezTextWord& lastWord = MemBuffer->WordBuffer[m_WordsEnd];
    if (lastWord.IsSpacer())
      charWidth = ezTextWord::CalculateCharacterWidth(nullptr, desc);
    else
      charWidth = lastWord.CalculateWidthWithCharacter(desc) - lastWord.GetWidth();
  }
  else
  {
    charWidth = ezTextWord::CalculateCharacterWidth(nullptr, desc);
  }

  return m_Width + charWidth;
}

ezUInt32 ezTextData::ezTextLine::FillBuffer(ezUInt32 page, ezVec2* vertices, ezVec2* uvs, ezUInt32* indexes, ezUInt32 offset, ezUInt32 size) const
{
  ezUInt32 numQuads = 0;

  if (m_IsEmpty)
    return numQuads;

  ezUInt32 penX = 0;
  ezUInt32 penNegativeXOffset = 0;
  for (ezUInt32 i = m_WordsStart; i <= m_WordsEnd; i++)
  {
    const ezTextWord& word = m_TextData->GetWord(i);

    if (word.IsSpacer())
    {
      // We store invisible space quads in the first page. Even though they aren't needed
      // for rendering and we could just leave an empty space, they are needed for intersection tests
      // for things like determining caret placement and selection areas
      if (page == 0)
      {
        ezInt32 curX = penX;
        ezInt32 curY = 0;

        ezUInt32 curVert = offset * 4;
        ezUInt32 curIndex = offset * 6;

        vertices[curVert + 0] = ezVec2((float)curX, (float)curY);
        vertices[curVert + 1] = ezVec2((float)(curX + word.GetWidth()), (float)curY);
        vertices[curVert + 2] = ezVec2((float)curX, (float)curY + (float)m_TextData->GetLineHeight());
        vertices[curVert + 3] = ezVec2((float)(curX + word.GetWidth()), (float)curY + (float)m_TextData->GetLineHeight());

        if (uvs != nullptr)
        {
          uvs[curVert + 0] = ezVec2(0.0f, 0.0f);
          uvs[curVert + 1] = ezVec2(0.0f, 0.0f);
          uvs[curVert + 2] = ezVec2(0.0f, 0.0f);
          uvs[curVert + 3] = ezVec2(0.0f, 0.0f);
        }

        // Triangles are back-facing which makes them invisible
        if (indexes != nullptr)
        {
          indexes[curIndex + 0] = curVert + 0;
          indexes[curIndex + 1] = curVert + 2;
          indexes[curIndex + 2] = curVert + 1;
          indexes[curIndex + 3] = curVert + 1;
          indexes[curIndex + 4] = curVert + 2;
          indexes[curIndex + 5] = curVert + 3;
        }

        offset++;
        numQuads++;

        if (offset > size)
          EZ_REPORT_FAILURE("Out of buffer bounds. Buffer size: {0}", size);
      }

      penX += word.GetWidth();
    }
    else
    {
      ezUInt32 kerning = 0;
      for (ezUInt32 j = word.GetCharsStart(); j <= word.GetCharsEnd(); j++)
      {
        const ezFontGlyph& curChar = m_TextData->GetCharacter(j);

        ezInt32 curX = penX + curChar.m_XOffset;
        ezInt32 curY = (m_TextData->GetBaselineOffset() - curChar.m_YOffset);

        curX += penNegativeXOffset;
        penX += curChar.m_XAdvance + kerning;

        kerning = 0;
        if ((j + 1) <= word.GetCharsEnd())
        {
          const ezFontGlyph& nextChar = m_TextData->GetCharacter(j + 1);
          for (ezUInt32 j = 0; j < curChar.m_KerningPairs.GetCount(); j++)
          {
            if (curChar.m_KerningPairs[j].m_OtherCharacterId == nextChar.m_CharacterId)
            {
              kerning = curChar.m_KerningPairs[j].m_Amount;
              break;
            }
          }
        }

        if (curChar.m_Page != page)
          continue;

        ezUInt32 curVert = offset * 4;
        ezUInt32 curIndex = offset * 6;

        vertices[curVert + 0] = ezVec2((float)curX, (float)curY);
        vertices[curVert + 1] = ezVec2((float)(curX + curChar.m_Width), (float)curY);
        vertices[curVert + 2] = ezVec2((float)curX, (float)curY + (float)curChar.m_Height);
        vertices[curVert + 3] = ezVec2((float)(curX + curChar.m_Width), (float)curY + (float)curChar.m_Height);

        if (uvs != nullptr)
        {
          uvs[curVert + 0] = ezVec2(curChar.m_UVX, curChar.m_UVY);
          uvs[curVert + 1] = ezVec2(curChar.m_UVX + curChar.m_UVWidth, curChar.m_UVY);
          uvs[curVert + 2] = ezVec2(curChar.m_UVX, curChar.m_UVY + curChar.m_UVHeight);
          uvs[curVert + 3] = ezVec2(curChar.m_UVX + curChar.m_UVWidth, curChar.m_UVY + curChar.m_UVHeight);
        }

        if (indexes != nullptr)
        {
          indexes[curIndex + 0] = curVert + 0;
          indexes[curIndex + 1] = curVert + 1;
          indexes[curIndex + 2] = curVert + 2;
          indexes[curIndex + 3] = curVert + 1;
          indexes[curIndex + 4] = curVert + 3;
          indexes[curIndex + 5] = curVert + 2;
        }

        offset++;
        numQuads++;

        if (offset > size)
          EZ_REPORT_FAILURE("Out of buffer bounds. Buffer size: {0}", size);
      }
    }
  }

  return numQuads;
}

bool ezTextData::ezTextLine::IsAtWordBoundary() const
{
  return m_IsEmpty || MemBuffer->WordBuffer[m_WordsEnd].IsSpacer();
}

ezUInt32 ezTextData::ezTextLine::GetNumCharacters() const
{
  if (m_IsEmpty)
    return 0;

  ezUInt32 numChars = 0;
  for (ezUInt32 i = m_WordsStart; i <= m_WordsEnd; i++)
  {
    ezTextWord& word = MemBuffer->WordBuffer[i];

    if (word.IsSpacer())
      numChars++;
    else
      numChars += (ezUInt32)word.GetNumCharacters();
  }

  return numChars;
}

void ezTextData::ezTextLine::AddCharacter(ezUInt32 charIdx, const ezFontGlyph& charDesc)
{
  ezUInt32 charWidth = 0;
  if (m_IsEmpty)
  {
    m_WordsStart = m_WordsEnd = MemBuffer->AllocateWord(false);
    m_IsEmpty = false;
  }
  else
  {
    if (MemBuffer->WordBuffer[m_WordsEnd].IsSpacer())
      m_WordsEnd = MemBuffer->AllocateWord(false);
  }

  ezTextWord& lastWord = MemBuffer->WordBuffer[m_WordsEnd];
  charWidth = lastWord.AddCharacter(charIdx, charDesc);

  m_Width += charWidth;
  m_Height = ezMath::Max(m_Height, lastWord.GetHeight());
}

void ezTextData::ezTextLine::AddSpace(ezUInt32 spaceWidth)
{
  if (m_IsEmpty)
  {
    m_WordsStart = m_WordsEnd = MemBuffer->AllocateWord(true);
    m_IsEmpty = false;
  }
  else
    m_WordsEnd = MemBuffer->AllocateWord(true); // Each space is counted as its own word, to make certain operations easier

  ezTextWord& lastWord = MemBuffer->WordBuffer[m_WordsEnd];
  lastWord.AddSpace(spaceWidth);

  m_Width += spaceWidth;
}

void ezTextData::ezTextLine::AddWord(ezUInt32 wordIdx, const ezTextWord& word)
{
  if (m_IsEmpty)
  {
    m_WordsStart = m_WordsEnd = wordIdx;
    m_IsEmpty = false;
  }
  else
    m_WordsEnd = wordIdx;

  m_Width += word.GetWidth();
  m_Height = ezMath::Max(m_Height, word.GetHeight());
}

void ezTextData::ezTextLine::Initialize(ezTextData* textData)
{
  m_Width = 0;
  m_Height = 0;
  m_IsEmpty = true;
  m_TextData = textData;
  m_WordsStart = m_WordsEnd = 0;
}

void ezTextData::ezTextLine::Finalize(bool hasNewlineChar)
{
  m_HasNewline = hasNewlineChar;
}

ezUInt32 ezTextData::ezTextLine::RemoveLastWord()
{
  if (m_IsEmpty)
  {
    EZ_REPORT_FAILURE("Attempting to remove word from empty line.");
    return 0;
  }

  ezUInt32 lastWord = m_WordsEnd--;
  if (m_WordsStart == lastWord)
  {
    m_IsEmpty = true;
    m_WordsStart = m_WordsEnd = 0;
  }

  CalculateBounds();

  return lastWord;
}

void ezTextData::ezTextLine::CalculateBounds()
{
  m_Width = 0;
  m_Height = 0;

  if (m_IsEmpty)
    return;

  for (ezUInt32 i = m_WordsStart; i <= m_WordsEnd; i++)
  {
    ezTextWord& word = MemBuffer->WordBuffer[i];

    m_Width += word.GetWidth();
    m_Height = ezMath::Max(m_Height, word.GetHeight());
  }
}

////////////////////////////////////////////////////////////////////////////////

ezTextData::ezTextData(const ezString32& text, const ezFontResourceHandle& font, ezUInt32 fontSize, ezUInt32 width, ezUInt32 height, bool wordWrap, bool wordBreak)
  : m_NumChars(0)
  , m_NumWords(0)
  , m_NumLines(0)
  , m_NumPageInfos(0)
{
  m_FontBitmap = nullptr;
  Initialize();

  if (font.IsValid())
  {
    ezResourceLock<ezFontResource> fontResource(font, ezResourceAcquireMode::BlockTillLoaded);
    ezUInt32 closestSize = fontResource->GetClosestSize(fontSize);

    m_FontBitmap = &fontResource->GetBitmap(closestSize);
  }

  if (m_FontBitmap == nullptr || m_FontBitmap->m_TexturePages.GetCount() == 0)
    return;

  if (m_FontBitmap->m_Size != fontSize)
  {
    ezLog::Warning("Unable to find font with specified size ({0}). Using nearest available size: {1}", fontSize, m_FontBitmap->m_Size);
  }

  bool widthIsLimited = width > 0;
  m_Font = font;

  ezUInt32 curLineIdx = MemBuffer->AllocateLine(this);
  ezUInt32 curHeight = m_FontBitmap->m_LineHeight;
  ezUInt32 charIdx = 0;

  while (true)
  {
    if (charIdx >= text.GetCharacterCount())
      break;

    ezUInt32 charId = text[charIdx];
    const ezFontGlyph& charDesc = m_FontBitmap->GetFontGlyph(charId);

    ezTextLine* curLine = &MemBuffer->LineBuffer[curLineIdx];

    if (text[charIdx] == '\n' || text[charIdx] == '\r')
    {
      curLine->Finalize(true);

      curLineIdx = MemBuffer->AllocateLine(this);
      curLine = &MemBuffer->LineBuffer[curLineIdx];

      curHeight += m_FontBitmap->m_LineHeight;

      charIdx++;

      // Check for \r\n
      if (text[charIdx - 1] == '\r' && charIdx < text.GetCharacterCount())
      {
        if (text[charIdx] == '\n')
          charIdx++;
      }

      continue;
    }

    if (widthIsLimited && wordWrap)
    {
      ezUInt32 widthWithChar = 0;
      if (charIdx == EZ_SPACE_CHAR)
        widthWithChar = curLine->GetWidth() + GetSpaceWidth();
      else if (charIdx == EZ_TAB_CHAR)
        widthWithChar = curLine->GetWidth() + GetSpaceWidth() * 4;
      else
        widthWithChar = curLine->CalculateWidthWithCharacter(charDesc);

      if (widthWithChar > width && !curLine->IsEmpty())
      {
        bool atWordBoundary = charId == EZ_SPACE_CHAR || charId == EZ_TAB_CHAR || curLine->IsAtWordBoundary();

        if (!atWordBoundary) // Need to break word into multiple pieces, or move it to next line
        {
          ezUInt32 lastWordIdx = curLine->RemoveLastWord();
          ezTextWord& lastWord = MemBuffer->WordBuffer[lastWordIdx];

          bool wordFits = lastWord.CalculateWidthWithCharacter(charDesc) <= width;
          if (wordFits && !curLine->IsEmpty())
          {
            curLine->Finalize(false);

            curLineIdx = MemBuffer->AllocateLine(this);
            curLine = &MemBuffer->LineBuffer[curLineIdx];

            curHeight += m_FontBitmap->m_LineHeight;

            curLine->AddWord(lastWordIdx, lastWord);
          }
          else
          {
            if (wordBreak)
            {
              curLine->AddWord(lastWordIdx, lastWord);
              curLine->Finalize(false);

              curLineIdx = MemBuffer->AllocateLine(this);
              curLine = &MemBuffer->LineBuffer[curLineIdx];

              curHeight += m_FontBitmap->m_LineHeight;
            }
            else
            {
              if (!curLine->IsEmpty()) // Add new line unless current line is empty (to avoid constantly moving the word to new lines)
              {
                curLine->Finalize(false);

                curLineIdx = MemBuffer->AllocateLine(this);
                curLine = &MemBuffer->LineBuffer[curLineIdx];

                curHeight += m_FontBitmap->m_LineHeight;
              }

              curLine->AddWord(lastWordIdx, lastWord);
            }
          }
        }
        else if (charId != EZ_SPACE_CHAR && charId != EZ_TAB_CHAR) // If current char is whitespace add it to the existing line even if it doesn't fit
        {
          curLine->Finalize(false);

          curLineIdx = MemBuffer->AllocateLine(this);
          curLine = &MemBuffer->LineBuffer[curLineIdx];

          curHeight += m_FontBitmap->m_LineHeight;
        }
      }
    }

    if (charId == EZ_SPACE_CHAR)
    {
      curLine->AddSpace(GetSpaceWidth());
      MemBuffer->AddCharacterToPage(0, *m_FontBitmap);
    }
    else if (charId == EZ_TAB_CHAR)
    {
      curLine->AddSpace(GetSpaceWidth() * 4);
      MemBuffer->AddCharacterToPage(0, *m_FontBitmap);
    }
    else
    {
      curLine->AddCharacter(charIdx, charDesc);
      MemBuffer->AddCharacterToPage(charDesc.m_Page, *m_FontBitmap);
    }

    charIdx++;
  }

  MemBuffer->LineBuffer[curLineIdx].Finalize(true);

  // Now that we have all the data we need, allocate the permanent buffers and copy the data
  m_NumChars = text.GetCharacterCount();
  m_NumWords = MemBuffer->NextFreeWord;
  m_NumLines = MemBuffer->NextFreeLine;
  m_NumPageInfos = MemBuffer->NextFreePageInfo;

  GeneratePersistentData(text);
}

ezUInt32 ezTextData::GetLineHeight() const
{
  return m_FontBitmap->m_LineHeight;
}

const ezTexture2DResourceHandle& ezTextData::GetTextureForPage(ezUInt32 page) const
{
  return m_FontBitmap->m_TexturePages[page];
}

ezUInt32 ezTextData::GetWidth() const
{
  ezUInt32 width = 0;

  for (ezUInt32 i = 0; i < m_NumLines; i++)
    width = ezMath::Max(width, m_Lines[i].GetWidth());

  return width;
}

ezUInt32 ezTextData::GetHeight() const
{
  ezUInt32 height = 0;

  for (ezUInt32 i = 0; i < m_NumLines; i++)
    height += m_Lines[i].GetHeight();

  return height;
}

void ezTextData::GeneratePersistentData(const ezString& text, bool freeTemporary)
{
  m_Chars.SetCount(m_NumChars);

  for (ezUInt32 i = 0; i < m_NumChars; i++)
  {
    ezUInt32 charId = text[i];
    const ezFontGlyph& charDesc = m_FontBitmap->GetFontGlyph(charId);
    m_Chars[i] = &charDesc;
  }

  m_Words.SetCount(m_NumWords);
  ezMemoryUtils::Copy(m_Words.GetData(), MemBuffer->WordBuffer.GetData(), m_NumWords);
  //m_Words.GetArrayPtr().CopyFrom(MemBuffer->WordBuffer.GetArrayPtr());

  m_Lines.SetCount(m_NumLines);
  ezMemoryUtils::Copy(m_Lines.GetData(), MemBuffer->LineBuffer.GetData(), m_NumLines);
  //m_Lines.GetArrayPtr().CopyFrom(MemBuffer->LineBuffer.GetArrayPtr());

  m_PageInfos.SetCount(m_NumPageInfos);
  ezMemoryUtils::Copy(m_PageInfos.GetData(), MemBuffer->PageBuffer.GetData(), m_NumPageInfos);
  //m_PageInfos.GetArrayPtr().CopyFrom(MemBuffer->PageBuffer.GetArrayPtr());

  if (freeTemporary)
    MemBuffer->DeallocateAll();
}

ezInt32 ezTextData::GetBaselineOffset() const
{
  return m_FontBitmap->m_BaselineOffset;
}

ezUInt32 ezTextData::GetSpaceWidth() const
{
  return m_FontBitmap->m_SpaceWidth;
}

void ezTextData::Initialize()
{
  if (MemBuffer == nullptr)
    MemBuffer = EZ_DEFAULT_NEW(ezBufferData);
}


////////////////////////////////////////////////////////////////////////////////

thread_local ezTextData::ezBufferData* ezTextData::MemBuffer = nullptr;

ezTextData::ezBufferData::ezBufferData()
{
  WordBufferSize = 2000;
  LineBufferSize = 500;
  PageBufferSize = 20;

  NextFreeWord = 0;
  NextFreeLine = 0;
  NextFreePageInfo = 0;

  WordBuffer.SetCount(WordBufferSize);
  LineBuffer.SetCount(LineBufferSize);
  PageBuffer.SetCount(PageBufferSize);
}

ezUInt32 ezTextData::ezBufferData::AllocateWord(bool spacer)
{
  if (NextFreeWord >= WordBufferSize)
  {
    ezUInt32 newBufferSize = WordBufferSize * 2;

    WordBuffer.SetCount(newBufferSize);
    WordBufferSize = newBufferSize;
  }

  WordBuffer[NextFreeWord].Initialize(spacer);

  return NextFreeWord++;
}

ezUInt32 ezTextData::ezBufferData::AllocateLine(ezTextData* textData)
{
  if (NextFreeLine >= LineBufferSize)
  {
    ezUInt32 newBufferSize = LineBufferSize * 2;

    LineBuffer.SetCount(newBufferSize);
    LineBufferSize = newBufferSize;
  }

  LineBuffer[NextFreeLine].Initialize(textData);

  return NextFreeLine++;
}

void ezTextData::ezBufferData::AddCharacterToPage(ezUInt32 page, const ezFontBitmap& fontData)
{
  if (NextFreePageInfo >= PageBufferSize)
  {
    ezUInt32 newBufferSize = PageBufferSize * 2;

    PageBuffer.SetCount(newBufferSize);
    PageBufferSize = newBufferSize;
  }

  while (page >= NextFreePageInfo)
  {
    PageBuffer[NextFreePageInfo].m_NumQuads = 0;

    NextFreePageInfo++;
  }

  PageBuffer[page].m_NumQuads++;
}

void ezTextData::ezBufferData::DeallocateAll()
{
  NextFreeWord = 0;
  NextFreeLine = 0;
  NextFreePageInfo = 0;
}
