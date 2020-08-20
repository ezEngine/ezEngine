#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/Font/FontResource.h>
#include <RendererCore/RendererCoreDLL.h>

/**
	 * This object takes as input a string, a font and optionally some constraints (like word wrap) and outputs a set of
	 * character data you may use for rendering or metrics.
	 */
class ezTextData
{
protected:
  /**
		 * Represents a single word as a set of characters, or optionally just a blank space of a certain length.
		 */
  class ezTextWord
  {
  public:
    ezTextWord()
      : m_CharsStart(0)
      , m_CharsEnd(0)
      , m_Width(0)
      , m_Height(0)
      , m_LastChar(nullptr)
      , m_Spacer(false)
      , m_SpaceWidth(0)
    {
    }

    /**
			 * Initializes the word and signals if it just a space (or multiple spaces), or an actual word with letters.
			 */
    void Initialize(bool spacer);

    /**
			 * Appends a new character to the word.
			 *
			 * @param[in]	charIndex		Sequential index of the character in the original string.
			 * @param[in]	desc		Character description from the font.
			 * @return					How many pixels did the added character expand the word by.
			 */
    ezUInt32 AddCharacter(ezUInt32 charIndex, const ezFontGlyph& desc);

    /** Adds a space to the word. Word must have previously have been declared as a "spacer". */
    void AddSpace(ezUInt32 spaceWidth);

    /**	Returns the width of the word in pixels. */
    ezUInt32 GetWidth() const { return m_Width; }

    /**	Returns height of the word in pixels. */
    ezUInt32 GetHeight() const { return m_Height; }

    /**
			 * Calculates new width of the word if we were to add the provided character, without actually adding it.
			 *
			 * @param[in]	desc	Character description from the font.
			 * @return				Width of the word in pixels with the character appended to it.
			 */
    ezUInt32 CalculateWidthWithCharacter(const ezFontGlyph& desc);

    /**
			 * Returns true if word is a spacer. Spacers contain just a space of a certain length with no actual characters.
			 */
    bool IsSpacer() const { return m_Spacer; }

    /**	Returns the number of characters in the word. */
    ezUInt32 GetNumCharacters() const { return m_LastChar == nullptr ? 0 : (m_CharsEnd - m_CharsStart + 1); }

    /**	Returns the index of the starting character in the word. */
    ezUInt32 GetCharsStart() const { return m_CharsStart; }

    /**	Returns the index of the last character in the word. */
    ezUInt32 GetCharsEnd() const { return m_CharsEnd; }
    /**
			 * Calculates width of the character by which it would expand the width of the word if it was added to it.
			 *
			 * @param[in]	prevDesc	Descriptor of the character preceding the one we need the width for. Can be null.
			 * @param[in]	desc		Character description from the font.
			 * @return 					How many pixels would the added character expand the word by.
			 */
    static ezUInt32 CalculateCharacterWidth(const ezFontGlyph* prevDesc, const ezFontGlyph& desc);

  private:
    ezUInt32 m_CharsStart, m_CharsEnd;
    ezUInt32 m_Width;
    ezUInt32 m_Height;

    const ezFontGlyph* m_LastChar;

    bool m_Spacer;
    ezUInt32 m_SpaceWidth;
  };

  /**
		 * Contains information about a single texture page that contains rendered character data.
		 */
  struct ezPageInfo
  {
    EZ_DECLARE_POD_TYPE();

    ezPageInfo()
      : m_NumQuads(0)
      , m_Texture(nullptr)
    {
    }

    ezUInt32 m_NumQuads;
    ezTexture2DResourceHandle m_Texture;
  };

public:
  /**
		 * Represents a single line as a set of words.
		 */
  class EZ_RENDERERCORE_DLL ezTextLine
  {
  public:
    ezTextLine()
      : m_TextData(nullptr)
      , m_WordsStart(0)
      , m_WordsEnd(0)
      , m_Width(0)
      , m_Height(0)
      , m_IsEmpty(false)
      , m_HasNewline(false)
    {
    }

    /**	Returns width of the line in pixels. */
    ezUInt32 GetWidth() const { return m_Width; }

    /**	Returns height of the line in pixels. */
    ezUInt32 GetHeight() const { return m_Height; }

    /**	Returns an offset used to separate two lines. */
    ezUInt32 GetYOffset() const { return m_TextData->GetLineHeight(); }

    /**
			 * Calculates new width of the line if we were to add the provided character, without actually adding it.
			 *
			 * @param[in]	desc	Character description from the font.
			 * @return				Width of the line in pixels with the character appended to it.
			 */
    ezUInt32 CalculateWidthWithCharacter(const ezFontGlyph& desc);

    /**
			 * Fills the vertex/uv/index buffers for the specified page, with all the character data needed for rendering.
			 *
			 * @param[in]	page		The page.
			 * @param[out]	vertices	Pre-allocated array where character vertices will be written.
			 * @param[out]	uvs			Pre-allocated array where character uv coordinates will be written.
			 * @param[out]	indexes 	Pre-allocated array where character indices will be written.
			 * @param[in]	offset		Offsets the location at which the method writes to the buffers. Counted as number
			 *							of quads.
			 * @param[in]	size		Total number of quads that can fit into the specified buffers.
			 * @return					Number of quads that were written.
			 */
    ezUInt32 FillBuffer(ezUInt32 page, ezVec2* vertices, ezVec2* uvs, ezUInt32* indexes, ezUInt32 offset, ezUInt32 size) const;
    ezUInt32 FillBuffer(ezUInt32 page, ezDynamicArray<ezVec2>& vertices, ezDynamicArray<ezVec2>& uvs, ezDynamicArray<ezUInt32>& indexes, ezUInt32 offset, ezUInt32 size) const;

    /**	Checks are we at a word boundary (meaning the next added character will start a new word). */
    bool IsAtWordBoundary() const;

    /**	Returns the total number of characters on this line. */
    ezUInt32 GetNumCharacters() const;

    /**
			 * Query if this line was created explicitly due to a newline character. As opposed to a line that was created
			 * because a word couldn't fit on the previous line.
			 */
    bool HasNewlineCharacter() const { return m_HasNewline; }

  private:
    friend class ezTextData;

    /**
			 * Appends a new character to the line.
			 *
			 * @param[in]	charIdx		Sequential index of the character in the original string.
			 * @param[in]	charDesc	Character description from the font.
			 */
    void AddCharacter(ezUInt32 charIdx, const ezFontGlyph& charDesc);

    /**	Appends a space to the line. */
    void AddSpace(ezUInt32 spaceWidth);

    /**
			 * Adds a new word to the line.
			 *
			 * @param[in]	wordIdx		Sequential index of the word in the original string. Spaces are counted as words as
			 *							well.
			 * @param[in]	word		Description of the word.
			 */
    void AddWord(ezUInt32 wordIdx, const ezTextWord& word);

    /** Initializes the line. Must be called after construction. */
    void Initialize(ezTextData* textData);

    /**
			 * Finalizes the line. Do not add new characters/words after a line has been finalized.
			 *
			 * @param[in]	hasNewlineChar	Set to true if line was create due to an explicit newline char. As opposed to a
			 *								line that was created because a word couldn't fit on the previous line.
			 */
    void Finalize(bool hasNewlineChar);

    /**	Returns true if the line contains no words. */
    bool IsEmpty() const { return m_IsEmpty; }

    /**	Removes last word from the line and returns its sequential index. */
    ezUInt32 RemoveLastWord();

    /**	Calculates the line width and height in pixels. */
    void CalculateBounds();

  private:
    ezTextData* m_TextData;
    ezUInt32 m_WordsStart, m_WordsEnd;

    ezUInt32 m_Width;
    ezUInt32 m_Height;

    bool m_IsEmpty;
    bool m_HasNewline;
  };

public:
  /**
		 * Initializes a new text data using the specified string and font. Text will attempt to fit into the provided area.
		 * If enabled it will wrap words to new line when they don't fit. Individual words will be broken into multiple
		 * pieces if they don't fit on a single line when word break is enabled, otherwise they will be clipped. If the
		 * specified area is zero size then the text will not be clipped or word wrapped in any way.
		 *
		 * After this object is constructed you may call various getter methods to get needed information.
		 */
  EZ_RENDERERCORE_DLL ezTextData(const ezString32& text, const ezFontResourceHandle& font, ezUInt32 fontSize, ezUInt32 width = 0, ezUInt32 height = 0, bool wordWrap = false, bool wordBreak = true);

  EZ_RENDERERCORE_DLL  ~ezTextData() = default;

  /**	Returns the number of lines that were generated. */
  EZ_RENDERERCORE_DLL ezUInt32 GetNumLines() const { return m_NumLines; }

  /**	Returns the number of font pages references by the used characters. */
  EZ_RENDERERCORE_DLL ezUInt32 GetNumPages() const { return m_NumPageInfos; }

  /**	Returns the height of a line in pixels. */
  EZ_RENDERERCORE_DLL ezUInt32 GetLineHeight() const;

  /**	Gets information describing a single line at the specified index. */
  EZ_RENDERERCORE_DLL const ezTextLine& GetLine(ezUInt32 idx) const { return m_Lines[idx]; }

  /**	Returns font texture for the provided page index.  */
  EZ_RENDERERCORE_DLL const ezTexture2DResourceHandle& GetTextureForPage(ezUInt32 page) const;

  /**	Returns the number of quads used by all the characters in the provided page. */
  EZ_RENDERERCORE_DLL ezUInt32 GetNumQuadsForPage(ezUInt32 page) const { return m_PageInfos[page].m_NumQuads; }

  /**	Returns the width of the actual text in pixels. */
  EZ_RENDERERCORE_DLL ezUInt32 GetWidth() const;

  /**	Returns the height of the actual text in pixels. */
  EZ_RENDERERCORE_DLL ezUInt32 GetHeight() const;

private:
  /**
		 * Copies internally stored data in temporary buffers to a persistent buffer.
		 *
		 * @param[in]	text			Text originally used for creating the internal temporary buffer data.
		 * @param[in]	buffer			Memory location to copy the data to. If null then no data will be copied and the
		 *								parameter @p size will contain the required buffer size.
		 * @param[in]	size			Size of the provided memory buffer, or if the buffer is null, this will contain the
		 *								required buffer size after method exists.
		 * @param[in]	freeTemporary	If true the internal temporary data will be freed after copying.
		 *
		 * @note	Must be called after text data has been constructed and is in the temporary buffers.
		 */
  EZ_RENDERERCORE_DLL void GeneratePersistentData(const ezString& text, bool freeTemporary = true);

private:
  friend class ezTextLine;

  /**	Returns Y offset that determines the line on which the characters are placed. In pixels. */
  ezInt32 GetBaselineOffset() const;

  /**	Returns the width of a single space in pixels. */
  ezUInt32 GetSpaceWidth() const;

  /** Gets a description of a single character referenced by its sequential index based on the original string. */
  const ezFontGlyph& GetCharacter(ezUInt32 idx) const { return *m_Chars[idx]; }

  /** Gets a description of a single word referenced by its sequential index based on the original string. */
  const ezTextWord& GetWord(ezUInt32 idx) const { return m_Words[idx]; }

private:
  ezDynamicArray<const ezFontGlyph*> m_Chars;
  ezUInt32 m_NumChars;

  ezDynamicArray<ezTextWord> m_Words;
  ezUInt32 m_NumWords;

  ezDynamicArray<ezTextLine> m_Lines;
  ezUInt32 m_NumLines;

  ezDynamicArray<ezPageInfo> m_PageInfos;
  ezUInt32 m_NumPageInfos;

  ezFontResourceHandle m_Font;
  const ezFontBitmap* m_FontBitmap;

  // Static buffers used to reduce runtime memory allocation
protected:
  /** Stores per-thread memory buffers used to reduce memory allocation. */
  // Note: I could replace this with the global frame allocator to avoid the extra logic
  struct ezBufferData
  {
    EZ_DECLARE_POD_TYPE();

    ezBufferData();
    ~ezBufferData() = default;

    /**
			 * Allocates a new word and adds it to the buffer. Returns index of the word in the word buffer.
			 *
			 * @param[in]	spacer	Specify true if the word is only to contain spaces. (Spaces are considered a special
			 *						type of word).
			 */
    ezUInt32 AllocateWord(bool spacer);

    /** Allocates a new line and adds it to the buffer. Returns index of the line in the line buffer. */
    ezUInt32 AllocateLine(ezTextData* textData);

    /**
			 * Increments the count of characters for the referenced page, and optionally creates page info if it doesn't
			 * already exist.
			 */
    void AddCharacterToPage(ezUInt32 page, const ezFontBitmap& fontData);

    /**	Resets all allocation counters, but doesn't actually release memory. */
    void DeallocateAll();

    ezDynamicArray<ezTextWord> WordBuffer;
    ezUInt32 WordBufferSize;
    ezUInt32 NextFreeWord;

    ezDynamicArray<ezTextLine> LineBuffer;
    ezUInt32 LineBufferSize;
    ezUInt32 NextFreeLine;

    ezDynamicArray<ezPageInfo> PageBuffer;
    ezUInt32 PageBufferSize;
    ezUInt32 NextFreePageInfo;
  };

  static thread_local ezBufferData* MemBuffer;

  /**	Allocates an initial set of buffers that will be reused while parsing text data. */
  static void Initialize();
};
