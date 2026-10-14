---
name: ez-ddl
description: Read and edit ezEngine documents on disk - scenes (.ezScene), prefabs (.ezPrefab) and assets (.ez*Asset) are OpenDDL text. Use Utilities/Scripts/ezddl.py to convert GUIDs ('{ xxxxxxxx-... }' <-> 'u4{low,high}') and hex floats (0x0000803F), resolve asset GUIDs to paths, find which documents use an asset, print a scene hierarchy, add game objects and components, set properties, validate a document, and make the editor re-transform a hand-edited asset. Triggers - ezScene, ezPrefab, ezMaterialAsset, ezTextureAsset, DDL, OpenDDL, asset GUID, Uuid u4, asset reference, hex float, edit scene file, add object to scene, which asset is this GUID, who references this asset, HeaderV2, Types section, type version.
---

# Editing ezEngine documents as text

`python Utilities/Scripts/ezddl.py <command>` (stdlib only, run from the repo root). `-h` on any command lists
its options. All commands accept GUIDs in any form: `"{ 7c71fc2d-2a1a-4938-9305-f3a0f26d73cc }"`,
`u4{14732239694908818835,5276013256709897261}` or `14732239694908818835,5276013256709897261`.

| Task | Command |
| --- | --- |
| Convert a GUID / make new ones | `guid <guid> [-r]` (`-r` names the asset), `guid new 3` |
| Hex float <-> decimal | `float 0x0000803F 1.5` |
| Which asset is this GUID / GUID of this file | `asset <guid>`, `asset Coin.ezPrefab`, `asset "*Materials/*Wood*"` |
| Who references an asset | `uses <guid or path>` (`-l` for file names only) |
| Read a document | `show <file>` (decimal floats, GUIDs as strings, asset paths), `-o <guid or Name>`, `-t "*Light*"` |
| Scene / prefab hierarchy | `tree <file> [--props] [--depth N]` |
| Validate after editing | `check <file>...` |
| Add a game object | `new-object <file> --name N --parent <guid or Name> --pos 1,2,3 -c ezPointLightComponent` |
| Set a property | `set <file> <object> <Property> <value>`, `--type` if not stored yet |
| Type version, base classes, property names, enum values | `type ezPointLightComponent`, `type --list "*Light*"` |
| A real object of a type, as a template | `example ezSpotLightComponent` |
| Make the editor re-transform after a hand edit | `touch <file>` |

Asset lookups index the data directories of the project that contains the file (from
`RuntimeConfigs/DataDirectories.ddl`); without a file they index all of `Data/`. Override with `--project` or
`--root`.

## Files or MCP?

- **Editor running with the document open** -> use MCP (`ez-mcp` skill). The editor does not reload a document
  that changes on disk; its next save silently overwrites the edit. If it must be a file edit, `document_close`
  first and `document_open` afterwards.
- **Editor not running, many documents, or mechanical changes** (rename a reference everywhere, add the same
  component to many prefabs) -> edit the files.
- `show`/`tree`/`uses`/`asset` are usually the fastest way to *understand* data even when the editor runs.

## Format facts

- Three sections: `HeaderV2` (asset info: `DocumentID`, `Hash`, `Dependencies`, `References`, `PackageDeps`),
  `Objects` (every object as `o { Uuid %id, s %t (type), u3 %v (version), p { properties } }`; the one with
  `s %n{"ObjectTree"}` is the root), `Types` (descriptors of the types used, with their versions).
- A scene's hierarchy is GUID lists: root `Children` -> game objects, each with `Children` and `Components`.
  Every object must be referenced exactly once; `check` verifies that.
- Asset references are strings: `s %Mesh{"{ guid }"}`. Some are paths (`"White.color"`,
  `"Shaders/Materials/DefaultMaterial.ezShader"`). Game object references (e.g. a constraint's `ChildActor`)
  are also GUID strings, pointing to an object in the same document.
- Floats are written as hex bytes, but the parser accepts decimals: `f %Intensity{2.5}` and
  `Vec3 %LocalPosition{f{1,2,-3}}` load fine. `Angle` is stored in radians, `Time` as double seconds,
  `ColorGamma` as `u1{r,g,b,a}`, `Color` as linear floats. Enums and bitflags are strings: `"ezFoo::Bar"`,
  `"ezFoo::A|ezFoo::B"`.
- Properties that are not written take the type's default when loaded. A new object only needs the
  properties that differ from the default.
- Order of objects and properties does not matter for loading; the editor sorts both when it saves.
- Editable prefab instances (and materials with a base material) carry `MetaFromPrefab` (the source asset),
  `MetaPrefabSeed` and `MetaBasePrefab` (the whole source document as a string). Their objects' ids are
  `prefab object id + seed` (`guid add/sub`). Do not edit `MetaBasePrefab`.

## Rules that are not obvious

- **After a hand edit run `touch`.** The asset curator compares only the header `Hash` with the transformed
  output. Edited content with an unchanged `Hash` counts as up to date and is never transformed.
- **The header is not recomputed by `touch`.** `Dependencies`/`References`/`PackageDeps` are written by the
  editor based on property attributes (a scene to load, for instance, is deliberately not a dependency).
  Usually it does not matter for a transform, but export packaging and `asset_uses` read them. For an exact
  header open the document in the editor and save it with `document_save` `force` true.
- **New GUIDs must be random and new** (`guid new`, or let `new-object` make them). Never copy an object
  block without changing its id, never derive ids by incrementing.
- **Type versions.** If the `Types` section lists a type, its version (not the object's `v`) is what loading
  uses, and graph patches run from there to the code's version. `new-object` writes the matching version and
  warns when the file is older than the code - then upgrade the file first (open and save in the editor).
  When adding an object of a type that is not in `Types` yet, `v` must be the code's version (`type <name>`).
- **Run `check` after every structural edit.** It reports parse errors with a line number and warns
  about GUID strings that are neither a known asset nor an object of the document - typos, deleted assets, or
  sub-assets (e.g. Fmod events), which the index does not know.
- Files may use CRLF; the script keeps the existing line endings. When editing with other tools, keep tabs
  for indentation.
