#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezToolsProject;
class ezDocument;

struct ezToolsProjectEvent
{
  enum class Type
  {
    ProjectCreated,
    ProjectOpened,
    ProjectFirstSetup,
    ProjectSaveState,
    ProjectClosing,
    ProjectClosed,
    ProjectConfigChanged, ///< Sent when global project configuration data was changed and thus certain menus would need to update their content (or
                          ///< just deselect any item, forcing the user to reselect and thus update state)
    SaveAll,              ///< When sent, this shall save all outstanding modifications
  };

  ezToolsProject* m_pProject;
  Type m_Type;
};

struct ezToolsProjectRequest
{
  ezToolsProjectRequest();

  enum class Type
  {
    CanCloseProject,        ///< Can we close the project? Listener needs to set m_bCanClose if not.
    CanCloseDocuments,      ///< Can we close the documents in m_Documents? Listener needs to set m_bCanClose if not.
    SuggestContainerWindow, ///< m_Documents contains one element that a container window should be suggested for and written to
                            ///< m_iContainerWindowUniqueIdentifier.
    GetPathForDocumentGuid,
  };

  Type m_Type;
  bool m_bCanClose;                        ///< When the event is sent, interested code can set this to false to prevent closing.
  ezDynamicArray<ezDocument*> m_Documents; ///< In case of 'CanCloseDocuments', these will be the documents in question.
  ezInt32
    m_iContainerWindowUniqueIdentifier;    ///< In case of 'SuggestContainerWindow', the ID of the container to be used for the docs in m_Documents.

  ezUuid m_documentGuid;
  ezStringBuilder m_sAbsDocumentPath;
};

class EZ_TOOLSFOUNDATION_DLL ezToolsProject
{
  EZ_DECLARE_SINGLETON(ezToolsProject);

public:
  static ezEvent<const ezToolsProjectEvent&, ezMutex> s_Events;
  static ezEvent<ezToolsProjectRequest&> s_Requests;

public:
  static bool IsProjectOpen() { return GetSingleton() != nullptr; }
  static bool IsProjectClosing() { return (GetSingleton() != nullptr && GetSingleton()->m_bIsClosing); }
  static void CloseProject();
  static void SaveProjectState();
  /// Returns true when the project can be closed. Uses ezToolsProjectRequest::Type::CanCloseProject event.
  static bool CanCloseProject();
  /// Returns true when the given list of documents can be closed. Uses ezToolsProjectRequest::Type::CanCloseDocuments event.
  static bool CanCloseDocuments(ezArrayPtr<ezDocument*> documents);
  /// Returns the unique ID of the container window this document should use for its window. Uses
  /// ezToolsProjectRequest::Type::SuggestContainerWindow event.
  static ezInt32 SuggestContainerWindow(ezDocument* pDoc);
  /// Resolve document GUID into an absolute path.
  ezStringBuilder GetPathForDocumentGuid(const ezUuid& guid);
  static ezStatus OpenProject(ezStringView sProjectPath);
  static ezStatus CreateProject(ezStringView sProjectPath);

  /// Broadcasts the SaveAll event, though otherwise has no direct effect.
  static void BroadcastSaveAll();

  /// Sent when global project configuration data was changed and thus certain menus would need to update their content (or just deselect any
  /// item, forcing the user to reselect and thus update state)
  static void BroadcastConfigChanged();

  /// Returns the path to the 'ezProject' file
  const ezString& GetProjectFile() const { return m_sProjectPath; }

  /// Returns the short name of the project (extracted from the path).
  ///
  /// \param bSanitize Whether to replace whitespace and other problematic characters, such that it can be used in code.
  const ezString GetProjectName(bool bSanitize) const;

  /// Returns the path in which the 'ezProject' file is stored
  ezString GetProjectDirectory() const;

  /// Returns the directory path in which project settings etc. should be stored
  ezString GetProjectDataFolder() const;

  /// Starts at the  given document and then searches the tree upwards until it finds an ezProject file.
  static ezString FindProjectDirectoryForDocument(ezStringView sDocumentPath);

  bool IsDocumentInAllowedRoot(ezStringView sDocumentPath, ezString* out_pRelativePath = nullptr) const;

  void AddAllowedDocumentRoot(ezStringView sPath);

  /// Makes sure the given sub-folder exists inside the project directory
  void CreateSubFolder(ezStringView sFolder) const;

private:
  static ezStatus CreateOrOpenProject(ezStringView sProjectPath, bool bCreate);

private:
  ezToolsProject(ezStringView sProjectPath);
  ~ezToolsProject();

  ezStatus Create();
  ezStatus Open();

private:
  bool m_bIsClosing;
  ezString m_sProjectPath;
  ezHybridArray<ezString, 4> m_AllowedDocumentRoots;
};
