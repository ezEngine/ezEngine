#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/UniquePtr.h>

#include <Foundation/Containers/IdTable.h>
#include <RmlUi/Core/FileInterface.h>

namespace ezRmlUiInternal
{
  struct FileId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static FileId FromRml(Rml::FileHandle hFile) { return FileId(static_cast<ezUInt32>(hFile)); }

    Rml::FileHandle ToRml() const { return m_Data; }
  };

  //////////////////////////////////////////////////////////////////////////

  class FileInterface final : public Rml::FileInterface
  {
  public:
    FileInterface();
    virtual ~FileInterface();

    virtual Rml::FileHandle Open(const Rml::String& path) override;
    virtual void Close(Rml::FileHandle file) override;

    virtual size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;

    virtual bool Seek(Rml::FileHandle file, long offset, int origin) override;
    virtual size_t Tell(Rml::FileHandle file) override;

    virtual size_t Length(Rml::FileHandle file) override;

  private:
    struct OpenFile
    {
      ezMemoryStreamStorage m_Storage;
      ezMemoryStreamReader m_Reader;
    };

    ezIdTable<FileId, ezUniquePtr<OpenFile>> m_OpenFiles;
  };
} // namespace ezRmlUiInternal
