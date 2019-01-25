#include <PCH.h>

#include <TexConv2/TexConv2.h>

ezTexConv2::ezTexConv2()
    : ezApplication("TexConv2")
{
}

void ezTexConv2::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("texconv");

  SUPER::BeforeCoreSystemsStartup();
}

void ezTexConv2::AfterCoreSystemsStartup()
{
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

ezApplication::ApplicationExecution ezTexConv2::Run()
{
  ezTexConvProcessor processor;
  processor.m_Descriptor.m_InputFiles.PushBack("D:/TexConv2/Test1.tga");
  processor.m_Descriptor.m_TargetFormat = ezTexConvTargetFormat::Color;

  if (processor.Process().Failed())
    return ezApplication::ApplicationExecution::Quit;

  processor.m_Output.SaveTo("D:/TexConv2/Test1 - out.png");


  return ezApplication::ApplicationExecution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv2);
