#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <EditorPluginMcp/McpTools/ActionTool.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/McpTranslation.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Document/Document.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpActionTool, 1, ezRTTIDefaultAllocator<ezMcpActionTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  constexpr ezUInt32 uiMaxActions = 200;

  ezStringView ToString(ezActionScope::Enum scope)
  {
    switch (scope)
    {
      case ezActionScope::Global:
        return "Global";
      case ezActionScope::Document:
        return "Document";
      case ezActionScope::Window:
        return "Window";
    }

    return "Unknown";
  }

  ezStringView ToString(ezActionType::Enum type)
  {
    switch (type)
    {
      case ezActionType::Action:
        return "Action";
      case ezActionType::Category:
        return "Category";
      case ezActionType::Menu:
        return "Menu";
      case ezActionType::ActionAndMenu:
        return "ActionAndMenu";
    }

    return "Unknown";
  }

  /// Returns a live instance of an action, or nullptr.
  ///
  /// Global actions exist once per window they are mapped into, and all of those agree, so the first one
  /// is as good as any. Document and window actions exist once per document window they are mapped into,
  /// and each of those reports something different, so one is only picked when a document says which.
  ///
  /// Reading an existing instance rather than creating one matters twice over: creating an action has
  /// side effects (it registers event handlers, and some constructors touch the editor), and a fresh
  /// instance would not have received the updates that keep m_bEnabled current.
  const ezAction* GetLiveInstance(const ezActionDescriptor& desc, const ezDocument* pDocument)
  {
    const ezArrayPtr<ezAction* const> instances = desc.GetCreatedActions();

    if (desc.m_Scope == ezActionScope::Global)
      return instances.IsEmpty() ? nullptr : instances[0];

    if (pDocument == nullptr)
      return nullptr;

    for (const ezAction* pAction : instances)
    {
      if (pAction->GetContext().m_pDocument == pDocument)
        return pAction;
    }

    return nullptr;
  }

  /// Whether the action would let itself be triggered. True for the types that have no such flag.
  bool IsActionEnabled(const ezAction* pAction)
  {
    if (const ezButtonAction* pButton = ezDynamicCast<const ezButtonAction*>(pAction))
      return pButton->IsEnabled();

    if (const ezDynamicActionAndMenuAction* pMenu = ezDynamicCast<const ezDynamicActionAndMenuAction*>(pAction))
      return pMenu->IsEnabled();

    if (const ezSliderAction* pSlider = ezDynamicCast<const ezSliderAction*>(pAction))
      return pSlider->IsEnabled();

    return true;
  }

  /// Writes 'enabled', and 'checkable'/'checked' where they apply.
  ///
  /// Everything that cannot be determined is written as the string "unknown" rather than omitted, so
  /// that a caller can tell 'not applicable or not known' apart from 'false'. That happens for
  /// document actions when no document was named, for actions that are not currently mapped into any
  /// open window, and for action types that carry no enabled flag at all (categories and plain menus).
  void WriteActionState(ezMcpJsonWriter& ref_writer, const ezActionDescriptor& desc, const ezDocument* pDocument)
  {
    const ezAction* pAction = GetLiveInstance(desc, pDocument);

    if (pAction == nullptr)
    {
      ref_writer.AddVariableString("enabled", "unknown");

      if (desc.m_Scope != ezActionScope::Global && pDocument == nullptr)
      {
        ref_writer.AddVariableString("stateReason", "This action's state depends on a document. Pass 'document' to read it.");
      }
      else
      {
        ref_writer.AddVariableString("stateReason", "No instance of this action exists right now, so it is not mapped into any open window.");
      }

      return;
    }

    if (const ezButtonAction* pButton = ezDynamicCast<const ezButtonAction*>(pAction))
    {
      ref_writer.AddVariableBool("enabled", pButton->IsEnabled());
      ref_writer.AddVariableBool("visible", pButton->IsVisible());

      // only meaningful for toggles, and reporting 'checked:false' for every push button would suggest
      // a state that does not exist
      if (pButton->IsCheckable())
      {
        ref_writer.AddVariableBool("checkable", true);
        ref_writer.AddVariableBool("checked", pButton->IsChecked());
      }
    }
    else if (const ezDynamicActionAndMenuAction* pMenu = ezDynamicCast<const ezDynamicActionAndMenuAction*>(pAction))
    {
      ref_writer.AddVariableBool("enabled", pMenu->IsEnabled());
      ref_writer.AddVariableBool("visible", pMenu->IsVisible());
    }
    else if (const ezSliderAction* pSlider = ezDynamicCast<const ezSliderAction*>(pAction))
    {
      ref_writer.AddVariableBool("enabled", pSlider->IsEnabled());
      ref_writer.AddVariableBool("visible", pSlider->IsVisible());
      ref_writer.AddVariableInt32("value", pSlider->GetValue());

      ezInt32 iMin = 0, iMax = 0;
      pSlider->GetRange(iMin, iMax);
      ref_writer.AddVariableInt32("minValue", iMin);
      ref_writer.AddVariableInt32("maxValue", iMax);
    }
    else
    {
      ref_writer.AddVariableString("enabled", "unknown");
      ref_writer.AddVariableString("stateReason", "This action type has no enabled state.");
    }
  }

  /// Writes the strings a user would see for this action, skipping those that translate to nothing.
  ///
  /// All three lookups are keyed on the action name.
  void WriteTranslations(ezMcpJsonWriter& ref_writer, const ezActionDescriptor& desc)
  {
    const ezStringView sName = desc.m_sActionName;

    ezMcpTranslation::AddOptionalString(ref_writer, "displayName", ezMcpTranslation::GetDisplayName(sName));
    ezMcpTranslation::AddOptionalString(ref_writer, "tooltip", ezMcpTranslation::GetTooltip(sName));
    ezMcpTranslation::AddOptionalString(ref_writer, "helpUrl", ezMcpTranslation::GetHelpURL(sName));
  }
} // namespace

void ezMcpActionTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  ezMcpToolDesc& list = out_tools.ExpandAndGetRef();
  list.m_sName = "action_list";
  list.m_sDescription = "Lists the editor's actions - the commands behind its menu entries and toolbar buttons - with their "
                        "internal name, category, keyboard shortcut and the text the user sees for them. Use it to find out "
                        "what the editor can do and what to pass to action_execute, or to explain a feature to a user in the "
                        "wording that actually appears in their UI. Actions of scope 'Global' can be executed as they are; "
                        "'Document' and 'Window' ones act on one document and need its guid or path passed along, so pass "
                        "'document' here to list and inspect those.";
  list.m_sInputSchema = R"({"type":"object","properties":{)"
                        R"("contains":{"type":"string","description":"Only list actions whose internal name or category contains this text, case insensitive."},)"
                        R"("matchTranslation":{"type":"boolean","description":"Also match 'contains' against the translated display name and tooltip, i.e. the words the user sees. Useful when searching for a feature by how it is described rather than by its internal name. Default false."},)"
                        R"("document":{"type":"string","description":"Guid or path of an open document. Makes 'globalOnly' default to false, and is what the state of document and window actions is read from."},)"
                        R"("globalOnly":{"type":"boolean","description":"Only list actions of scope 'Global'. Defaults to true, or to false when 'document' is given."},)"
                        R"("includeMenus":{"type":"boolean","description":"Also list menus, which only group other entries and cannot be executed. Default false. Turn this on to describe the structure of the editor's menu bar to a user, not to find something to run."},)"
                        R"("includeState":{"type":"boolean","description":"Include the current enabled/checked state of each action. Default false, because it makes the result considerably longer."}}})";

  ezMcpToolDesc& state = out_tools.ExpandAndGetRef();
  state.m_sName = "action_state";
  state.m_sDescription = "Reports whether one action is currently enabled, and whether it is checked if it is a toggle. Read "
                         "from the live action as the UI shows it, so it is accurate at the moment of the call - but it is "
                         "only a hint, since anything that happens afterwards can change it. 'enabled' comes back as the "
                         "string \"unknown\" when it cannot be determined, which is the case for actions not currently "
                         "present in any open window, and for document actions when no 'document' was passed.";
  state.m_sInputSchema = R"({"type":"object","properties":{)"
                         R"("name":{"type":"string","description":"The internal action name, e.g. 'Document.Save' or 'Editor.Project.Settings'. Get it from action_list."},)"
                         R"("category":{"type":"string","description":"The category the action is registered in. Only needed when the same name exists in several categories, which action_list reports."},)"
                         R"("document":{"type":"string","description":"Guid or path of an open document. Required for actions of scope 'Document' or 'Window', whose state is per document."}},)"
                         R"("required":["name"]})";

  ezMcpToolDesc& exec = out_tools.ExpandAndGetRef();
  exec.m_sName = "action_execute";
  exec.m_sDescription = "Triggers one action, exactly as clicking its menu entry or toolbar button would. Refuses and "
                        "reports 'wasDisabled' if the action is currently disabled, rather than invoking something that the "
                        "UI would not let a user invoke. Actions of scope 'Document' or 'Window' act on one document and "
                        "need 'document' passed; that document has to be open, and for window actions it also needs a "
                        "window, i.e. it must have been opened with focus.\n"
                        "This is how play-the-game is driven: 'Scene.GameMode.Play' starts it and 'Scene.GameMode.Stop' "
                        "ends it, both on the scene document. The game then runs in the editor's engine process, which "
                        "serves its own MCP port (see the editor's app_info) for screenshots, input and frames.\n"
                        "Many actions open a dialog for the user to fill in. Those are closed unanswered here, which means the "
                        "action did nothing - the result then lists them under 'suppressedDialogs', so check that field before "
                        "assuming the action took effect. Some operations behind such a dialog have their own tool, e.g. "
                        "project_export.\n"
                        "IMPORTANT - this call runs synchronously. Every action the editor currently has was checked not to "
                        "block, but a modal window that slips through would stop the editor for good: this call never returns, "
                        "app_quit cannot get through either, and the only way out is killing the process. So: always call this "
                        "with a timeout of about 30 seconds rather than waiting indefinitely, know the process id from app_info "
                        "or app_ping beforehand, and if the timeout expires use app_ping to check - a responsive editor means "
                        "the action is merely slow, an unresponsive one has to be killed by process id.";
  exec.m_sInputSchema = R"({"type":"object","properties":{)"
                        R"("name":{"type":"string","description":"The internal action name, as reported by action_list."},)"
                        R"("category":{"type":"string","description":"The category the action is registered in. Only needed to disambiguate a name that exists in several categories."},)"
                        R"("document":{"type":"string","description":"Guid or path of an open document to run the action on. Required for actions of scope 'Document' or 'Window', ignored for global ones."},)"
                        R"("value":{"description":"Optional value passed to the action. Most actions ignore it; sliders and toggles use it."},)"
                        R"("force":{"type":"boolean","description":"Execute even when the action reports itself as disabled. Default false. The action may misbehave, since it is being invoked in a state its UI would prevent."}},)"
                        R"("required":["name"]})";
}

void ezMcpActionTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "action_list")
  {
    ExecuteList(arguments, out_result);
  }
  else if (sToolName == "action_state")
  {
    ExecuteState(arguments, out_result);
  }
  else if (sToolName == "action_execute")
  {
    ExecuteAction(arguments, out_result);
  }
}

const ezActionDescriptor* ezMcpActionTool::FindAction(ezStringView sName, ezStringView sCategory, bool& out_bAmbiguous)
{
  out_bAmbiguous = false;

  const ezActionDescriptor* pFound = nullptr;

  for (auto it = ezActionManager::GetActionIterator(); it.IsValid(); ++it)
  {
    const ezActionDescriptor* pDesc = it.Value();

    if (pDesc->m_sActionName != sName)
      continue;

    if (!sCategory.IsEmpty() && pDesc->m_sCategoryPath != sCategory)
      continue;

    if (pFound != nullptr)
    {
      // a name registered in several categories - reported rather than silently resolved, because the
      // two may do entirely different things
      out_bAmbiguous = true;
      return nullptr;
    }

    pFound = pDesc;
  }

  return pFound;
}

void ezMcpActionTool::ExecuteList(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sContains = ezMcpJson::GetString(arguments, "contains");
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");
  const bool bMatchTranslation = ezMcpJson::GetBool(arguments, "matchTranslation", false);
  const bool bIncludeMenus = ezMcpJson::GetBool(arguments, "includeMenus", false);
  const bool bIncludeState = ezMcpJson::GetBool(arguments, "includeState", false);

  ezDocument* pDocument = nullptr;

  if (!sDocument.IsEmpty())
  {
    pDocument = ezMcpDocument::Find(sDocument);

    if (pDocument == nullptr)
    {
      ezMcpDocument::SetNotOpenError(out_result, sDocument);
      return;
    }
  }

  // naming a document is only ever done to get at that document's actions, so listing global ones
  // exclusively would answer a question that was not asked
  const bool bGlobalOnly = ezMcpJson::GetBool(arguments, "globalOnly", pDocument == nullptr);

  ezUInt32 uiTotalMatches = 0;
  ezUInt32 uiReturned = 0;

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.BeginArray("actions");

  for (auto it = ezActionManager::GetActionIterator(); it.IsValid(); ++it)
  {
    const ezActionDescriptor* pDesc = it.Value();

    if (bGlobalOnly && pDesc->m_Scope != ezActionScope::Global)
      continue;

    // categories are pure grouping for the shortcut dialog, they are never executable and would only
    // pad the list
    if (pDesc->m_Type == ezActionType::Category)
      continue;

    // Menus are half of all global entries and none of them can be executed, so they are left out
    // unless asked for. 'ActionAndMenu' is kept either way - it is a button that also has a menu, and
    // the button part does work.
    if (!bIncludeMenus && pDesc->m_Type == ezActionType::Menu)
      continue;

    if (!sContains.IsEmpty())
    {
      bool bMatches = pDesc->m_sActionName.FindSubString_NoCase(sContains) != nullptr ||
                      pDesc->m_sCategoryPath.FindSubString_NoCase(sContains) != nullptr;

      if (!bMatches && bMatchTranslation)
      {
        const ezStringView sName = pDesc->m_sActionName;
        const ezStringBuilder sDisplay = ezMcpTranslation::GetDisplayName(sName);
        const ezStringBuilder sTooltip = ezMcpTranslation::GetTooltip(sName);

        bMatches = sDisplay.FindSubString_NoCase(sContains) != nullptr || sTooltip.FindSubString_NoCase(sContains) != nullptr;
      }

      if (!bMatches)
        continue;
    }

    ++uiTotalMatches;

    if (uiReturned >= uiMaxActions)
      continue;

    writer.BeginObject();

    writer.AddVariableString("name", pDesc->m_sActionName);

    if (!pDesc->m_sCategoryPath.IsEmpty())
    {
      writer.AddVariableString("category", pDesc->m_sCategoryPath);
    }

    writer.AddVariableString("scope", ToString(pDesc->m_Scope));

    // 'Action' is the overwhelming majority, so only the exceptions are worth the tokens
    if (pDesc->m_Type != ezActionType::Action)
    {
      writer.AddVariableString("type", ToString(pDesc->m_Type));
    }

    if (!pDesc->m_sShortcut.IsEmpty())
    {
      writer.AddVariableString("shortcut", pDesc->m_sShortcut);
    }

    WriteTranslations(writer, *pDesc);

    if (bIncludeState)
    {
      WriteActionState(writer, *pDesc, pDocument);
    }

    writer.EndObject();

    ++uiReturned;
  }

  writer.EndArray();

  writer.AddVariableUInt32("totalMatches", uiTotalMatches);
  writer.AddVariableUInt32("returned", uiReturned);
  writer.AddVariableBool("truncated", uiTotalMatches > uiReturned);

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpActionTool::ExecuteState(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sName = ezMcpJson::GetString(arguments, "name");
  const ezStringView sCategory = ezMcpJson::GetString(arguments, "category");

  if (sName.IsEmpty())
  {
    out_result.SetError("No action name given. Call action_list to find one.");
    return;
  }

  bool bAmbiguous = false;
  const ezActionDescriptor* pDesc = FindAction(sName, sCategory, bAmbiguous);

  if (pDesc == nullptr)
  {
    ezStringBuilder sMsg;

    if (bAmbiguous)
    {
      sMsg.SetFormat("The action '{}' exists in more than one category. Pass 'category' as well, see action_list.", sName);
    }
    else
    {
      sMsg.SetFormat("No action named '{}'. Call action_list to find the correct name.", sName);
    }

    out_result.SetError(sMsg);
    return;
  }

  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = nullptr;

  if (!sDocument.IsEmpty())
  {
    pDocument = ezMcpDocument::Find(sDocument);

    if (pDocument == nullptr)
    {
      ezMcpDocument::SetNotOpenError(out_result, sDocument);
      return;
    }
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("name", pDesc->m_sActionName);
  writer.AddVariableString("category", pDesc->m_sCategoryPath);
  writer.AddVariableString("scope", ToString(pDesc->m_Scope));

  WriteActionState(writer, *pDesc, pDocument);
  WriteTranslations(writer, *pDesc);

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpActionTool::ExecuteAction(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sName = ezMcpJson::GetString(arguments, "name");
  const ezStringView sCategory = ezMcpJson::GetString(arguments, "category");
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");
  const bool bForce = ezMcpJson::GetBool(arguments, "force", false);

  if (sName.IsEmpty())
  {
    out_result.SetError("No action name given. Call action_list to find one.");
    return;
  }

  bool bAmbiguous = false;
  const ezActionDescriptor* pDesc = FindAction(sName, sCategory, bAmbiguous);

  if (pDesc == nullptr)
  {
    ezStringBuilder sMsg;

    if (bAmbiguous)
    {
      sMsg.SetFormat("The action '{}' exists in more than one category. Pass 'category' as well, see action_list.", sName);
    }
    else
    {
      sMsg.SetFormat("No action named '{}'. Call action_list to find the correct name.", sName);
    }

    out_result.SetError(sMsg);
    return;
  }

  // categories and plain menus have an Execute() that does nothing, so calling them would report
  // success while achieving nothing at all
  if (pDesc->m_Type == ezActionType::Category || pDesc->m_Type == ezActionType::Menu)
  {
    ezStringBuilder sMsg;
    sMsg.SetFormat("'{}' is a '{}', which only groups other actions and does nothing when triggered.", sName, ToString(pDesc->m_Type));

    out_result.SetError(sMsg);
    return;
  }

  // Document and window actions do their work on one document, and the descriptor alone does not say
  // which. Without it the action would be constructed with a null document and dereference it right
  // away, so this is refused rather than left to crash.
  ezDocument* pDocument = nullptr;

  if (!sDocument.IsEmpty())
  {
    pDocument = ezMcpDocument::Find(sDocument);

    if (pDocument == nullptr)
    {
      ezMcpDocument::SetNotOpenError(out_result, sDocument);
      return;
    }
  }
  else if (pDesc->m_Scope != ezActionScope::Global)
  {
    ezStringBuilder sMsg;
    sMsg.SetFormat("'{}' has scope '{}', so it acts on one document. Pass 'document' with the guid or path of an open document, "
                   "see document_list.",
      sName, ToString(pDesc->m_Scope));

    out_result.SetError(sMsg);
    return;
  }

  // Window actions read their widget out of the context, so a document that was opened without a window
  // cannot serve them. The window is also handed to document actions, because it costs nothing and some
  // of them use it for dialogs.
  QWidget* pWindow = (pDocument != nullptr) ? ezQtDocumentWindow::FindWindowByDocument(pDocument) : nullptr;

  if (pDesc->m_Scope == ezActionScope::Window && pWindow == nullptr)
  {
    ezStringBuilder sMsg;
    sMsg.SetFormat("'{}' has scope 'Window', but '{}' has no window - it was opened without focus. Call document_open with "
                   "focus true first.",
      sName, sDocument);

    out_result.SetError(sMsg);
    return;
  }

  // Checked against a live instance before doing anything, because ezActionManager::ExecuteAction()
  // does not: it creates an action and calls Execute() on it regardless of the enabled flag. Invoking a
  // disabled action is what the UI exists to prevent, and for some actions it would assert or act on
  // state that isn't there.
  if (const ezAction* pExisting = GetLiveInstance(*pDesc, pDocument))
  {
    if (!IsActionEnabled(pExisting) && !bForce)
    {
      ezMcpJsonWriter writer;
      writer.BeginObject();
      writer.AddVariableBool("executed", false);
      writer.AddVariableBool("wasDisabled", true);
      writer.AddVariableString("reason",
        "The action is currently disabled, so it was not executed. Whatever it needs is not available right now - for many "
        "actions that means no document is open or nothing is selected. Pass force=true to run it anyway.");
      writer.EndObject();

      // an error, so that this cannot be mistaken for the action having run
      out_result.m_sText = writer.GetResult();
      out_result.m_bIsError = true;
      return;
    }
  }

  const ezVariant* pValue = nullptr;
  arguments.TryGetValue("value", pValue);

  ezActionContext context;
  context.m_pDocument = pDocument;
  context.m_pWindow = pWindow;

  ezAction* pAction = pDesc->CreateAction(context);

  if (pAction == nullptr)
  {
    out_result.SetError(ezStringBuilder("Failed to create the action '", sName, "'."));
    return;
  }

  // The call runs with ezQtScopedUnattended active (see ezMcpToolRegistry::Execute), so an action that
  // opens one of the editor's own dialogs gets it rejected instead of entering a nested event loop that
  // nobody would ever leave. It can still hang on a modal window that goes around ezQtDialog - a
  // QFileDialog, for instance - which is what the timeout in the tool description is for.
  pAction->Execute(pValue != nullptr ? *pValue : ezVariant());

  pDesc->DeleteAction(pAction);

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableBool("executed", true);
  writer.AddVariableString("name", pDesc->m_sActionName);

  if (pDocument != nullptr)
  {
    writer.AddVariableString("document", pDocument->GetDocumentPath());
  }

  // Actions report nothing back - ezAction::Execute() returns void - so there is no result to pass on
  // and 'executed' only means that it was invoked without anything going wrong on the way there. What
  // it did has to be observed elsewhere, e.g. through log_read.
  writer.AddVariableString("note", "Actions do not return a result. Check log_read or query the editor state to see what happened.");

  // Only written when something was actually suppressed: for the majority of actions this stays empty,
  // and an always present empty array would suggest the field is worth looking at every time.
  const ezArrayPtr<const ezString> suppressed = ezQtUiServices::GetSuppressedDialogs();

  if (!suppressed.IsEmpty())
  {
    writer.BeginArray("suppressedDialogs");
    for (const ezString& s : suppressed)
    {
      writer.WriteString(s);
    }
    writer.EndArray();

    writer.AddVariableString("suppressedDialogsNote",
      "This action wanted user input, which is not available here, so the listed dialogs were closed unanswered. Whatever they "
      "would have configured did not happen, and the action most likely did nothing. Look for a dedicated tool for this "
      "operation, or ask the user to perform it in the editor.");
  }

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}
