---
name: ez-mcp
description: Drive a running ezEditor or ezPlayer/game from an agent over MCP - launch one with a port, then call tools to inspect and change a live project. Use for ezEngine tasks that need the running app rather than files on disk. Triggers - ezEditor, ezPlayer, ezEditorProcessor, MCP server, editor-mcpport, mcpport, transform asset, asset dependencies, asset browser, open document, edit scene, build a scene, game object, component, prefab, property grid, selection, reflected types, RTTI, CVar, editor action, play the game, screenshot, inject input, press key, step frames, export project, create project, build the C++ plugin.
---

# Driving ezEditor and the game over MCP

Two hosts, same protocol, **separate ports and separate tool lists**:

| Host | Started with | Answers about |
| --- | --- | --- |
| **Editor** (`ezEditor.exe`) | `-editor-mcpport <n>` | assets, documents, scene contents, reflected types, editor actions, the C++ plugin |
| **Game** (`ezPlayer.exe`, or the engine process the editor plays in) | `-mcpport <n>` | what is rendered and simulated: screenshots, input, frames, the clock |

Neither is a registered project MCP server: neither runs most of the time and the port is per run. Start
one, then talk to it. For anything the file system already answers, read the files instead.

## Launching

```shell
# editor
ezEditor.exe -project "Data/Samples/PacMan" -unattended -editor-mcpport 7399
# game, standalone - no editor needed
ezPlayer.exe -project <project-folder> -scene <path/to/Scene.ezBinScene> -profile Default -mcpport 7401
```

Binaries are in `Workspace/<workspace>-output/Bin/WinVs2026Dev64/`.

- **`-unattended` is required for the editor.** Otherwise a modal dialog during startup (e.g. "compile
  the C++ plugin?") hangs it before it serves anything. Tool calls suppress dialogs themselves; startup
  does not.
- Launch detached (`start ""` / `Start-Process`); it runs until told to quit.
- Startup takes seconds - **poll the port**, do not sleep a fixed time.
- Pick a port other than the default 7391 so a user's own editor keeps working.
- Without `-mcpport` the game starts **no** server at all.
- `-editor-mcpport` and `-mcpport` are separate names on purpose: the editor forwards its whole command
  line to its engine process, which takes `-editor-mcpport` + 1. Read that port from the editor's
  `app_info` as `engineMcpPort` rather than computing it.
- `-project` or `-createProject` keeps the dashboard away.

## Calling tools

```shell
curl -s -X POST http://127.0.0.1:7399/mcp -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"project_info","arguments":{}}}'
```

`{"jsonrpc":"2.0","id":1,"method":"tools/list"}` returns every tool with its full description and input
schema. **Read that list before guessing** - the descriptions carry the argument semantics and the
warnings, plugins can add tools, and the two hosts do not have the same list. Results are JSON in
`content[0].text`; failures carry `"isError":true` and a sentence saying what to do instead.

To register a running host for the session:
`claude mcp add --transport http ez-editor http://127.0.0.1:7399/mcp`.

## What is there

Both hosts: `app_info`, `app_ping`, `app_quit`, `app_command_line_options`, `log_read`, `log_write`,
`cvar_list`, `cvar_set`, `rtti_find_types`, `rtti_type_info`, `rtti_type_properties`,
`rtti_derived_types` - each answering for *its own* process.

| Editor only | |
| --- | --- |
| Project | `project_info`, `project_export` |
| C++ plugin | `cpp_status`, `cpp_generate`, `cpp_build` |
| Assets | `asset_types`, `asset_find`, `asset_info`, `asset_uses`, `asset_thumbnail`, `asset_transform`, `asset_health`, `asset_history`, `asset_processor`, `asset_importers`, `asset_import` |
| Documents | `document_types`, `document_list`, `document_open`, `document_create`, `document_save`, `document_close`, `document_focus`, `document_delete` |
| Contents | `object_tree`, `object_properties`, `object_modify`, `object_undo` |
| Selection | `selection_get`, `selection_set` |
| Actions | `action_list`, `action_state`, `action_execute` |

| Game only | |
| --- | --- |
| Rendering | `app_screenshot` |
| Time | `game_info`, `game_wait`, `game_pause`, `game_speed` |
| Input | `input_slots`, `input_set`, `input_sequence` |

Start with `project_info` on the editor (paths and asset states are relative to what it reports), and
with `game_info` on the game.

## Editing a document

Nothing works on a closed document, and nothing opens implicitly:

1. `asset_find` → path or guid. Its paths start with the data directory name
   (`Testing Chambers/Scenes/DynamicFog.ezScene`) - that is the form other tools want.
2. `document_open` with `path` (not `document` - see below) and `focus` false, unless the user is meant
   to look at it.
3. `object_tree` → guids. Asset documents have one top level object, so `object_properties` with only
   `document` already reads an asset's whole configuration. Scenes have many.
4. `object_properties` / `object_modify`.
5. `document_save` - nothing saves on its own, and the asset system sees nothing until it is saved.

**Which argument names a tool takes**, because guessing costs a round trip and they follow a rule:

| Argument | Tools | Why |
| --- | --- | --- |
| `path` | `document_open`, `document_create`, `document_delete` | act on a file that need not be open yet |
| `document` | `object_*`, `selection_*`, `document_save`/`_close`/`_focus`, and the `document` argument of the action tools | act on an already open document |
| `asset` | `asset_info`, `asset_transform`, `asset_thumbnail`, `asset_uses` | act on the asset database, open or not |
| `name` | `action_execute`, `action_state`, `rtti_*`, `cvar_set` | the thing named is not a document at all |

So it is `document_open` `path`, `action_execute` `name`, `rtti_derived_types` `name`. When in doubt read
the schema from `tools/list` rather than guessing - and note that a wrong name fails with a message that
says the right one.

A scene's game objects and components live in the same hierarchy, told apart by the parent property they
sit in. Building one is all `object_modify`:

```
addObject   property "Children",   type "ezGameObject"          -> child object
addObject   property "Components", type "ezPointLightComponent"  -> component on it
set         property "Name" / "LocalPosition" / any component property
moveObject  newParent <guid>    | duplicateObject | deleteObject  (each takes everything below it)
```

`rtti_derived_types` of `ezComponent` lists component types; `rtti_type_properties` gives property
names, value types and enum values before writing any. Vectors and colours go in as they come out
(`{"x":1,"y":2,"z":3}` or `[1,2,3]`). `selection_set` is worth calling even when it changes nothing - it
is how the user sees which object is meant, and `selection_get` answers "this object".

## Driving the game

`gameStateActive` false in `game_info` means nothing is being played - normal for the editor's engine
process until play-the-game runs, and why input does nothing and screenshots time out.

- Start it from the **editor** port: `action_execute` `Scene.GameMode.Play` with the scene document;
  `Scene.GameMode.Stop` ends it. Both are asynchronous (the editor asks the engine process), so `Stop`
  reads as disabled for a moment and `game_info` on the game port is the better confirmation.
- **`app_screenshot` returns a path, not the image.** Read that file. A rendered frame is the evidence a
  change works; "did not crash" is not.
- **`input_set` then `game_wait`** - input is consumed one frame at a time, so setting a slot alone does
  nothing. `input_sequence` does a whole set/wait/text/clear chain in one call. Injected values merge
  with the real keyboard, larger wins, so a human is never locked out.
- **`game_pause` stops the engine clock, not necessarily the game's own simulation.**
- `cvar_list` is worth far more here than in the editor: render passes, physics and AI visualisation are
  mostly CVars.

## Editor actions

`action_execute` runs global actions by name. Actions of scope `Document` or `Window` need a `document`
argument (guid or path, already open); so do `action_list` and `action_state`, and that is the only way
to read their per-document enabled state. Passing `document` to `action_list` also flips `globalOnly`
off, so `{"contains":"GameMode","document":"…","includeState":true}` is the usual way to find them.
`Window` actions additionally need a window (`document_open` with `focus` true).

## C++ plugin

`cpp_status` first: `hasCppProject` false means no C++ code at all and `cpp_generate` creates it;
`compilerUsable` false means CMake will refuse and `cpp_generate` with `useSdkCompiler` is the fix.
After editing sources call `cpp_build`, with `force` when files were added, removed or renamed (CMake
collects them at generation time). Both return the full build log - on failure it is the only place the
compiler errors are. Then check `pluginBinary` / `pluginBinaryExists`: a build that reports success
without producing the library means the plugin is not in use. Headless equivalent:
`ezEditorProcessor.exe -project <path> -compile` (`-recompile` forces CMake too).

## Creating a project

No tool for it - a different set of plugins is loaded, so it happens at launch:

```shell
ezEditor.exe -createProject "C:/Projects/MyGame" -projectTemplate "Basic FPS" -unattended -editor-mcpport 7399
```

The directory must be empty or absent; the project opens right away. Without `-projectTemplate` the
project is blank, using `-pluginTemplate` (default `General3D`). `-listTemplates` logs the valid names.
`ezEditorProcessor.exe` takes the same options and quits when done - better when no editor is wanted
afterwards.

## Rules that are not obvious

- **Every call blocks its host.** One at a time, on the main thread. No progress, no cancel.
- **Some calls run for minutes**: `asset_transform` over a project, `project_export`, `cpp_build`,
  `cpp_generate`. Give curl a timeout in tens of minutes.
- **`app_ping` tells "busy" from "hung".** It answers only when the main thread is free, so silence
  during a long call is expected and silence afterwards is not. Take the process id from `app_info` up
  front - a hung host must be killed by pid, and `app_quit` will not get through either.
- **`log_read` only sees its own process.** The editor cannot see what the engine logged, or vice versa.
  `app_info` reports `engineLogFilePattern` for when that process has died.
- **A failed assert comes back as a tool error** with file, line and expression. That is an engine bug,
  not bad arguments - retrying will not help. The host keeps answering but has continued past a check
  meant to stop it, so restart it before trusting anything further, and report the text.
- **Check `app_info`'s `buildTimestamp` before concluding a tool is missing.** A binary built before a
  feature returns the same `tools/list` as one that never had it.
- **`cvar_set` on a `requiresRestart` CVar does not change what the engine reads.** The response returns
  `pendingValue` separately; re-reading keeps showing the old `value`, and that is not a failed write.
- **Quit before rebuilding**: `app_quit` (with `discardChanges` if documents are modified), or the plugin
  DLLs stay locked and the build fails.
- **Nothing saves implicitly.** `object_modify` only marks the document modified; `object_undo` reverts.
