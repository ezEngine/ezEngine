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

    static FileId FromRml(Rml::FileHandle file) { return FileId(static_cast<ezUInt32>(file)); }

    Rml::FileHandle ToRml() const { return m_Data; }
  };

  //////////////////////////////////////////////////////////////////////////

  class FileInterface final : public Rml::FileInterface
  {
  public:
    FileInterface();
    virtual ~FileInterface();

    virtual Rml::FileHandle Open(const Rml::String& sPath) override;
    virtual void Close(Rml::FileHandle file) override;

    virtual size_t Read(void* pBuffer, size_t uiSize, Rml::FileHandle file) override;

    virtual bool Seek(Rml::FileHandle file, long iOffset, int iOrigin) override;
    virtual size_t Tell(Rml::FileHandle file) override;

    virtual size_t Length(Rml::FileHandle file) override;

  private:
    struct OpenFile
    {
      ezDefaultMemoryStreamStorage m_Storage;
      ezMemoryStreamReader m_Reader;
    };

    ezIdTable<FileId, ezUniquePtr<OpenFile>> m_OpenFiles;
  };
} // namespace ezRmlUiInternal
