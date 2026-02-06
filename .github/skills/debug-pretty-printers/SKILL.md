---
name: debug-pretty-printers
description: Debug and develop GDB and LLDB pretty printers for ezEngine C++ types. Use this skill when working with ezEngine debugger visualizers in ezEngine-gdb.py, ezEngine.py, or ezEngine.natvis. Covers testing with VisualizerZoo.cpp, common errors, and debugging techniques.
---

# Debugging ezEngine Pretty Printers

This skill provides guidance for developing and debugging pretty printers (debugger visualizers) for ezEngine.

## File Locations

All pretty printer files are in `Code/Engine/Foundation/`:

- **GDB**: `Code/Engine/Foundation/ezEngine-gdb.py` - Python pretty printers for GDB
- **LLDB**: `Code/Engine/Foundation/ezEngine.py` - Python pretty printers for LLDB  
- **Natvis**: `Code/Engine/Foundation/ezEngine.natvis` - Visual Studio/MSVC visualizers (XML)
- **Auto-load**: `.gdbinit` in repository root - Automatically loads GDB printers

## Testing with VisualizerZoo.cpp

The primary test file for pretty printers is `Code/UnitTests/FoundationTest/CodeUtils/VisualizerZoo.cpp`. This file contains test variables for all visualized types organized in `EZ_TEST_BLOCK` sections.

### Finding test sections

Search for `EZ_TEST_BLOCK` macros to find each section:

```bash
grep -n "EZ_TEST_BLOCK" Code/UnitTests/FoundationTest/CodeUtils/VisualizerZoo.cpp
```
Eac hsection is named descriptively, e.g. `EZ_TEST_BLOCK(ezTestBlock::Enabled, "Strings")` contains tests for the string types.

### Setting breakpoints

Each `EZ_TEST_BLOCK` section ends with `EZ_TEST_BOOL(true);` - this is where you should set your breakpoint. At this point, all variables in the block are initialized and in scope.

To find the breakpoint line for a section:

```bash
# Find the section start, then look for the EZ_TEST_BOOL(true) before the closing brace
grep -n "EZ_TEST_BLOCK.*Strings" Code/UnitTests/FoundationTest/CodeUtils/VisualizerZoo.cpp
# Then search forward from that line for EZ_TEST_BOOL(true)
```

Or in an editor, search for `EZ_TEST_BLOCK.*SectionName`, then find the `EZ_TEST_BOOL(true);` at the end of that block.

### Running the test

First, ensure FoundationTest is built:

```bash
cmake --build Workspace/copilot --target FoundationTest
```

### Testing GDB printers in batch mode

Use this exact command pattern to test printers (run from the repository root):

```bash
gdb -batch \
  -ex "set pagination off" \
  -ex "source Code/Engine/Foundation/ezEngine-gdb.py" \
  -ex "break VisualizerZoo.cpp:LINE_NUMBER" \
  -ex "run -run -noGui -all" \
  -ex "print variableName" \
  ./Workspace/copilot-output/Bin/LinuxNinjaGccDebug64/FoundationTest
```

Replace `LINE_NUMBER` with the line of `EZ_TEST_BOOL(true);` in the relevant section.

Key points:
- Use `timeout 30` prefix if the test might hang
- The breakpoint must be on the `EZ_TEST_BOOL(true);` line at the END of the block (all variables initialized)
- Add `2>&1 | tail -15` to limit output

### Example: Testing string printers

1. Find the Strings section:
   ```bash
   grep -n "EZ_TEST_BLOCK.*Strings" Code/UnitTests/FoundationTest/CodeUtils/VisualizerZoo.cpp
   ```

2. Find the `EZ_TEST_BOOL(true);` at the end of that block and note the line number

3. Run GDB with that breakpoint (from repository root):
   ```bash
   gdb -batch \
     -ex "set pagination off" \
     -ex "source Code/Engine/Foundation/ezEngine-gdb.py" \
     -ex "break VisualizerZoo.cpp:101" \
     -ex "run -run -noGui -all" \
     -ex "print string" \
     -ex "print stringView" \
     ./Workspace/copilot-output/Bin/LinuxNinjaGccDebug64/FoundationTest
   ```

### Viewing expanded children

Use `set print array on` to see children on separate lines:

```bash
gdb -batch \
  -ex "set pagination off" \
  -ex "set print array on" \
  -ex "source Code/Engine/Foundation/ezEngine-gdb.py" \
  -ex "break VisualizerZoo.cpp:LINE_NUMBER" \
  -ex "run -run -noGui -all" \
  -ex "print variableName" \
  ./Workspace/copilot-output/Bin/LinuxNinjaGccDebug64/FoundationTest
```

## ezEngine GDB Printer Structure

### Registration pattern (from ezEngine-gdb.py)

```python
def build_pretty_printers():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("ezEngine")
    
    # Strings
    pp.add_printer('ezHybridStringBase', r'^ezHybridStringBase<.*>$', ezHybridStringPrinter)
    pp.add_printer('ezStringBuilder', r'^ezStringBuilder$', ezHybridStringPrinter)
    
    # Containers  
    pp.add_printer('ezDynamicArray', r'^ezDynamicArray<.*>$', ezDynamicArrayPrinter)
    
    return pp

gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printers())
```

### Printer class pattern

```python
class ezMyTypePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        """Return the display string for the value."""
        try:
            return f"{{ count={count} }}"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        """Yield (name, value) tuples for expandable children."""
        try:
            yield 'm_uiCount', self.val['m_uiCount']
            yield 'm_pAllocator', self.val['m_pAllocator']
            for i in range(count):
                yield f'[{i}]', (m_pElements + i).dereference()
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'  # or 'string' or 'map'
```

## Common Errors and Solutions

### "Type is not a template"

**Problem**: Calling `val.type.template_argument(0)` on `ezStringBuilder` (not a template).

**Solution**: Get size from the actual member instead:
```python
# Instead of: local_storage_size = val.type.template_argument(0)
local_storage_size = m_Data['m_StaticData'].type.sizeof
```

### "Trying to read string with inappropriate type"

**Problem**: Calling `.string()` on array address instead of `char*`.

**Solution**: Cast to `char*` first:
```python
char_ptr_type = gdb.lookup_type('char').pointer()
ptr = m_Data['m_StaticData'].address.cast(char_ptr_type)
result = ptr.string(length=count)
```

### "There is no member named X"

**Problem**: Wrong member name. Check the LLDB or Natvis implementation for correct names.

**Solution**: Use GDB to inspect the actual type:
```bash
gdb -batch -ex "ptype ezHashedString" ./Output/Bin/LinuxNinjaGccDebug64/FoundationTest
```

### Children showing as strings instead of expandable values

**Problem**: Yielding formatted strings in `children()` instead of GDB values.

**Solution**: Yield actual GDB values that have their own printers:
```python
# Wrong - yields a string, not expandable
yield 'c0', f"{{ x={x}, y={y}, z={z} }}"

# Correct - yields the actual value, GDB will use ezVec3Printer on it
yield 'c0', elements[0].address.cast(vec3_type.pointer()).dereference()
```

### ezHybridString inline vs heap storage

The `ezHybridStringBase` uses inline storage (`m_StaticData`) when capacity fits, otherwise heap (`m_pElements`):

```python
def _get_string_data(self):
    m_Data = self.val['m_Data']
    m_uiCount = int(m_Data['m_uiCount'])
    m_uiCapacity = int(m_Data['m_uiCapacity'])
    local_storage_size = m_Data['m_StaticData'].type.sizeof

    if m_uiCapacity <= local_storage_size:
        # Using inline storage
        char_ptr_type = gdb.lookup_type('char').pointer()
        ptr = m_Data['m_StaticData'].address.cast(char_ptr_type)
    else:
        # Using heap storage
        ptr = m_Data['m_pElements']

    return ptr, m_uiCount
```

### Template type lookup for math types

To cast array elements to vector types (e.g., for ezMat3 columns):

```python
elements = self.val['m_fElementsCM']
elem_type = elements[0].type
vec3_type = gdb.lookup_type(f'ezVec3Template<{elem_type}>')
col0 = elements[0].address.cast(vec3_type.pointer()).dereference()
```

## LLDB Comparison

When implementing GDB printers, reference the LLDB implementation in `ezEngine.py`:

- LLDB uses `set_fields(['m_uiCount', 'm_uiCapacity', 'm_pAllocator'])` to add member children
- GDB equivalent: yield each field in `children()` before array elements
- LLDB `GetChildMemberWithName('m_Data')` → GDB `self.val['m_Data']`
- LLDB `GetValueAsUnsigned(0)` → GDB `int(self.val['m_Member'])`

## Debugging Workflow

1. **Identify the type**: Check which printer handles it in `build_pretty_printers()`
2. **Find test variable**: Look in VisualizerZoo.cpp for a variable of that type
3. **Set breakpoint after initialization**: Use the table above for correct line numbers
4. **Test to_string() first**: Get the display string working before children()
5. **Add children() incrementally**: Test each child field separately
6. **Compare with LLDB/Natvis**: Use existing implementations as reference for member names

## Auto-loading

The `.gdbinit` file in the repository root automatically loads the printers when debugging in the ezEngine directory. It uses path detection relative to the current working directory.
