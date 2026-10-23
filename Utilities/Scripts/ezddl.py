#!/usr/bin/env python3
"""Inspect and edit ezEngine DDL documents (scenes, prefabs, assets) as text.

The editor stores documents (*.ezScene, *.ezPrefab, *.ez*Asset) as OpenDDL with three top level sections:
'HeaderV2' (an ezAssetDocumentInfo named "Header"), 'Objects' (every document object as an 'o' block, one of
them named "ObjectTree" is the root) and 'Types' (descriptors of the reflected types used, for versioning).

Encodings that are easy to get wrong by hand, and which this script handles:

- A Uuid is stored as 'Uuid{u4{<low>,<high>}}' (two decimal uint64). Asset references are strings in Windows
  GUID notation, '"{ 7c71fc2d-2a1a-4938-9305-f3a0f26d73cc }"', which is the 16 bytes of the ezUuid in memory
  (high first, then low, both little endian) printed as Data1-Data2-Data3-Data4.
- float/double values are written as '0x' + the bytes in memory order (little endian), e.g. 1.0f = 0x0000803F.
  Zero is written as '0'. The parser also accepts decimal literals ('f{1.5,-2}'), so hand-written decimals load
  correctly; the editor writes hex again on the next save.
- 'u3 %v{N}' on each object is the version of its type. If the 'Types' section lists that type, the version
  recorded there replaces 'v' while loading, and graph patches run from that version up to the one in the code.
  A new object therefore has to match the Types section, or, for a type not listed there, the code.
- The asset curator reads only the header to decide whether an asset needs to be transformed. A text edit that
  leaves 'Hash' unchanged is not picked up. See the 'touch' command.

Run 'ezddl.py <command> -h' for the arguments of each command. Only the Python standard library is used.
"""

import argparse
import json
import os
import random
import re
import struct
import sys
import uuid as pyuuid

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SDK_ROOT = os.path.normpath(os.path.join(SCRIPT_DIR, "..", ".."))

U64 = (1 << 64) - 1

# ---------------------------------------------------------------------------------------------------------------
# Uuid encoding
# ---------------------------------------------------------------------------------------------------------------

_RE_GUID_STR = re.compile(r"\{?\s*([0-9a-fA-F]{8})-([0-9a-fA-F]{4})-([0-9a-fA-F]{4})-([0-9a-fA-F]{4})-([0-9a-fA-F]{12})\s*\}?")
_RE_GUID_U4 = re.compile(r"(?:Uuid\s*)?\{?\s*(?:u4|unsigned_int64)?\s*\{?\s*(\d+)\s*,\s*(\d+)\s*\}?\s*\}?")


def guid_to_string(low, high):
    """(low, high) as stored in 'u4{low,high}' -> '{ xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx }'."""
    raw = struct.pack("<QQ", high & U64, low & U64)
    return "{ " + str(pyuuid.UUID(bytes_le=raw)) + " }"


def string_to_guid(text):
    """'{ xxxxxxxx-... }' (braces and spaces optional) -> (low, high)."""
    m = _RE_GUID_STR.fullmatch(text.strip())
    if not m:
        raise ValueError("not a GUID string: %r" % text)
    raw = pyuuid.UUID("-".join(m.groups())).bytes_le
    high, low = struct.unpack("<QQ", raw)
    return low, high


def parse_guid(text):
    """Accepts the string form, 'u4{low,high}', 'Uuid{u4{low,high}}' or 'low,high'. Returns (low, high)."""
    text = text.strip()
    if _RE_GUID_STR.fullmatch(text):
        return string_to_guid(text)
    m = _RE_GUID_U4.fullmatch(text)
    if m:
        return int(m.group(1)), int(m.group(2))
    raise ValueError("cannot interpret %r as a GUID (expected '{ xxxxxxxx-xxxx-... }', 'u4{low,high}' or 'low,high')" % text)


def guid_to_u4(low, high):
    return "u4{%d,%d}" % (low, high)


def new_guid():
    """Random (low, high), like ezUuid::MakeUuid()."""
    while True:
        low, high = random.getrandbits(64), random.getrandbits(64)
        if low or high:
            return low, high


def guid_add(a, b):
    """ezUuid::CombineWithSeed - objects of an editable prefab instance have 'prefab object id + seed'."""
    return (a[0] + b[0]) & U64, (a[1] + b[1]) & U64


def guid_sub(a, b):
    """ezUuid::RevertCombinationWithSeed."""
    return (a[0] - b[0]) & U64, (a[1] - b[1]) & U64


# ---------------------------------------------------------------------------------------------------------------
# float encoding
# ---------------------------------------------------------------------------------------------------------------


def hex_to_float(text, double=False):
    text = text.strip()
    if text.lower().startswith("0x"):
        raw = bytes.fromhex(text[2:])
        if len(raw) == 8 or double:
            return struct.unpack("<d", raw.rjust(8, b"\0"))[0]
        return struct.unpack("<f", raw.rjust(4, b"\0"))[0]
    return float(text)


def float_to_hex(value, double=False):
    """Encodes like ezOpenDdlWriter in 'Exact' mode: zero is '0', everything else '0x' + memory bytes."""
    if value == 0:
        return "0"
    return "0x" + struct.pack("<d" if double else "<f", value).hex().upper()


def fmt_float(value, double=False):
    """Shortest decimal text that round-trips to the same float32 (or float64)."""
    if value != value or value in (float("inf"), float("-inf")):
        return repr(value)
    if double:
        return repr(value)
    target = struct.pack("<f", value)
    for prec in range(1, 10):
        s = "%.*g" % (prec, value)
        if struct.pack("<f", float(s)) == target:
            s = repr(float(s))  # '1e+01' -> '10.0', exponent only for very large/small values
            return s[:-2] if s.endswith(".0") else s
    return repr(value)


# ---------------------------------------------------------------------------------------------------------------
# OpenDDL parser (keeps source offsets so that edits can splice the original text)
# ---------------------------------------------------------------------------------------------------------------

PRIMITIVE_TYPES = {
    "b": "bool", "bool": "bool",
    "i1": "int", "i2": "int", "i3": "int", "i4": "int",
    "int8": "int", "int16": "int", "int32": "int", "int64": "int",
    "u1": "int", "u2": "int", "u3": "int", "u4": "int",
    "unsigned_int8": "int", "unsigned_int16": "int", "unsigned_int32": "int", "unsigned_int64": "int",
    "f": "float", "float": "float",
    "d": "double", "double": "double",
    "s": "string", "string": "string",
}


class DdlError(Exception):
    pass


class Node:
    """One DDL structure. Either a primitive list ('f{...}': values) or a custom structure ('o{...}': children).

    start/end are offsets into the source text covering the whole structure (type name to closing brace).
    """

    __slots__ = ("type", "name", "values", "children", "start", "end", "body_start", "parent")

    def __init__(self, type_, name, start):
        self.type = type_
        self.name = name
        self.values = None
        self.children = None
        self.start = start
        self.end = None
        self.body_start = None  # offset of the opening brace
        self.parent = None

    @property
    def is_primitive(self):
        return self.type in PRIMITIVE_TYPES

    def child(self, name=None, type_=None):
        for c in self.children or ():
            if (name is None or c.name == name) and (type_ is None or c.type == type_):
                return c
        return None

    def prim(self):
        """For 'Vec3 %X{f{...}}' returns the inner primitive node, for 'f %X{...}' the node itself."""
        if self.is_primitive:
            return self
        if self.children and len(self.children) == 1 and self.children[0].is_primitive:
            return self.children[0]
        return None

    def __repr__(self):
        return "Node(%s %s)" % (self.type, self.name or "")


_RE_TOKEN = re.compile(
    r"""
    (?P<ws>\s+)
  | (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<string>"(?:[^"\\]|\\.)*")
  | (?P<punct>[{},\[\]])
  | (?P<name>[%$](?:'[^']*'|[A-Za-z0-9_:.]+))
  | (?P<number>[-+]?(?:0[xX][0-9a-fA-F]+|[0-9.][0-9.eE_+-]*))
  | (?P<ident>'[^']*'|[A-Za-z_][A-Za-z0-9_:.]*)
    """,
    re.VERBOSE | re.DOTALL,
)

_ESCAPES = {'"': '"', "\\": "\\", "/": "/", "b": "\b", "f": "\f", "n": "\n", "r": "\r", "t": "\t"}


def unescape_string(lit):
    body = lit[1:-1]
    if "\\" not in body:
        return body
    out = []
    i = 0
    while i < len(body):
        c = body[i]
        if c == "\\" and i + 1 < len(body):
            out.append(_ESCAPES.get(body[i + 1], body[i + 1]))
            i += 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def escape_string(text):
    """Same escaping as ezOpenDdlWriter::OutputEscapedString."""
    for a, b in (("\\", "\\\\"), ('"', '\\"'), ("\b", "\\b"), ("\r", "\\r"), ("\f", "\\f"), ("\n", "\\n"), ("\t", "\\t")):
        text = text.replace(a, b)
    return '"' + text + '"'


def _tokenize(text):
    pos = 0
    n = len(text)
    toks = []
    while pos < n:
        m = _RE_TOKEN.match(text, pos)
        if not m:
            line = text.count("\n", 0, pos) + 1
            raise DdlError("line %d: unexpected character %r" % (line, text[pos]))
        kind = m.lastgroup
        if kind not in ("ws", "comment"):
            toks.append((kind, m.group(), m.start(), m.end()))
        pos = m.end()
    return toks


def _convert_value(kind, tok, ptype):
    if ptype == "string":
        if kind != "string":
            raise DdlError("expected string literal, got %r" % tok)
        return unescape_string(tok)
    if ptype == "bool":
        return tok in ("1", "true")
    if ptype in ("float", "double"):
        sign = -1.0 if tok.startswith("-") else 1.0
        t = tok.lstrip("+-")
        if t.lower().startswith("0x"):
            return sign * hex_to_float(t, ptype == "double")
        return sign * float(t.replace("_", ""))
    # int
    t = tok.replace("_", "")
    return int(t, 0) if t.lower().lstrip("+-").startswith("0x") else int(t)


def parse(text):
    """Parses DDL text into a list of top level Nodes. Raises DdlError with a line number on malformed input."""
    toks = _tokenize(text)
    i = 0
    n = len(toks)

    def line_of(off):
        return text.count("\n", 0, off) + 1

    def err(msg, off):
        raise DdlError("line %d: %s" % (line_of(off), msg))

    def parse_structure():
        nonlocal i
        kind, tok, s, e = toks[i]
        if kind != "ident":
            err("expected a structure type, got %r" % tok, s)
        node = Node(tok.strip("'"), None, s)
        i += 1
        if i < n and toks[i][0] == "name":
            node.name = toks[i][1][1:].strip("'")
            i += 1
        if i < n and toks[i][1] == "[":
            # array sub-type, e.g. 'float[3]' - not used by ez, skip it
            while i < n and toks[i][1] != "]":
                i += 1
            i += 1
        if i >= n or toks[i][1] != "{":
            err("expected '{' after %r" % node.type, s)
        node.body_start = toks[i][2]
        i += 1
        if node.type in PRIMITIVE_TYPES:
            ptype = PRIMITIVE_TYPES[node.type]
            node.values = []
            while i < n and toks[i][1] != "}":
                k, t, ts, te = toks[i]
                if t == ",":
                    i += 1
                    continue
                if k not in ("string", "number", "ident"):
                    err("unexpected %r in primitive list" % t, ts)
                try:
                    node.values.append(_convert_value(k, t, ptype))
                except ValueError:
                    err("cannot read %r as %s" % (t, ptype), ts)
                i += 1
        else:
            node.children = []
            while i < n and toks[i][1] != "}":
                c = parse_structure()
                c.parent = node
                node.children.append(c)
        if i >= n:
            err("missing '}' for %r" % node.type, s)
        node.end = toks[i][3]
        i += 1
        return node

    roots = []
    while i < n:
        roots.append(parse_structure())
    return roots


# ---------------------------------------------------------------------------------------------------------------
# Document model
# ---------------------------------------------------------------------------------------------------------------


def node_value(node):
    """Converts a property node to a plain Python value (Uuid -> GUID string, Vec3 -> list, ...)."""
    if node.is_primitive:
        return node.values[0] if len(node.values) == 1 else list(node.values)
    t = node.type
    if t == "Uuid":
        p = node.prim()
        return guid_to_string(p.values[0], p.values[1]) if p and len(p.values) == 2 else None
    if t == "VarArray":
        return [node_value(c) for c in node.children]
    if t == "VarDict":
        return {c.name: node_value(c) for c in node.children}
    if t == "Invalid":
        return None
    if t == "Angle":
        p = node.prim()
        return {"deg": fmt_float_num(p.values[0] * 180.0 / 3.141592653589793)} if p else None
    p = node.prim()
    if p is not None:
        return list(p.values) if len(p.values) != 1 else p.values[0]
    return {c.name or c.type: node_value(c) for c in node.children}


def fmt_float_num(v):
    return float(fmt_float(v))


class Document:
    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            raw = f.read()
        self.newline = "\r\n" if b"\r\n" in raw[:4096] else "\n"
        self.bom = raw.startswith(b"\xef\xbb\xbf")
        self.text = raw[3:].decode("utf-8") if self.bom else raw.decode("utf-8")
        self.roots = parse(self.text)
        self.header = self._section("HeaderV2")
        self.objects_section = self._section("Objects")
        self.types_section = self._section("Types")
        self.objects = {}
        self.order = []
        for o in (self.objects_section.children if self.objects_section else []):
            oid = self.obj_id(o)
            self.objects[oid] = o
            self.order.append(oid)

    def _section(self, name):
        for r in self.roots:
            if r.type == name:
                return r
        return None

    @staticmethod
    def obj_id(o):
        idn = o.child("id")
        p = idn.prim() if idn else None
        return (p.values[0], p.values[1]) if p else None

    @staticmethod
    def obj_type(o):
        t = o.child("t")
        return t.values[0] if t else None

    @staticmethod
    def obj_version(o):
        v = o.child("v")
        return v.values[0] if v else None

    @staticmethod
    def obj_props(o):
        return o.child(type_="p")

    def header_props(self):
        if not self.header:
            return None
        for o in self.header.children:
            if o.child("n") and o.child("n").values[0] == "Header":
                return self.obj_props(o)
        return None

    def root(self):
        for o in self.objects_section.children if self.objects_section else []:
            n = o.child("n")
            if n and n.values[0] == "ObjectTree":
                return o
        return None

    def prop(self, o, name):
        p = self.obj_props(o)
        return p.child(name) if p else None

    def prop_value(self, o, name, default=None):
        n = self.prop(o, name)
        return node_value(n) if n is not None else default

    def uuid_list(self, o, name):
        n = self.prop(o, name)
        out = []
        if n is None:
            return out
        items = n.children if n.type == "VarArray" else [n]
        for c in items:
            if c.type == "Uuid":
                p = c.prim()
                if p and len(p.values) == 2:
                    out.append((p.values[0], p.values[1]))
        return out

    def type_versions(self):
        """Type name -> version as recorded in the 'Types' section."""
        out = {}
        for o in self.types_section.children if self.types_section else []:
            name = self.prop_value(o, "TypeName")
            if name:
                out[name] = self.prop_value(o, "TypeVersion")
        return out

    def save(self, text):
        with open(self.path, "wb") as f:
            f.write((b"\xef\xbb\xbf" if self.bom else b"") + text.encode("utf-8"))


def splice(text, edits):
    """Applies [(start, end, replacement)] to text. Edits must not overlap."""
    for start, end, rep in sorted(edits, key=lambda e: e[0], reverse=True):
        text = text[:start] + rep + text[end:]
    return text


def line_indent(text, offset):
    ls = text.rfind("\n", 0, offset) + 1
    m = re.match(r"[ \t]*", text[ls:offset])
    return m.group() if m else ""


# ---------------------------------------------------------------------------------------------------------------
# Projects, data directories and the asset index
# ---------------------------------------------------------------------------------------------------------------

DOC_EXT = re.compile(r"\.ez[A-Za-z0-9]+$")
SKIP_DIRS = {"AssetCache", ".git", "Workspace", "Output", "node_modules", "__pycache__"}


def find_project_dir(path):
    """Walks up from path to the folder that contains the 'ezProject' file."""
    p = os.path.abspath(path)
    if os.path.isfile(p):
        p = os.path.dirname(p)
    while True:
        if os.path.isfile(os.path.join(p, "ezProject")):
            return p
        parent = os.path.dirname(p)
        if parent == p:
            return None
        p = parent


def project_data_dirs(project_dir):
    """Resolves RuntimeConfigs/DataDirectories.ddl of a project ('>sdk/' = this repository, '>project/')."""
    cfg = os.path.join(project_dir, "RuntimeConfigs", "DataDirectories.ddl")
    dirs = []
    if os.path.isfile(cfg):
        with open(cfg, "r", encoding="utf-8") as f:
            for node in parse(f.read()):
                if node.type != "DataDir":
                    continue
                pn = node.child("Path")
                if not pn or not pn.values:
                    continue
                path = pn.values[0]
                if path.startswith(">sdk/"):
                    path = os.path.join(SDK_ROOT, path[5:])
                elif path.startswith(">project/"):
                    path = os.path.join(project_dir, path[9:])
                elif not os.path.isabs(path):
                    path = os.path.join(project_dir, path)
                dirs.append(os.path.normpath(path))
    if not dirs:
        dirs = [os.path.join(SDK_ROOT, "Data", "Base"), project_dir]
    return dirs


def scan_roots(args, hint_path=None):
    """Directories to index: --root if given, else the data directories of --project / the project containing
    hint_path, else the whole Data folder of this repository."""
    if getattr(args, "root", None):
        return [os.path.abspath(r) for r in args.root]
    proj = getattr(args, "project", None) or (find_project_dir(hint_path) if hint_path else None)
    if proj:
        return project_data_dirs(proj)
    return [os.path.join(SDK_ROOT, "Data")]


def iter_documents(roots):
    seen = set()
    for root in roots:
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
            for fn in filenames:
                if not DOC_EXT.search(fn) or fn.startswith("ezBin"):
                    continue
                full = os.path.normpath(os.path.join(dirpath, fn))
                key = os.path.normcase(full)
                if key in seen:
                    continue
                seen.add(key)
                yield full


_RE_HDR_DOCID = re.compile(r"Uuid\s+%DocumentID\s*\{\s*u4\s*\{\s*(\d+)\s*,\s*(\d+)\s*\}")
_RE_HDR_TYPE = re.compile(r's\s+%AssetType\s*\{\s*"([^"]*)"')


def read_header_text(path):
    """Returns the HeaderV2 section text of a document, or None if the file is no DDL document."""
    try:
        with open(path, "rb") as f:
            head = f.read(64 * 1024)
    except OSError:
        return None
    if not head.lstrip(b"\xef\xbb\xbf \t\r\n").startswith(b"HeaderV2"):
        return None
    head = head.decode("utf-8", "replace")
    m = re.search(r"\n\s*Objects\s*\n", head)
    return head[: m.start()] if m else head


class AssetIndex:
    def __init__(self, roots):
        self.roots = roots
        self.by_guid = {}  # (low, high) -> (path, asset type)
        self.files = []
        for path in iter_documents(roots):
            hdr = read_header_text(path)
            if hdr is None:
                continue
            self.files.append(path)
            m = _RE_HDR_DOCID.search(hdr)
            if not m:
                continue
            t = _RE_HDR_TYPE.search(hdr)
            self.by_guid[(int(m.group(1)), int(m.group(2)))] = (path, t.group(1) if t else "")

    def lookup(self, guid):
        return self.by_guid.get(guid)

    def find(self, pattern):
        """Assets whose path matches pattern. If some file names match it exactly, only those are returned."""
        rx = re.compile(fnmatch_to_regex(pattern), re.IGNORECASE)
        hits = [(g, p, t) for g, (p, t) in sorted(self.by_guid.items(), key=lambda kv: kv[1][0]) if rx.search(p.replace("\\", "/"))]
        exact = [h for h in hits if os.path.basename(h[1]).lower() == pattern.lower() or h[1].replace("\\", "/").lower().endswith("/" + pattern.lower())]
        return exact or hits


def fnmatch_to_regex(pattern):
    """Without wildcards: substring match. With '*' or '?': the pattern has to match from the start of a path
    component to the end, so 'Coin*' matches '.../Coin.ezPrefab' but not '.../CollectCoin.ezPrefab'. '*' does not
    cross '/'."""
    if any(c in pattern for c in "*?"):
        return "(?:^|/)" + re.escape(pattern).replace(r"\*", "[^/]*").replace(r"\?", "[^/]") + "$"
    return re.escape(pattern)


def rel(path):
    try:
        r = os.path.relpath(path, SDK_ROOT)
        if not r.startswith(".."):
            return r.replace("\\", "/")
    except ValueError:
        pass
    return path.replace("\\", "/")


# ---------------------------------------------------------------------------------------------------------------
# Type information from the C++ sources
# ---------------------------------------------------------------------------------------------------------------

_RE_BEGIN_TYPE = re.compile(
    r"EZ_BEGIN_(?P<kind>COMPONENT_TYPE|ABSTRACT_COMPONENT_TYPE|DYNAMIC_REFLECTED_TYPE|ABSTRACT_DYNAMIC_REFLECTED_TYPE|STATIC_REFLECTED_TYPE|STATIC_REFLECTED_ENUM|STATIC_REFLECTED_BITFLAGS)\s*\(\s*(?P<args>[^)]*)\)"
)
_RE_DECL_BASE = re.compile(r"EZ_(?:DECLARE_COMPONENT_TYPE|DECLARE_ABSTRACT_COMPONENT_TYPE|ADD_DYNAMIC_REFLECTION)\s*\(\s*(\w+)\s*,\s*(\w+)")
_RE_PROPERTY = re.compile(r"EZ_(?P<macro>(?:[A-Z]+_)*(?:MEMBER|ACCESSOR)_PROPERTY(?:_READ_ONLY)?)\s*\(\s*\"(?P<name>[^\"]+)\"(?P<rest>[^;\n]*)")
_RE_ENUM_CONST = re.compile(r"EZ_ENUM_CONSTANTS?\s*\(([^)]*)\)|EZ_BITFLAGS_CONSTANTS?\s*\(([^)]*)\)")

_type_cache = None


def type_index():
    """Scans Code/ (without ThirdParty) for reflection blocks. Returns {type name: info dict}."""
    global _type_cache
    if _type_cache is not None:
        return _type_cache
    types = {}
    bases = {}
    code = os.path.join(SDK_ROOT, "Code")
    for dirpath, dirnames, filenames in os.walk(code):
        dirnames[:] = [d for d in dirnames if d != "ThirdParty"]
        for fn in filenames:
            if not fn.endswith((".cpp", ".h", ".inl")):
                continue
            full = os.path.join(dirpath, fn)
            try:
                with open(full, "r", encoding="utf-8", errors="replace") as f:
                    src = f.read()
            except OSError:
                continue
            if "EZ_" not in src:
                continue
            for m in _RE_DECL_BASE.finditer(src):
                bases[m.group(1)] = m.group(2)
            for m in _RE_BEGIN_TYPE.finditer(src):
                args = [a.strip() for a in re.sub(r"/\*.*?\*/", "", m.group("args")).split(",")]
                kind = m.group("kind")
                info = {"name": args[0], "kind": kind, "file": rel(full), "line": src.count("\n", 0, m.start()) + 1, "base": None, "version": None}
                try:
                    if kind == "STATIC_REFLECTED_TYPE":
                        info["base"] = None if args[1] == "ezNoBase" else args[1]
                        info["version"] = int(args[2])
                    else:
                        info["version"] = int(args[1])
                except (IndexError, ValueError):
                    continue  # no literal version: the macro definitions themselves
                if kind.endswith("COMPONENT_TYPE") or "DYNAMIC" in kind:
                    info["_dynamic"] = True
                # body: up to the matching EZ_END_
                end = src.find("EZ_END_", m.end())
                body = src[m.end() : end if end >= 0 else len(src)]
                if kind in ("STATIC_REFLECTED_ENUM", "STATIC_REFLECTED_BITFLAGS"):
                    consts = []
                    for cm in _RE_ENUM_CONST.finditer(body):
                        consts += [c.strip() for c in (cm.group(1) or cm.group(2) or "").split(",") if c.strip()]
                    info["constants"] = consts
                else:
                    props = []
                    for pm in _RE_PROPERTY.finditer(body):
                        props.append({"name": pm.group("name"), "macro": pm.group("macro"), "args": pm.group("rest").strip().lstrip(",").strip().rstrip(",")})
                    info["properties"] = props
                types[args[0]] = info
    for name, info in types.items():
        if info.pop("_dynamic", False):
            info["base"] = bases.get(name, info["base"])
    _type_cache = types
    return types


def type_chain(name):
    types = type_index()
    chain = []
    while name and name in types and name not in [c["name"] for c in chain]:
        chain.append(types[name])
        name = types[name]["base"]
    return chain


def current_type_version(name):
    info = type_index().get(name)
    return info["version"] if info else None


# ---------------------------------------------------------------------------------------------------------------
# Pretty printing
# ---------------------------------------------------------------------------------------------------------------


class Printer:
    def __init__(self, index=None, decode=True, doc=None, max_string=None):
        self.index = index
        self.decode = decode
        self.doc = doc
        self.max_string = max_string

    def guid_note(self, low, high):
        s = guid_to_string(low, high)
        note = s
        if self.doc is not None and (low, high) in self.doc.objects and self.doc.obj_id(self.doc.objects[(low, high)]) is not None:
            o = self.doc.objects[(low, high)]
            name = self.doc.prop_value(o, "Name")
            return note + " -> object %s%s" % (self.doc.obj_type(o), (' "%s"' % name) if name else "")
        if self.index:
            hit = self.index.lookup((low, high))
            if hit:
                note += " -> " + rel(hit[0])
        return note

    def string_note(self, value):
        if not _RE_GUID_STR.fullmatch(value):
            return None
        g = string_to_guid(value)
        if self.doc is not None and g in self.doc.objects:
            o = self.doc.objects[g]
            name = self.doc.prop_value(o, "Name")
            return "-> object %s%s" % (self.doc.obj_type(o), (' "%s"' % name) if name else "")
        if self.index:
            hit = self.index.lookup(g)
            return ("-> " + rel(hit[0])) if hit else "-> (unknown asset)"
        return None

    def fmt_prim(self, node):
        pt = PRIMITIVE_TYPES[node.type]
        if pt == "string":
            vals = [escape_string(v) for v in node.values]
            if self.max_string:
                vals = [v if len(v) <= self.max_string else v[: self.max_string] + '..." (%d chars, --full shows all)' % len(v) for v in vals]
        elif pt == "bool":
            vals = ["1" if v else "0" for v in node.values]
        elif pt in ("float", "double"):
            vals = [fmt_float(v, pt == "double") if self.decode else float_to_hex(v, pt == "double") for v in node.values]
        else:
            vals = [str(v) for v in node.values]
        return "%s{%s}" % (node.type, ",".join(vals))

    def lines(self, node, indent=""):
        head = node.type + (" %" + node.name if node.name else "")
        if node.is_primitive:
            note = None
            if PRIMITIVE_TYPES[node.type] == "string" and len(node.values) == 1:
                note = self.string_note(node.values[0])
            yield indent + node.type + (" %" + node.name if node.name else "") + "{" + self.fmt_prim(node)[len(node.type) + 1 :] + ("  // " + note if note else "")
            return
        p = node.prim()
        if p is not None and node.type != "VarArray":
            note = ""
            if node.type == "Uuid" and len(p.values) == 2:
                note = "  // " + (guid_to_string(p.values[0], p.values[1]) if node.name == "id" else self.guid_note(p.values[0], p.values[1]))
            elif node.type == "Angle" and p.values and self.decode:
                note = "  // %s deg" % fmt_float(p.values[0] * 180.0 / 3.141592653589793)
            yield indent + head + "{" + self.fmt_prim(p) + "}" + note
            return
        if not node.children:
            yield indent + head + "{}"
            return
        yield indent + head
        yield indent + "{"
        for c in node.children:
            yield from self.lines(c, indent + "\t")
        yield indent + "}"


# ---------------------------------------------------------------------------------------------------------------
# Value encoding for 'set' and 'new-object'
# ---------------------------------------------------------------------------------------------------------------

VECTOR_TYPES = {
    "Vec2": ("f", 2), "Vec3": ("f", 3), "Vec4": ("f", 4), "Quat": ("f", 4), "Color": ("f", 4),
    "Vec2i": ("i3", 2), "Vec3i": ("i3", 3), "Vec4i": ("i3", 4),
    "Vec2u": ("u3", 2), "Vec3u": ("u3", 3), "Vec4u": ("u3", 4),
    "ColorGamma": ("u1", 4), "Transform": ("f", 10), "Mat3": ("f", 9), "Mat4": ("f", 16),
}


def _split_numbers(value):
    return [v for v in re.split(r"[\s,;()\[\]{}]+", value.strip()) if v]


def encode_value(dtype, value):
    """Returns the DDL text of the value part ('{...}' incl. braces) for a property of the given type.

    Angle needs a 'deg' or 'rad' suffix, Time is seconds, Uuid takes any GUID form, Color takes 3 or 4 linear
    floats (alpha defaults to 1), Quat takes x,y,z,w.
    """
    value = value.strip()
    if dtype in PRIMITIVE_TYPES:
        pt = PRIMITIVE_TYPES[dtype]
        if pt == "string":
            return "{" + escape_string(value) + "}"
        if pt == "bool":
            v = value.lower()
            if v not in ("0", "1", "true", "false"):
                raise ValueError("bool value must be 0/1/true/false")
            return "{" + ("1" if v in ("1", "true") else "0") + "}"
        if pt in ("float", "double"):
            return "{" + ",".join(float_to_hex(float(v), pt == "double") for v in _split_numbers(value)) + "}"
        return "{" + ",".join(str(int(v, 0)) for v in _split_numbers(value)) + "}"
    if dtype == "Uuid":
        low, high = parse_guid(value)
        return "{" + guid_to_u4(low, high) + "}"
    if dtype == "Angle":
        m = re.fullmatch(r"([-+0-9.eE]+)\s*(deg|rad)", value)
        if not m:
            raise ValueError("Angle values need a unit suffix, e.g. '90deg' or '1.5708rad'")
        rad = float(m.group(1)) * (3.141592653589793 / 180.0 if m.group(2) == "deg" else 1.0)
        return "{f{" + float_to_hex(rad) + "}}"
    if dtype == "Time":
        return "{d{" + float_to_hex(float(value.rstrip("s")), True) + "}}"
    if dtype in ("HashedString", "TempHashedString"):
        return "{s{" + escape_string(value) + "}}"
    if dtype in VECTOR_TYPES:
        prim, count = VECTOR_TYPES[dtype]
        nums = _split_numbers(value)
        if dtype == "Color" and len(nums) == 3:
            nums.append("1")
        if dtype == "ColorGamma" and len(nums) == 3:
            nums.append("255")
        if len(nums) != count:
            raise ValueError("%s needs %d numbers, got %d" % (dtype, count, len(nums)))
        if prim == "f":
            return "{f{" + ",".join(float_to_hex(float(v)) for v in nums) + "}}"
        return "{%s{%s}}" % (prim, ",".join(str(int(v, 0)) for v in nums))
    raise ValueError("encoding values of type '%s' is not supported, edit the text directly or use --raw" % dtype)


# ---------------------------------------------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------------------------------------------


def find_object(doc, spec):
    try:
        guid = parse_guid(spec)
    except ValueError:
        guid = None
    if guid is not None:
        o = doc.objects.get(guid)
        if o is None:
            raise SystemExit("error: no object with id %s in %s" % (guid_to_string(*guid), doc.path))
        return o
    # by name of a game object
    hits = [o for o in doc.objects.values() if doc.prop_value(o, "Name") == spec]
    if len(hits) == 1:
        return hits[0]
    if not hits:
        raise SystemExit("error: '%s' is neither a GUID nor the name of an object" % spec)
    raise SystemExit("error: %d objects are named '%s', use the GUID instead" % (len(hits), spec))


def cmd_guid(args):
    vals = args.values
    if vals and vals[0] == "new":
        count = int(vals[1]) if len(vals) > 1 else 1
        for _ in range(count):
            low, high = new_guid()
            print("%s  %s" % (guid_to_string(low, high), guid_to_u4(low, high)))
        return 0
    if len(vals) == 3 and vals[0] in ("add", "sub"):
        a, b = parse_guid(vals[1]), parse_guid(vals[2])
        r = guid_add(a, b) if vals[0] == "add" else guid_sub(a, b)
        print("%s  %s" % (guid_to_string(*r), guid_to_u4(*r)))
        return 0
    if not vals:
        raise SystemExit("error: pass a GUID, 'new [count]', or 'add|sub <guid> <seed>'")
    index = AssetIndex(scan_roots(args)) if args.resolve else None
    for v in vals:
        low, high = parse_guid(v)
        line = "%s  %s" % (guid_to_string(low, high), guid_to_u4(low, high))
        if index:
            hit = index.lookup((low, high))
            line += "  " + (("%s (%s)" % (rel(hit[0]), hit[1])) if hit else "(no asset with this GUID)")
        print(line)
    return 0


def cmd_float(args):
    for v in args.values:
        if v.lower().lstrip("-").startswith("0x"):
            sign = -1.0 if v.startswith("-") else 1.0
            f = sign * hex_to_float(v.lstrip("-"), args.double)
            print("%s = %s" % (v, fmt_float(f, args.double or len(v.lstrip("-")) > 10)))
        else:
            print("%s = %s" % (v, float_to_hex(float(v), args.double)))
    return 0


def cmd_asset(args):
    index = AssetIndex(scan_roots(args))
    found = 0
    for q in args.query:
        try:
            guid = parse_guid(q)
        except ValueError:
            guid = None
        if guid is not None:
            hit = index.lookup(guid)
            if hit:
                print("%s  %s  %s" % (guid_to_string(*guid), hit[1], rel(hit[0])))
                found += 1
            else:
                print("%s  (no asset with this GUID in %s)" % (guid_to_string(*guid), ", ".join(rel(r) for r in index.roots)))
            continue
        for g, path, atype in index.find(q):
            print("%s  %s  %s" % (guid_to_string(*g), atype, rel(path)))
            found += 1
    return 0 if found else 1


def _iter_property_paths(node, path=""):
    """Yields (property path, node) for every primitive or Uuid leaf below node."""
    for c in node.children or ():
        p = path + ("." if path and c.name else "") + (c.name or "")
        if c.is_primitive or c.type == "Uuid":
            yield p, c
        else:
            yield from _iter_property_paths(c, p if c.name else path + "[]")


def cmd_uses(args):
    targets = []
    for q in args.asset:
        try:
            targets.append(parse_guid(q))
        except ValueError:
            idx = AssetIndex(scan_roots(args))
            hits = list(idx.find(q))
            if len(hits) != 1:
                raise SystemExit("error: '%s' matches %d assets, pass a GUID or a more specific path" % (q, len(hits)))
            targets.append(hits[0][0])
    strs = {guid_to_string(*g): g for g in targets}
    u4s = {"%d,%d" % g: g for g in targets}
    roots = scan_roots(args)
    rc = 1
    for path in iter_documents(roots):
        try:
            with open(path, "rb") as f:
                raw = f.read()
        except OSError:
            continue
        if not raw.lstrip(b"\xef\xbb\xbf \t\r\n").startswith(b"HeaderV2"):
            continue
        text = raw.decode("utf-8", "replace")
        if not any(s in text for s in strs) and not any(u in text.replace(" ", "") for u in u4s):
            continue
        try:
            doc = Document(path)
        except DdlError as e:
            print("%s: parse error: %s" % (rel(path), e))
            continue
        hits = []
        for oid in doc.order:
            o = doc.objects[oid]
            props = doc.obj_props(o)
            if props is None:
                continue
            for ppath, leaf in _iter_property_paths(props):
                if leaf.type == "s" and leaf.values and leaf.values[0] in strs:
                    hits.append((oid, doc.obj_type(o), ppath))
                elif leaf.type == "Uuid":
                    p = leaf.prim()
                    if p and tuple(p.values) in targets:
                        hits.append((oid, doc.obj_type(o), ppath))
        if not hits:
            continue
        rc = 0
        hp = doc.header_props()
        in_header = []
        if hp is not None:
            for key in ("Dependencies", "References", "PackageDeps"):
                vals = node_value(hp.child(key)) if hp.child(key) is not None else []
                if any(v in strs for v in vals or []):
                    in_header.append(key)
        print("%s%s" % (rel(path), ("  [header: %s]" % ", ".join(in_header)) if in_header else "  [not in header]"))
        if not args.files_only:
            for oid, otype, ppath in hits:
                print("    %s  %s.%s" % (guid_to_string(*oid), otype, ppath))
    return rc


def cmd_show(args):
    doc = Document(args.file)
    index = None if args.no_resolve else AssetIndex(scan_roots(args, args.file))
    pr = Printer(index, decode=not args.hex, doc=doc, max_string=None if args.full else 200)
    out = []
    if args.object:
        for spec in args.object:
            out += list(pr.lines(find_object(doc, spec)))
    else:
        sections = args.section or (["Objects"] if args.type else ["HeaderV2", "Objects"])
        for r in doc.roots:
            if r.type not in sections:
                continue
            if r.type == "Objects" and args.type:
                out.append("Objects")
                out.append("{")
                for oid in doc.order:
                    o = doc.objects[oid]
                    if any(re.fullmatch(fnmatch_to_regex(t), doc.obj_type(o) or "") for t in args.type):
                        out += list(pr.lines(o, "\t"))
                out.append("}")
            else:
                out += list(pr.lines(r))
    print("\n".join(out))
    return 0


def _go_summary(doc, o, pr):
    name = doc.prop_value(o, "Name")
    parts = []
    pos = doc.prop(o, "LocalPosition")
    if pos is not None and pos.prim() and any(pos.prim().values):
        parts.append("pos(" + ", ".join(fmt_float(v) for v in pos.prim().values) + ")")
    if doc.prop_value(o, "Active") is False:
        parts.append("inactive")
    mfp = doc.prop(o, "MetaFromPrefab")
    if mfp is not None and mfp.prim():
        parts.append("prefab instance of " + pr.guid_note(*mfp.prim().values))
    return name, parts


def cmd_tree(args):
    doc = Document(args.file)
    index = None if args.no_resolve else AssetIndex(scan_roots(args, args.file))
    pr = Printer(index, doc=doc)
    root = doc.root()
    if root is None:
        raise SystemExit("error: no 'ObjectTree' root object in %s" % args.file)
    print("%s  %s (root)" % (guid_to_string(*doc.obj_id(root)), doc.obj_type(root)))

    def show_component(cid, indent):
        c = doc.objects.get(cid)
        if c is None:
            print("%s- MISSING component %s" % (indent, guid_to_string(*cid)))
            return
        extra = []
        if args.props:
            props = doc.obj_props(c)
            for p in props.children if props else []:
                if p.type == "s" and p.values and _RE_GUID_STR.fullmatch(p.values[0]):
                    note = pr.string_note(p.values[0])
                    extra.append("%s=%s" % (p.name, note[3:] if note else p.values[0]))
        print("%s- %s  %s%s" % (indent, guid_to_string(*cid), doc.obj_type(c), ("  " + ", ".join(extra)) if extra else ""))

    def walk(oid, indent):
        o = doc.objects.get(oid)
        if o is None:
            print("%s* MISSING object %s" % (indent, guid_to_string(*oid)))
            return
        name, parts = _go_summary(doc, o, pr)
        t = doc.obj_type(o)
        label = ('"%s"' % name) if name else "(unnamed)"
        if t != "ezGameObject":
            label += " [%s]" % t
        print("%s* %s  %s%s" % (indent, guid_to_string(*oid), label, ("  " + ", ".join(parts)) if parts else ""))
        if not args.no_components:
            for cid in doc.uuid_list(o, "Components"):
                show_component(cid, indent + "    ")
        if args.depth is None or len(indent) // 2 < args.depth:
            for cid in doc.uuid_list(o, "Children"):
                walk(cid, indent + "  ")

    for cid in doc.uuid_list(root, "Children"):
        walk(cid, "  ")
    settings = doc.uuid_list(root, "Settings")
    if settings and args.settings:
        print("settings:")
        print("\n".join(pr.lines(doc.objects[settings[0]], "  ")))
    return 0


def cmd_check(args):
    problems = 0
    notes = 0
    indices = {}
    for path in args.files:
        index = None
        if not args.no_resolve:
            roots = tuple(scan_roots(args, path))
            if roots not in indices:
                indices[roots] = AssetIndex(list(roots))
            index = indices[roots]
        try:
            doc = Document(path)
        except DdlError as e:
            print("%s: ERROR parse: %s" % (rel(path), e))
            problems += 1
            continue
        except UnicodeDecodeError as e:
            print("%s: ERROR not UTF-8: %s" % (rel(path), e))
            problems += 1
            continue
        msgs = []

        def err(m):
            msgs.append("ERROR " + m)

        def warn(m):
            msgs.append("WARN  " + m)

        def info(m):
            msgs.append("note  " + m)

        for sec in ("HeaderV2", "Objects", "Types"):
            if doc._section(sec) is None:
                err("section '%s' is missing" % sec)
        if len(doc.objects) != len(doc.order):
            seen = set()
            for oid in doc.order:
                if oid in seen:
                    err("object id %s is used more than once" % guid_to_string(*oid))
                seen.add(oid)
        root = doc.root()
        if doc.objects_section is not None and root is None:
            err("no object named 'ObjectTree' (the document root)")

        # object references: every Children/Components entry must exist and have one owner
        owner = {}
        for oid in doc.order:
            o = doc.objects[oid]
            for key in ("Children", "Components", "Settings"):
                for ref in doc.uuid_list(o, key):
                    if ref not in doc.objects:
                        err("%s %s.%s references missing object %s" % (doc.obj_type(o), guid_to_string(*oid), key, guid_to_string(*ref)))
                    elif ref in owner:
                        err("object %s is referenced by %s and %s" % (guid_to_string(*ref), guid_to_string(*owner[ref]), guid_to_string(*oid)))
                    else:
                        owner[ref] = oid
        # reachability: objects nobody owns are not part of the document
        owned_via_other = set()
        for oid in doc.order:
            o = doc.objects[oid]
            props = doc.obj_props(o)
            for _, leaf in _iter_property_paths(props) if props is not None else ():
                if leaf.type == "Uuid" and leaf.prim() and len(leaf.prim().values) == 2:
                    owned_via_other.add(tuple(leaf.prim().values))
        for oid in doc.order:
            o = doc.objects[oid]
            if o is root or oid in owner:
                continue
            if oid in owned_via_other:
                continue  # referenced from some other object property (e.g. graph nodes / connections)
            warn("object %s (%s) is not referenced by any other object" % (guid_to_string(*oid), doc.obj_type(o)))

        # asset references
        header_refs = set()
        hp = doc.header_props()
        if hp is None:
            err("no 'Header' object in HeaderV2")
        else:
            for key in ("Dependencies", "References", "PackageDeps"):
                n = hp.child(key)
                if n is not None:
                    header_refs.update(v for v in node_value(n) if isinstance(v, str))
        missing_in_header = {}
        unknown = {}
        for oid in doc.order:
            o = doc.objects[oid]
            props = doc.obj_props(o)
            if props is None:
                continue
            for ppath, leaf in _iter_property_paths(props):
                if ppath == "MetaBasePrefab":
                    continue
                if leaf.type == "s":
                    for v in leaf.values:
                        if isinstance(v, str) and _RE_GUID_STR.fullmatch(v):
                            g = string_to_guid(v)
                            if g in doc.objects:
                                continue  # a game object reference (e.g. a constraint's ChildActor) within this document
                            if index and index.lookup(g) is None:
                                unknown.setdefault(v, []).append("%s.%s" % (doc.obj_type(o), ppath))
                            if v not in header_refs:
                                missing_in_header.setdefault(v, ppath)

        for v, where in unknown.items():
            warn("%s is neither an object in this document nor a known asset (typo, deleted, sub-asset, or outside %s) - used %dx, e.g. %s"
                 % (v, ", ".join(rel(r) for r in index.roots), len(where), where[0]))

        if missing_in_header and args.verbose:
            info("%d asset reference(s) are not listed in the header Dependencies/References/PackageDeps, e.g. %s (%s). "
                 "Normal for properties that are no dependency (e.g. a scene to load), or after a hand edit until the editor saves the document."
                 % (len(missing_in_header), next(iter(missing_in_header)), next(iter(missing_in_header.values()))))

        # type versions
        if not args.no_versions:
            file_types = doc.type_versions()
            for oid in doc.order:
                o = doc.objects[oid]
                t, v = doc.obj_type(o), doc.obj_version(o)
                if t is None or v is None:
                    continue
                cur = current_type_version(t)
                if cur is None:
                    continue
                if v > cur:
                    err("%s %s has version %d, but the code declares version %d" % (t, guid_to_string(*oid), v, cur))
                elif file_types.get(t) is not None and v != file_types[t]:
                    warn("%s %s has version %d, but the Types section says %d, which is what loading uses (the code is at %d)" % (t, guid_to_string(*oid), v, file_types[t], cur))

        status = "OK" if not any(m.startswith("ERROR") for m in msgs) else "FAILED"
        if msgs or args.verbose:
            print("%s: %s" % (rel(path), status))
            for m in msgs:
                print("  " + m)
        problems += sum(1 for m in msgs if m.startswith("ERROR"))
        notes += len(msgs)
    if not notes:
        print("%d file(s) OK" % len(args.files))
    return 1 if problems else 0


def cmd_touch(args):
    """Sets the header Hash to a new random value.

    The curator compares the header Hash (not the file content) with the hash stored in the transformed output, so
    this makes it re-transform the asset. Dependencies/References in the header are left as they are; they are
    written by the editor according to property attributes and are only exact after the editor saved the document.
    """
    for path in args.files:
        doc = Document(path)
        hp = doc.header_props()
        hn = hp.child("Hash") if hp is not None else None
        if hn is None:
            raise SystemExit("error: %s has no header Hash" % path)
        old = hn.values[0]
        new = random.getrandbits(64) or 1
        doc.save(splice(doc.text, [(hn.body_start, hn.end, "{%d}" % new)]))
        print("%s: Hash %d -> %d" % (rel(path), old, new))
    return 0


def _property_insert(doc, props_node, prop_name, prop_text):
    """Returns an edit that inserts prop_text into a 'p' block, keeping the alphabetical order the editor uses."""
    nl = doc.newline
    ind = line_indent(doc.text, props_node.start) + "\t"
    after = None
    for c in props_node.children:
        if (c.name or "").lower() < prop_name.lower():
            after = c
    if after is not None:
        return (after.end, after.end, nl + ind + prop_text)
    if props_node.children:
        first = props_node.children[0]
        return (first.start, first.start, prop_text + nl + ind)
    return (props_node.body_start, props_node.end, "{" + nl + ind + prop_text + nl + ind[:-1] + "}")


def cmd_set(args):
    doc = Document(args.file)
    o = find_object(doc, args.object)
    props = doc.obj_props(o)
    if props is None:
        raise SystemExit("error: object has no 'p' block")
    node = props.child(args.property)
    dtype = args.type or (node.type if node is not None else None)
    if dtype is None:
        raise SystemExit("error: '%s' is not stored on this %s yet, pass --type (e.g. f, s, b, u3, Vec3, Color, Angle, Uuid)" % (args.property, doc.obj_type(o)))
    if args.raw:
        value_text = args.value if args.value.startswith("{") else "{" + args.value + "}"
    else:
        try:
            value_text = encode_value(dtype, args.value)
        except ValueError as e:
            raise SystemExit("error: %s" % e)
    prop_text = "%s %%%s%s" % (dtype, args.property, value_text)
    if node is not None:
        edit = (node.start, node.end, prop_text)
    else:
        edit = _property_insert(doc, props, args.property, prop_text)
    new_text = splice(doc.text, [edit])
    parse(new_text)  # never write something that does not parse
    if args.dry_run:
        print(prop_text)
        return 0
    doc.save(new_text)
    print("%s: %s %s.%s = %s" % (rel(args.file), guid_to_string(*doc.obj_id(o)), doc.obj_type(o), args.property, prop_text))
    return 0


def _object_block(nl, guid, type_name, version, props):
    """Text of an 'o' block. props is a list of already formatted property lines (without indentation)."""
    lines = ["o", "{", "\tUuid %%id{%s}" % guid_to_u4(*guid), "\ts %%t{%s}" % escape_string(type_name), "\tu3 %%v{%d}" % version, "\tp", "\t{"]
    lines += ["\t\t" + p for p in props]
    lines += ["\t}", "}"]
    return nl.join(lines)


def _uuid_array_edit(doc, owner_props, key, add_guids):
    """Edit that appends Uuids to the VarArray 'key' of a 'p' block (creating it when missing)."""
    nl = doc.newline
    node = owner_props.child(key)
    if node is None:
        entries = ["Uuid{%s}" % guid_to_u4(*g) for g in add_guids]
        ind = line_indent(doc.text, owner_props.start) + "\t"
        text = "VarArray %%%s%s%s{%s%s%s}" % (key, nl, ind, nl, "".join(ind + "\t" + e + nl for e in entries), ind)
        return _property_insert(doc, owner_props, key, text)
    ind = line_indent(doc.text, node.start)
    entries = [doc.text[c.start : c.end] for c in node.children] + ["Uuid{%s}" % guid_to_u4(*g) for g in add_guids]
    body = nl + ind + "{" + nl + "".join(ind + "\t" + e + nl for e in entries) + ind + "}"
    return (node.start, node.end, "VarArray %" + key + body)


def cmd_new_object(args):
    doc = Document(args.file)
    root = doc.root()
    if args.parent:
        parent = find_object(doc, args.parent)
    else:
        parent = root
    if parent is None:
        raise SystemExit("error: no parent")
    parent_props = doc.obj_props(parent)

    file_versions = doc.type_versions()
    lagging = []

    def version_for(type_name):
        # When the Types section lists a type, its version replaces the object's 'v' during loading (see
        # ezGraphPatchContext::UpdateBaseClasses), so the object must match it. Otherwise the code version applies.
        code_ver = current_type_version(type_name)
        file_ver = file_versions.get(type_name)
        if file_ver is not None:
            if code_ver is not None and file_ver < code_ver:
                lagging.append((type_name, file_ver, code_ver))
            return file_ver
        if code_ver is None:
            raise SystemExit("error: cannot determine the version of '%s' (not found in Code/ or in the file's Types section). Check the type name." % type_name)
        return code_ver

    go_type = "ezGameObject"
    go_version = version_for(go_type)
    go_id = new_guid()
    comp_blocks = []
    comp_ids = []
    for ctype in args.component or []:
        ver = version_for(ctype)
        cid = new_guid()
        comp_ids.append(cid)
        comp_blocks.append(_object_block(doc.newline, cid, ctype, ver, ["b %Active{1}"]))

    pos = encode_value("Vec3", args.pos or "0,0,0")
    rot = encode_value("Quat", args.rot or "0,0,0,1")
    scale = encode_value("Vec3", args.scale or "1,1,1")
    comps = "VarArray %Components{}" if not comp_ids else ("VarArray %Components" + doc.newline + "\t\t{" + doc.newline + "".join("\t\t\tUuid{%s}%s" % (guid_to_u4(*c), doc.newline) for c in comp_ids) + "\t\t}")
    tags = "VarArray %Tags{}" if not args.tag else ("VarArray %Tags" + doc.newline + "\t\t{" + doc.newline + "".join("\t\t\ts{%s}%s" % (escape_string(t), doc.newline) for t in args.tag) + "\t\t}")
    go_props = [
        "b %Active{1}",
        "VarArray %Children{}",
        comps,
        "s %GlobalKey{\"\"}",
        "Vec3 %LocalPosition" + pos,
        "Quat %LocalRotation" + rot,
        "Vec3 %LocalScaling" + scale,
        "f %LocalUniformScaling{0x0000803F}",
        "s %Mode{\"ezObjectMode::Automatic\"}",
        "s %%Name{%s}" % escape_string(args.name or ""),
        tags,
    ]
    go_block = _object_block(doc.newline, go_id, go_type, go_version, go_props)

    nl = doc.newline
    last = doc.objects_section.children[-1]
    insert = (last.end, last.end, nl + nl.join([go_block] + comp_blocks))
    edits = [insert, _uuid_array_edit(doc, parent_props, "Children", [go_id])]
    new_text = splice(doc.text, edits)
    parse(new_text)
    doc.save(new_text)
    print("game object  %s  %s" % (guid_to_string(*go_id), guid_to_u4(*go_id)))
    for ctype, cid in zip(args.component or [], comp_ids):
        print("component    %s  %s  %s" % (guid_to_string(*cid), guid_to_u4(*cid), ctype))
    print("Properties that are not written use the type's default. Set them with 'ezddl.py set'. Run 'ezddl.py touch' when done.")
    for t, fv, cv in lagging:
        print("WARNING: this file records %s as version %d, the code is at version %d. The new object is loaded as version %d and "
              "graph patches for %d..%d run on it. Unless those patches are no-ops for default properties, open and save the "
              "document in the editor first (that upgrades the file), then add the object." % (t, fv, cv, fv, fv + 1, cv))
    return 0


def cmd_type(args):
    types = type_index()
    if args.list:
        rx = re.compile(fnmatch_to_regex(args.name), re.IGNORECASE)
        for n in sorted(types):
            if rx.search(n):
                i = types[n]
                print("%-48s v%-3s %s:%d" % (n, i["version"], i["file"], i["line"]))
        return 0
    chain = type_chain(args.name)
    if not chain:
        raise SystemExit("error: no reflection block for '%s' found in Code/ (try --list with a pattern)" % args.name)
    for info in chain:
        print("%s  version %s  (%s:%d)%s" % (info["name"], info["version"], info["file"], info["line"], ("  base " + info["base"]) if info["base"] else ""))
        for p in info.get("properties", []):
            print("    %-28s %-28s %s" % (p["name"], p["macro"], p["args"]))
        if info.get("constants"):
            print("    values: " + ", ".join(info["constants"]))
    print("Property lists are extracted from source text: types and defaults are not resolved. The running editor's rtti_type_properties (MCP) is exact.")
    return 0


def cmd_example(args):
    roots = scan_roots(args)
    needle = 's %%t{"%s"}' % args.type
    best = []
    for path in iter_documents(roots):
        try:
            with open(path, "rb") as f:
                raw = f.read()
        except OSError:
            continue
        if needle.encode() not in raw:
            continue
        try:
            doc = Document(path)
        except DdlError:
            continue
        for oid in doc.order:
            o = doc.objects[oid]
            if doc.obj_type(o) == args.type:
                best.append((doc.obj_version(o) or 0, len(doc.obj_props(o).children or []) if doc.obj_props(o) else 0, path, o))
    if not best:
        print("no object of type '%s' found in %s" % (args.type, ", ".join(rel(r) for r in roots)))
        return 1
    best.sort(key=lambda b: (b[0], b[1]), reverse=True)
    pr = Printer(None, decode=not args.hex)
    print("// %d object(s) of this type found, showing the newest version (v%d) from %s" % (len(best), best[0][0], rel(best[0][2])))
    print("\n".join(pr.lines(best[0][3])))
    return 0


def cmd_selftest(args):
    """Parses every document and checks the encodings against data the engine wrote.

    Material assets that derive from a base material store the base both as a string ('BaseMaterial') and as a
    u4 Uuid ('MetaFromPrefab'), which makes them a ground truth for the GUID conversion.
    """
    assert float_to_hex(1.0) == "0x0000803F" and hex_to_float("0x0000803F") == 1.0
    assert float_to_hex(0.5) == "0x0000003F" and float_to_hex(0.0) == "0"
    assert float_to_hex(1.0, True) == "0x000000000000F03F"
    roots = scan_roots(args)
    n = failed = pairs = bad_pairs = 0
    for path in iter_documents(roots):
        if read_header_text(path) is None:
            continue
        n += 1
        try:
            doc = Document(path)
        except (DdlError, UnicodeDecodeError) as e:
            print("FAIL %s: %s" % (rel(path), e))
            failed += 1
            continue
        for oid in doc.order:
            o = doc.objects[oid]
            mfp = doc.prop(o, "MetaFromPrefab")
            base = doc.prop_value(o, "BaseMaterial")
            if mfp is not None and isinstance(base, str) and _RE_GUID_STR.fullmatch(base):
                pairs += 1
                low, high = mfp.prim().values
                if guid_to_string(low, high) != base or string_to_guid(base) != (low, high):
                    bad_pairs += 1
                    print("FAIL %s: u4{%d,%d} -> %s, but BaseMaterial is %s" % (rel(path), low, high, guid_to_string(low, high), base))
    print("parsed %d document(s), %d failed; GUID conversion checked on %d pair(s), %d wrong" % (n, failed, pairs, bad_pairs))
    return 1 if failed or bad_pairs or not pairs else 0


# ---------------------------------------------------------------------------------------------------------------


def main(argv=None):
    ap = argparse.ArgumentParser(prog="ezddl.py", description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    def scope(p):
        p.add_argument("--project", help="project folder (contains 'ezProject'); its data directories are indexed")
        p.add_argument("--root", action="append", help="directory to index instead (repeatable)")

    p = sub.add_parser("guid", help="convert GUIDs between '{ ... }' and 'u4{low,high}', make new ones",
                       description="ezddl.py guid <guid>...      convert (any form) and print both forms\n"
                                   "ezddl.py guid new [count]    fresh random GUIDs\n"
                                   "ezddl.py guid add <guid> <seed> / sub <guid> <seed>   prefab instance id math (CombineWithSeed)",
                       formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("values", nargs="*")
    p.add_argument("--resolve", "-r", action="store_true", help="also look up which asset has this GUID")
    scope(p)
    p.set_defaults(fn=cmd_guid)

    p = sub.add_parser("float", help="convert between DDL hex floats (0x0000803F) and decimals")
    p.add_argument("values", nargs="+")
    p.add_argument("--double", "-d", action="store_true", help="treat values as double (8 bytes)")
    p.set_defaults(fn=cmd_float)

    p = sub.add_parser("asset", help="find assets by GUID or by path pattern (substring, or * ? wildcards)")
    p.add_argument("query", nargs="+")
    scope(p)
    p.set_defaults(fn=cmd_asset)

    p = sub.add_parser("uses", help="list documents (and properties) that reference an asset")
    p.add_argument("asset", nargs="+", help="GUID or unique path pattern")
    p.add_argument("--files-only", "-l", action="store_true")
    scope(p)
    p.set_defaults(fn=cmd_uses)

    p = sub.add_parser("show", help="print a document with decimal floats, GUID strings and asset paths")
    p.add_argument("file")
    p.add_argument("--object", "-o", action="append", help="only this object (GUID in any form, or a game object Name)")
    p.add_argument("--type", "-t", action="append", help="only objects of this type (wildcards allowed)")
    p.add_argument("--section", "-s", action="append", choices=["HeaderV2", "Objects", "Types"], help="default: HeaderV2 and Objects")
    p.add_argument("--hex", action="store_true", help="keep floats in hex")
    p.add_argument("--full", action="store_true", help="do not shorten long strings (e.g. MetaBasePrefab)")
    p.add_argument("--no-resolve", action="store_true", help="do not index assets (faster)")
    scope(p)
    p.set_defaults(fn=cmd_show)

    p = sub.add_parser("tree", help="print the object hierarchy of a scene or prefab")
    p.add_argument("file")
    p.add_argument("--depth", type=int)
    p.add_argument("--no-components", action="store_true")
    p.add_argument("--props", action="store_true", help="show asset references of components")
    p.add_argument("--settings", action="store_true", help="print the document settings object")
    p.add_argument("--no-resolve", action="store_true")
    scope(p)
    p.set_defaults(fn=cmd_tree)

    p = sub.add_parser("check", help="validate documents: syntax, object references, asset references, type versions")
    p.add_argument("files", nargs="+")
    p.add_argument("--no-resolve", action="store_true", help="skip the asset reference check")
    p.add_argument("--no-versions", action="store_true", help="skip the type version check (scans Code/)")
    p.add_argument("--verbose", "-v", action="store_true", help="also list files without findings, and asset references missing from the header")
    scope(p)
    p.set_defaults(fn=cmd_check)

    p = sub.add_parser("touch", help="change the header Hash so that the editor re-transforms a hand-edited asset")
    p.add_argument("files", nargs="+")
    p.set_defaults(fn=cmd_touch)

    p = sub.add_parser("set", help="set one property of one object, encoding the value correctly")
    p.add_argument("file")
    p.add_argument("object", help="object GUID (any form) or unique game object Name")
    p.add_argument("property")
    p.add_argument("value", help="e.g. '1,2,3' (Vec3), '90deg' (Angle), '{ guid }' (asset ref), 'ezFoo::Bar' (enum)")
    p.add_argument("--type", help="DDL type when the property is not stored yet (f, d, b, s, i3, u3, Vec3, Quat, Color, Angle, Time, Uuid, ...)")
    p.add_argument("--raw", action="store_true", help="value is the DDL text that goes between the property's braces, e.g. '1.5' for f, 'f{1,2,3}' for Vec3")
    p.add_argument("--dry-run", "-n", action="store_true")
    p.set_defaults(fn=cmd_set)

    p = sub.add_parser("new-object", help="add a game object (with components) to a scene or prefab")
    p.add_argument("file")
    p.add_argument("--parent", help="parent game object (GUID or Name); default: top level")
    p.add_argument("--name")
    p.add_argument("--pos", help="x,y,z")
    p.add_argument("--rot", help="quaternion x,y,z,w")
    p.add_argument("--scale", help="x,y,z")
    p.add_argument("--tag", action="append")
    p.add_argument("--component", "-c", action="append", help="component type name (repeatable)")
    p.set_defaults(fn=cmd_new_object)

    p = sub.add_parser("type", help="version, base classes and properties of a reflected type, from the C++ source")
    p.add_argument("name")
    p.add_argument("--list", action="store_true", help="list type names matching a pattern")
    p.set_defaults(fn=cmd_type)

    p = sub.add_parser("example", help="print an existing object of a type from the data, as a template")
    p.add_argument("type")
    p.add_argument("--hex", action="store_true")
    scope(p)
    p.set_defaults(fn=cmd_example)

    p = sub.add_parser("selftest", help="parse every document below the scan roots")
    scope(p)
    p.set_defaults(fn=cmd_selftest)

    args = ap.parse_args(argv)
    try:
        return args.fn(args)
    except DdlError as e:
        print("error: %s" % e, file=sys.stderr)
        return 2
    except BrokenPipeError:
        return 0


if __name__ == "__main__":
    sys.exit(main())
