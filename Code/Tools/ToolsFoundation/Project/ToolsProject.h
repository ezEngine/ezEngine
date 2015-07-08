#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <ToolsFoundation/Basics/Status.h>

class EZ_TOOLSFOUNDATION_DLL ezToolsProject
{
public:
  struct Event
  {
    enum class Type
    {
      ProjectCreated,
      ProjectOpened,
      ProjectOpened2,
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
  static ezToolsProject* GetInstance() { return s_pInstance; }

  static bool IsProjectOpen() { return s_pInstance != nullptr; }
  static bool IsProjectClosing() { return (s_pInstance != nullptr && s_pInstance->m_bIsClosing); }
  static void CloseProject();
  static bool CanCloseProject();
  static ezStatus OpenProject(const char* szProjectPath);
  static ezStatus CreateProject(const char* szProjectPath);

  const ezString& GetProjectPath() const { return m_sProjectPath; }

  static ezString FindProjectForDocument(const char* szDocumentPath);

  bool IsDocumentInAllowedRoot(const char* szDocumentPath, ezString* out_RelativePath = nullptr) const;

  void AddAllowedDocumentRoot(const char* szPath);

private:

  static ezStatus CreateOrOpenProject(const char* szProjectPath, bool bCreate);

private:
  ezToolsProject(const char* szProjectPath);
  ~ezToolsProject();

  ezStatus Create();
  ezStatus Open();

  static ezToolsProject* s_pInstance;

private:
  bool m_bIsClosing;
  ezString m_sProjectPath;
  ezHybridArray<ezString, 4> m_AllowedDocumentRoots;
};



