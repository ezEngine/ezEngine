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

    virtual Rml::FileHandle Open(const Rml::String& sPath) override;
    virtual void Close(Rml::FileHandle hFile) override;

    virtual size_t Read(void* pBuffer, size_t uiSize, Rml::FileHandle hFile) override;

    virtual bool Seek(Rml::FileHandle hFile, long iOffset, int iOrigin) override;
    virtual size_t Tell(Rml::FileHandle hFile) override;

    virtual size_t Length(Rml::FileHandle hFile) override;

  private:
    struct OpenFile
    {
      ezDefaultMemoryStreamStorage m_Storage;
      ezMemoryStreamReader m_Reader;
    };

    ezIdTable<FileId, ezUniquePtr<OpenFile>> m_OpenFiles;
  };
} // namespace ezRmlUiInternal
