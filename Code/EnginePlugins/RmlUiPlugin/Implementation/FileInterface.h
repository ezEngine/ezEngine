#pragma once

#include <Foundation/Types/UniquePtr.h>

#include <RmlUi/Core/FileInterface.h>

class ezFileReader;

namespace ezRmlUiInternal
{
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
    ezHashTable<Rml::Core::FileHandle, ezUniquePtr<ezFileReader>> m_OpenFiles;
    Rml::Core::FileHandle m_NextFileHandle = 1;
  };
} // namespace ezRmlUiInternal
