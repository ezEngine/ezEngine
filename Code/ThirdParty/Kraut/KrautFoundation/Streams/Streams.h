#ifndef AE_FOUNDATION_STREAMS_STREAMS_H
#define AE_FOUNDATION_STREAMS_STREAMS_H

#include "../Defines.h"
#include "../Strings/String.h"

namespace AE_NS_FOUNDATION
{
  //! Base class interface for all classes that provide data-streams for reading or writing, such as files.
  class AE_FOUNDATION_DLL aeStreamBase
  {
  public:
    //! Returns whether the stream was successfully opened.
    bool IsStreamOpen(void) const { return (m_bIsOpen); }

  protected:
    aeStreamBase(void)
      : m_bIsOpen(false)
    {
    }
    virtual ~aeStreamBase() {}

    //! \note This variable has to be updated by the implementing class.
    bool m_bIsOpen;

  private:
    aeStreamBase(const aeStreamBase& cc);
    void operator=(const aeStreamBase& cc);
  };

  //! Interface-class for all classes that allow to write data to a stream, such as files.
  class AE_FOUNDATION_DLL aeStreamOut : public virtual aeStreamBase
  {
  public:
    aeStreamOut(void)
      : m_uiDataWritten(0)
    {
    }

    //! Writes the given number of bytes from the buffer to the stream.
    void Write(const void* pData, aeUInt32 uiSize)
    {
      m_uiDataWritten += uiSize;
      WriteToStream(pData, uiSize);
    }

  private:
    //! Writes the given number of bytes from the buffer to the stream.
    virtual void WriteToStream(const void* pData, aeUInt32 uiSize) = 0;

    aeUInt32 m_uiDataWritten;
  };

  //! Interface-class for all classes that allow to read data from a stream, such as files.
  class AE_FOUNDATION_DLL aeStreamIn : public virtual aeStreamBase
  {
  public:
    aeStreamIn(void)
      : m_bEndOfStream(false)
      , m_uiDataRead(0)
    {
    }

    //! Attempts to read the given number of bytes into the buffer. Returns the actual number of bytes read.
    aeUInt32 Read(void* pData, aeUInt32 uiSize)
    {
      if (m_bEndOfStream)
        return (0);

      aeUInt32 uiRead = ReadFromStream(pData, uiSize);
      m_uiDataRead += uiRead;

      if (uiRead < uiSize)
        m_bEndOfStream = true;

      return (uiRead);
    }

    //! Returns true if the end of the data-stream has been reached.
    bool IsEndOfStream(void) const { return (m_bEndOfStream); }

    //! Reads a string from the stream until it encounters the first '\n' or '\0' or eof. Ignores all '\r' (skips them).
    void ReadLine(aeString& out_sString);

  protected:
    //! Implementations of aeStreamIn can call this to specify when a stream was read to the end. This might be more precise than the automatic eof detection.
    void SetEndOfStreamReached(void) { m_bEndOfStream = true; }

  private:
    virtual aeUInt32 ReadFromStream(void* pData, aeUInt32 uiSize) = 0;

    bool m_bEndOfStream;
    aeUInt32 m_uiDataRead;
  };
} // namespace AE_NS_FOUNDATION

#include "../Streams/StreamOperators.h"

#endif
