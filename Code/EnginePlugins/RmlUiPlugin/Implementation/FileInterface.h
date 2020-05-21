#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <Foundation/IO/MemoryStream.h>

#include <RmlUi/Core/FileInterface.h>

namespace ezRmlUiInternal
{
  struct FileId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static FileId FromRml(Rml::Core::FileHandle hFile)
    {
      return FileId(static_cast<ezUInt32>(hFile));
    }

    Rml::Core::FileHandle ToRml() const
    {
      return m_Data;
    }
  };

  //////////////////////////////////////////////////////////////////////////

  class FileInterface : public Rml::Core::FileInterface
  {
  public:
    FileInterface();
    virtual ~FileInterface();

    virtual Rml::Core::FileHandle Open(const Rml::Core::String& path) override;
    virtual void Close(Rml::Core::FileHandle file) override;

    virtual size_t Read(void* buffer, size_t size, Rml::Core::FileHandle file) override;

    virtual bool Seek(Rml::Core::FileHandle file, long offset, int origin) override;
    virtual size_t Tell(Rml::Core::FileHandle file) override;

    virtual size_t Length(Rml::Core::FileHandle file) override;

  private:
    struct OpenFile
    {
      ezMemoryStreamStorage m_Storage;
      ezMemoryStreamReader m_Reader;
    };

    ezIdTable<FileId, ezUniquePtr<OpenFile>> m_OpenFiles;
  };
} // namespace ezRmlUiInternal
