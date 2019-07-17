#include <Foundation/Application/Application.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/UniquePtr.h>


namespace
{
  EZ_ALWAYS_INLINE void SkipWhitespace(ezToken& token, ezUInt32& i, const ezDeque<ezToken>& tokens)
  {
    while (token.m_iType == ezTokenType::Whitespace)
    {
      token = tokens[++i];
    }
  }

  EZ_ALWAYS_INLINE void SkipLine(ezToken& token, ezUInt32& i, const ezDeque<ezToken>& tokens)
  {
    while (token.m_iType != ezTokenType::Newline && token.m_iType != ezTokenType::EndOfFile)
    {
      token = tokens[++i];
    }
  }
} // namespace

class ezHeaderCheckApp : public ezApplication
{
private:
  ezString m_sSearchDir;
  ezString m_sProjectName;
  bool m_bHadErrors;
  bool m_bHadSeriousWarnings;
  bool m_bHadWarnings;
  ezUniquePtr<ezStackAllocator<ezMemoryTrackingFlags::None>> m_stackAllocator;
  ezDynamicArray<ezString> m_includeDirectories;

  struct IgnoreInfo
  {
    ezHashSet<ezString> m_byName;
  };

  IgnoreInfo m_ignoreTarget;
  IgnoreInfo m_ignoreSource;
public:
  typedef ezApplication SUPER;

  ezHeaderCheckApp()
    : ezApplication("StaticLinkerApp")
  {
    m_bHadErrors = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const ezLoggingEventData& eventData)
  {
    ezHeaderCheckApp* app = (ezHeaderCheckApp*)ezApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case ezLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case ezLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case ezLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }

  ezResult ParseArray(const ezVariant& value, ezHashSet<ezString>& dst)
  {
    if (!value.CanConvertTo<ezVariantArray>())
    {
      ezLog::Error("Expected array");
      return EZ_FAILURE;
    }
    auto a = value.Get<ezVariantArray>();
    const auto arraySize = a.GetCount();
    for (ezUInt32 i = 0; i < arraySize; i++)
    {
      auto& el = a[i];
      if (!el.CanConvertTo<ezString>())
      {
        ezLog::Error("Value {0} at index {1} can not be converted to a string. Expected array of strings.", el, i);
        return EZ_FAILURE;
      }
      ezStringBuilder file = el.Get<ezString>();
      file.ToLower();
      dst.Insert(file);
    }
    return EZ_SUCCESS;
  }

  ezResult ParseIgnoreFile(const ezStringView ignoreFilePath)
  {
    ezJSONReader jsonReader;
    jsonReader.SetLogInterface(ezLog::GetThreadLocalLogSystem());

    ezFileReader reader;
    ezString sIgnoreFilePath = ignoreFilePath;
    if (reader.Open(sIgnoreFilePath).Failed())
    {
      ezLog::Error("Failed to open ignore file {0}", ignoreFilePath);
      return EZ_FAILURE;
    }

    if (jsonReader.Parse(reader).Failed())
      return EZ_FAILURE;

    const ezStringView includeTarget = "includeTarget";
    const ezStringView includeSource = "includeSource";
    const ezStringView byName = "byName";

    auto topLevel = jsonReader.GetTopLevelObject();
    for (auto it = topLevel.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Key() == includeTarget || it.Key() == includeSource)
      {
        IgnoreInfo& info = (it.Key() == includeTarget) ? m_ignoreTarget : m_ignoreSource;
        auto inner = it.Value().Get<ezVariantDictionary>();
        for (auto it2 = inner.GetIterator(); it2.IsValid(); it2.Next())
        {
          if (it2.Key() == byName)
          {
            if (ParseArray(it2.Value(), info.m_byName).Failed())
            {
              ezLog::Error("Failed to parse value of '{0}.{1}'.", it.Key(), it2.Key());
              EZ_FAILURE;
            }
          }
          else
          {
            ezLog::Error("Unknown field of '{0}.{1}'", it.Key(), it2.Key());
            return EZ_FAILURE;
          }
        }
      }
      else
      {
        ezLog::Error("Unknown json member in root object '{0}'", it.Key().GetView());
        return EZ_FAILURE;
      }
    }
    return EZ_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
    ezGlobalLog::AddLogWriter(LogInspector);

    m_stackAllocator = EZ_DEFAULT_NEW(ezStackAllocator<ezMemoryTrackingFlags::None>, "Temp Allocator", ezFoundation::GetAlignedAllocator());

    if (GetArgumentCount() < 2)
      ezLog::Error("This tool requires at leas one command-line argument: An absolute path to the top-level folder of a library.");

    // Add standard folder factory
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    ezStringBuilder sSearchDir;

    auto numArgs = GetArgumentCount();
    auto shortInclude = ezStringView("-i");
    auto longInclude = ezStringView("--includeDir");
    auto shortIgnoreFile = ezStringView("-f");
    auto longIgnoreFile = ezStringView("--ignoreFile");
    for (ezUInt32 argi = 1; argi < numArgs; argi++)
    {
      auto arg = ezStringView(GetArgument(argi));
      if (arg == shortInclude || arg == longInclude)
      {
        if (numArgs <= argi + 1)
        {
          ezLog::Error("Missing path for {0}", arg);
          return;
        }
        ezStringBuilder includeDir = GetArgument(argi + 1);
        if (includeDir == shortInclude || includeDir == longInclude || includeDir == shortIgnoreFile || includeDir == longIgnoreFile)
        {
          ezLog::Error("Missing path for {0} found {1} instead", arg, includeDir.GetView());
          return;
        }
        argi++;
        includeDir.MakeCleanPath();
        m_includeDirectories.PushBack(includeDir);
      }
      else if (arg == shortIgnoreFile || arg == longIgnoreFile)
      {
        if (numArgs <= argi + 1)
        {
          ezLog::Error("Missing path for {0}", arg);
          return;
        }
        ezStringBuilder ignoreFile = GetArgument(argi + 1);
        if (ignoreFile == shortInclude || ignoreFile == longInclude || ignoreFile == shortIgnoreFile || ignoreFile == longIgnoreFile)
        {
          ezLog::Error("Missing path for {0} found {1} instead", arg, ignoreFile.GetView());
          return;
        }
        argi++;
        ignoreFile.MakeCleanPath();
        if (ParseIgnoreFile(ignoreFile.GetView()).Failed())
          return;
      }
      else
      {
        if (sSearchDir.IsEmpty())
        {
          sSearchDir = arg;
          sSearchDir.MakeCleanPath();
        }
        else
        {
          ezLog::Error("Currently only one directory is supported for searching. Did you forget -i|--includeDir?");
        }
      }
    }

    if (!ezPathUtils::IsAbsolutePath(sSearchDir.GetData()))
      ezLog::Error("The given path is not absolute: '{0}'", sSearchDir);

    m_sSearchDir = sSearchDir;

    auto projectStart = m_sSearchDir.GetView().FindLastSubString("/");
    if (projectStart == nullptr)
    {
      ezLog::Error("Failed to parse project name from search path {0}", sSearchDir);
      return;
    }
    ezStringBuilder projectName = ezStringView(projectStart + 1, m_sSearchDir.GetView().GetEndPointer());
    projectName.ToUpper();
    m_sProjectName = projectName;

    // use such a path to write to an absolute file
    // ':abs/C:/some/file.txt"
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if (m_bHadWarnings || m_bHadSeriousWarnings || m_bHadErrors)
    {
      ezLog::Warning("There have been errors or warnings, see log for details.");
    }

    if (m_bHadErrors || m_bHadSeriousWarnings)
      SetReturnCode(2);
    else if (m_bHadWarnings)
      SetReturnCode(1);
    else
      SetReturnCode(0);

    m_stackAllocator = nullptr;

    ezGlobalLog::RemoveLogWriter(LogInspector);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  ezResult ReadEntireFile(const char* szFile, ezStringBuilder& sOut)
  {
    sOut.Clear();

    ezFileReader File;
    if (File.Open(szFile) == EZ_FAILURE)
    {
      ezLog::Error("Could not open for reading: '{0}'", szFile);
      return EZ_FAILURE;
    }

    ezDynamicArray<ezUInt8> FileContent;

    ezUInt8 Temp[4024];
    ezUInt64 uiRead = File.ReadBytes(Temp, EZ_ARRAY_SIZE(Temp));

    while (uiRead > 0)
    {
      FileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32)uiRead));

      uiRead = File.ReadBytes(Temp, EZ_ARRAY_SIZE(Temp));
    }

    FileContent.PushBack(0);

    if (!ezUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
    {
      ezLog::Error("The file \"{0}\" contains characters that are not valid Utf8. This often happens when you type special characters in "
                   "an editor that does not save the file in Utf8 encoding.",
        szFile);
      return EZ_FAILURE;
    }

    sOut = (const char*)&FileContent[0];

    return EZ_SUCCESS;
  }

  void IterateOverFiles()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    const ezUInt32 uiSearchDirLength = m_sSearchDir.GetElementCount() + 1;

    // get a directory iterator for the search directory
    ezFileSystemIterator it;
    if (it.StartSearch(m_sSearchDir.GetData(), true, false) == EZ_SUCCESS)
    {
      ezStringBuilder currentFile, sExt;

      // while there are additional files / folders
      do
      {
        // build the absolute path to the current file
        currentFile = it.GetCurrentPath();
        currentFile.AppendPath(it.GetStats().m_sName.GetData());

        // file extensions are always converted to lower-case actually
        sExt = currentFile.GetFileExtension();

        if (sExt.IsEqual_NoCase("h") || sExt.IsEqual_NoCase("inl"))
        {
          ezLog::Info("Checking: {}", currentFile);

          EZ_LOG_BLOCK("Header", &currentFile.GetData()[uiSearchDirLength]);
          CheckHeaderFile(currentFile);
          m_stackAllocator->Reset();
          continue;
        }
      } while (it.Next() == EZ_SUCCESS);
    }
    else
      ezLog::Error("Could not search the directory '{0}'", m_sSearchDir);
  }

  void CheckInclude(const ezStringBuilder& currentFile, const ezStringBuilder& includePath, ezUInt32 line)
  {
    ezStringBuilder absIncludePath(m_stackAllocator.Borrow());
    bool includeOutside = true;
    if (includePath.IsAbsolutePath())
    {
      for (auto& includeDir : m_includeDirectories)
      {
        if (includePath.StartsWith(includeDir))
        {
          includeOutside = false;
          break;
        }
      }
    }
    else
    {
      bool includeFound = false;
      if (includePath.StartsWith("ThirdParty"))
      {
        includeOutside = true;
      }
      else
      {
        for (auto& includeDir : m_includeDirectories)
        {
          absIncludePath = includeDir;
          absIncludePath.AppendPath(includePath);
          if (ezOSFile::ExistsFile(absIncludePath))
          {
            includeOutside = false;
            break;
          }
        }
      }
    }

    if (includeOutside)
    {
      ezStringBuilder includeFileLower = includePath.GetFileNameAndExtension();
      includeFileLower.ToLower();
      ezStringBuilder currentFileLower = currentFile.GetFileNameAndExtension();
      currentFileLower.ToLower();

      bool ignore = m_ignoreTarget.m_byName.Contains(includeFileLower)
        || m_ignoreSource.m_byName.Contains(currentFileLower);

      if (!ignore)
      {
        ezLog::Error(
          "Including '{0}' in {1}:{2} leaks underlying implementation details. Including system or thirdparty headers in public ez header "
          "files is not allowed. Please use an interface, factory or pimpl pattern to hide the implementation and avoid the include. See the Documentation Chapter 'General->Header Files' for details.",
          includePath.GetView(), currentFile.GetView(), line);
      }
    }
  }

  void CheckHeaderFile(const ezStringBuilder& currentFile)
  {
    ezStringBuilder fileContents(m_stackAllocator.Borrow());
    ReadEntireFile(currentFile.GetData(), fileContents);

    auto fileDir = currentFile.GetFileDirectory();

    ezStringBuilder internalMacroToken(m_stackAllocator.Borrow());
    internalMacroToken.Append("EZ_", m_sProjectName, "_INTERNAL_HEADER");
    auto internalMacroTokenView = internalMacroToken.GetView();

    ezTokenizer tokenizer(m_stackAllocator.Borrow());
    auto dataView = fileContents.GetView();
    tokenizer.Tokenize(ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(dataView.GetStartPointer()), dataView.GetElementCount()),
      ezLog::GetThreadLocalLogSystem());

    ezStringView hash("#");
    ezStringView include("include");
    ezStringView openAngleBracket("<");
    ezStringView closeAngleBracket(">");

    bool isInternalHeader = false;
    auto tokens = tokenizer.GetTokens();
    const auto numTokens = tokens.GetCount();
    for (ezUInt32 i = 0; i < numTokens; i++)
    {
      auto curToken = tokens[i];
      while (curToken.m_iType == ezTokenType::Whitespace)
      {
        curToken = tokens[++i];
      }
      if (curToken.m_iType == ezTokenType::NonIdentifier && curToken.m_DataView == hash)
      {
        do
        {
          curToken = tokens[++i];
        } while (curToken.m_iType == ezTokenType::Whitespace);

        if (curToken.m_iType == ezTokenType::Identifier && curToken.m_DataView == include)
        {
          auto includeToken = curToken;
          do
          {
            curToken = tokens[++i];
          } while (curToken.m_iType == ezTokenType::Whitespace);

          if (curToken.m_iType == ezTokenType::String1)
          {
            // #include "bla"
            ezStringBuilder absIncludePath(m_stackAllocator.Borrow());
            ezStringBuilder relativePath(m_stackAllocator.Borrow());
            relativePath = curToken.m_DataView;
            relativePath.Trim("\"");
            relativePath.MakeCleanPath();
            absIncludePath = fileDir;
            absIncludePath.AppendPath(relativePath);

            if (!ezOSFile::ExistsFile(absIncludePath))
            {
              ezLog::Error("The file '{0}' does not exist. Includes relative to the global include directories should use the #include "
                           "<path/to/file.h> syntax.",
                absIncludePath);
            }
            else if (!isInternalHeader)
            {
              CheckInclude(currentFile, absIncludePath, includeToken.m_uiLine);
            }
          }
          else if (curToken.m_iType == ezTokenType::NonIdentifier && curToken.m_DataView == openAngleBracket)
          {
            // #include <bla>
            bool error = false;
            auto startToken = curToken;
            do
            {
              curToken = tokens[++i];
              if (curToken.m_iType == ezTokenType::Newline)
              {
                ezLog::Error("Non-terminated '<' in #include {0} line {1}", currentFile.GetView(), includeToken.m_uiLine);
                error = true;
                break;
              }
            } while (curToken.m_iType != ezTokenType::NonIdentifier || curToken.m_DataView != closeAngleBracket);

            if (error)
            {
              // in case of error skip the malformed line in hopes that we can recover from the error.
              do
              {
                curToken = tokens[++i];
              } while (curToken.m_iType != ezTokenType::Newline);
            }
            else if (!isInternalHeader)
            {
              ezStringBuilder includePath(m_stackAllocator.Borrow());
              includePath = ezStringView(startToken.m_DataView.GetEndPointer(), curToken.m_DataView.GetStartPointer());
              includePath.MakeCleanPath();
              CheckInclude(currentFile, includePath, startToken.m_uiLine);
            }
          }
          else
          {
            // error
            ezLog::Error("Can not parse #include statement in {0} line {1}", currentFile.GetView(), includeToken.m_uiLine);
          }
        }
        else
        {
          while (curToken.m_iType != ezTokenType::Newline && curToken.m_iType != ezTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
      else
      {
        if (curToken.m_iType == ezTokenType::Identifier && curToken.m_DataView == internalMacroTokenView)
        {
          isInternalHeader = true;
        }
        else
        {
          while (curToken.m_iType != ezTokenType::Newline && curToken.m_iType != ezTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
    }
  }

  virtual ezApplication::ApplicationExecution Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return ezApplication::Quit;

    IterateOverFiles();

    return ezApplication::Quit;
  }
};

EZ_CONSOLEAPP_ENTRY_POINT(ezHeaderCheckApp);
