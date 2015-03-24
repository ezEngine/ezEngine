#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Communication/Event.h>
#include <ToolsFoundation/Basics/Status.h>

class EZ_EDITORFRAMEWORK_DLL ezEditorProject
{
public:
  struct Event
  {
    enum class Type
    {
      ProjectCreated,
      ProjectOpened,
      ProjectClosing,
      ProjectClosed,
    };

    Type m_Type;
  };

  struct Request
  {
    enum class Type
    {
      CanProjectClose,
    };

    Type m_Type;
    bool m_bProjectCanClose; // when the event 'CanProjectClose' is sent, interested code can set this to false to prevent project closing
  };

  static ezEvent<const Event&> s_Events;
  static ezEvent<Request&> s_Requests;

public:
  static ezEditorProject* GetInstance() { return s_pInstance; }

  static bool IsProjectOpen() { return s_pInstance != nullptr; }
  static void CloseProject();
  static bool CanCloseProject();
  static ezStatus OpenProject(const char* szProjectPath);
  static ezStatus CreateProject(const char* szProjectPath);

  const ezString& GetProjectPath() const { return m_sProjectPath; }

  static ezString FindProjectForDocument(const char* szDocumentPath);

  bool IsDocumentInProject(const char* szDocumentPath, ezString* out_RelativePath = nullptr) const;

private:

  static ezStatus CreateOrOpenProject(const char* szProjectPath, bool bCreate);

private:
  ezEditorProject(const char* szProjectPath);
  ~ezEditorProject();

  ezStatus Create();
  ezStatus Open();

  static ezEditorProject* s_pInstance;

private:
  ezString m_sProjectPath;
};



