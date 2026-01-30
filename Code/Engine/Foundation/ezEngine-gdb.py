# GDB Pretty Printers for ezEngine
# Ported from ezEngine.natvis and ezEngine.py (LLDB)

import gdb
import gdb.printing
import re
import struct


def build_pretty_printers():
    """Build and return the pretty printer collection for ezEngine types."""
    pp = gdb.printing.RegexpCollectionPrettyPrinter("ezEngine")

    # Strings
    pp.add_printer('ezHybridStringBase', r'^ezHybridStringBase<.*>$', ezHybridStringPrinter)
    pp.add_printer('ezHybridString', r'^ezHybridString<.*>$', ezHybridStringPrinter)
    pp.add_printer('ezStringBuilder', r'^ezStringBuilder$', ezHybridStringPrinter)
    pp.add_printer('ezStringView', r'^ezStringView$', ezStringViewPrinter)
    pp.add_printer('ezHashedString', r'^ezHashedString$', ezHashedStringPrinter)
    pp.add_printer('ezStringIterator', r'^ezStringIterator$', ezStringIteratorPrinter)
    pp.add_printer('ezStringReverseIterator', r'^ezStringReverseIterator$', ezStringIteratorPrinter)

    # Containers
    pp.add_printer('ezDynamicArrayBase', r'^ezDynamicArrayBase<.*>$', ezDynamicArrayPrinter)
    pp.add_printer('ezDynamicArray', r'^ezDynamicArray<.*>$', ezDynamicArrayPrinter)
    pp.add_printer('ezHybridArray', r'^ezHybridArray<.*>$', ezDynamicArrayPrinter)
    pp.add_printer('ezArrayBase', r'^ezArrayBase<.*>$', ezDynamicArrayPrinter)
    pp.add_printer('ezSmallArrayBase', r'^ezSmallArrayBase<.*>$', ezSmallArrayPrinter)
    pp.add_printer('ezSmallArray', r'^ezSmallArray<.*>$', ezSmallArrayPrinter)
    pp.add_printer('ezStaticArray', r'^ezStaticArray<.*>$', ezStaticArrayPrinter)
    pp.add_printer('ezArrayPtr', r'^ezArrayPtr<.*>$', ezArrayPtrPrinter)
    pp.add_printer('ezByteArrayPtr', r'^ezByteArrayPtr$', ezArrayPtrPrinter)
    pp.add_printer('ezConstByteArrayPtr', r'^ezConstByteArrayPtr$', ezArrayPtrPrinter)
    pp.add_printer('ezHashTableBase', r'^ezHashTableBase<.*>$', ezHashTablePrinter)
    pp.add_printer('ezHashTable', r'^ezHashTable<.*>$', ezHashTablePrinter)
    pp.add_printer('ezHashSetBase', r'^ezHashSetBase<.*>$', ezHashTablePrinter)
    pp.add_printer('ezHashSet', r'^ezHashSet<.*>$', ezHashTablePrinter)
    pp.add_printer('ezListBase', r'^ezListBase<.*>$', ezListPrinter)
    pp.add_printer('ezList', r'^ezList<.*>$', ezListPrinter)
    pp.add_printer('ezDequeBase', r'^ezDequeBase<.*>$', ezDequePrinter)
    pp.add_printer('ezDeque', r'^ezDeque<.*>$', ezDequePrinter)
    pp.add_printer('ezMapBase', r'^ezMapBase<.*>$', ezMapPrinter)
    pp.add_printer('ezMap', r'^ezMap<.*>$', ezMapPrinter)
    pp.add_printer('ezSetBase', r'^ezSetBase<.*>$', ezMapPrinter)
    pp.add_printer('ezSet', r'^ezSet<.*>$', ezMapPrinter)
    pp.add_printer('ezStaticRingBuffer', r'^ezStaticRingBuffer<.*>$', ezStaticRingBufferPrinter)

    # Basic Types
    pp.add_printer('ezEnum', r'^ezEnum<.*>$', ezEnumPrinter)
    pp.add_printer('ezBitflags', r'^ezBitflags<.*>$', ezBitflagsPrinter)
    pp.add_printer('ezVec2Template', r'^ezVec2Template<.*>$', ezVec2Printer)
    pp.add_printer('ezVec3Template', r'^ezVec3Template<.*>$', ezVec3Printer)
    pp.add_printer('ezVec4Template', r'^ezVec4Template<.*>$', ezVec4Printer)
    pp.add_printer('ezQuatTemplate', r'^ezQuatTemplate<.*>$', ezQuatPrinter)
    pp.add_printer('ezPlaneTemplate', r'^ezPlaneTemplate<.*>$', ezPlanePrinter)
    pp.add_printer('ezColor', r'^ezColor$', ezColorPrinter)
    pp.add_printer('ezColorBaseUB', r'^ezColorBaseUB$', ezColorUBPrinter)
    pp.add_printer('ezColorLinearUB', r'^ezColorLinearUB$', ezColorUBPrinter)
    pp.add_printer('ezColorGammaUB', r'^ezColorGammaUB$', ezColorUBPrinter)
    pp.add_printer('ezTime', r'^ezTime$', ezTimePrinter)
    pp.add_printer('ezAngleTemplate', r'^ezAngleTemplate<.*>$', ezAnglePrinter)
    pp.add_printer('ezUuid', r'^ezUuid$', ezUuidPrinter)
    pp.add_printer('ezMat3Template', r'^ezMat3Template<.*>$', ezMat3Printer)
    pp.add_printer('ezMat4Template', r'^ezMat4Template<.*>$', ezMat4Printer)
    pp.add_printer('ezTransformTemplate', r'^ezTransformTemplate<.*>$', ezTransformPrinter)
    pp.add_printer('ezTypedPointer', r'^ezTypedPointer$', ezTypedPointerPrinter)
    pp.add_printer('ezVariant', r'^ezVariant$', ezVariantPrinter)

    # Smart Pointers
    pp.add_printer('ezUniquePtr', r'^ezUniquePtr<.*>$', ezUniquePtrPrinter)
    pp.add_printer('ezSharedPtr', r'^ezSharedPtr<.*>$', ezSharedPtrPrinter)
    pp.add_printer('ezScopedRefPointer', r'^ezScopedRefPointer<.*>$', ezScopedRefPointerPrinter)

    return pp


# =============================================================================
# String Printers
# =============================================================================

class ezHybridStringPrinter:
    """Pretty printer for ezHybridStringBase<N> and ezStringBuilder."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_Data = self.val['m_Data']
            m_pElements = m_Data['m_pElements']
            m_uiCount = int(m_Data['m_uiCount'])

            if m_uiCount == 0:
                return '""'
            if m_uiCount > 0x10000000:  # Detect uninitialized
                return "<uninitialized>"

            # String length excludes null terminator for display
            str_len = m_uiCount - 1 if m_uiCount > 0 else 0
            return m_pElements.string(length=str_len)
        except Exception as e:
            return f"<error: {e}>"

    def display_hint(self):
        return 'string'


class ezStringViewPrinter:
    """Pretty printer for ezStringView."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_pStart = self.val['m_pStart']
            m_uiElementCount = int(self.val['m_uiElementCount'])

            if m_pStart == 0 or m_uiElementCount == 0:
                return "<empty>"

            str_len = min(m_uiElementCount, 1024)
            return m_pStart.string(length=str_len)
        except Exception as e:
            return f"<error: {e}>"

    def display_hint(self):
        return 'string'


class ezHashedStringPrinter:
    """Pretty printer for ezHashedString."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_Data = self.val['m_Data']
            m_pElement = m_Data['m_pElement']
            m_Value = m_pElement['m_Value']
            m_sString = m_Value['m_sString']
            # m_sString is an ezHybridString, delegate to its printer
            return ezHybridStringPrinter(m_sString).to_string()
        except Exception as e:
            return f"<error: {e}>"

    def display_hint(self):
        return 'string'


class ezStringIteratorPrinter:
    """Pretty printer for ezStringIterator and ezStringReverseIterator."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_pCurPtr = self.val['m_pCurPtr']
            if m_pCurPtr == 0:
                return "<invalid iterator>"

            # Read up to 4 bytes for UTF-8 character
            char_bytes = []
            for i in range(4):
                b = int((m_pCurPtr + i).dereference())
                # Convert signed char (-128 to 127) to unsigned (0 to 255)
                if b < 0:
                    b = b + 256
                
                # For bytes after the first, check if it's a continuation byte
                # Continuation bytes have pattern 10xxxxxx (0x80-0xBF)
                if i > 0 and (b & 0xC0) != 0x80:
                    # This byte starts a new character, don't include it
                    break
                
                char_bytes.append(b)

            if not char_bytes:
                return "<empty>"

            # Decode UTF-8 character
            data = bytes(char_bytes)
            try:
                char = data.decode('utf-8')[0]
                return f"'{char}'"
            except:
                return f"<invalid UTF-8>"
        except Exception as e:
            return f"<error: {e}>"


# =============================================================================
# Container Printers
# =============================================================================

class ezDynamicArrayPrinter:
    """Pretty printer for ezDynamicArrayBase<T>, ezDynamicArray<T>, ezHybridArray<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['m_uiCount'])
        return f"{{ count={count} }}"

    def children(self):
        try:
            count = int(self.val['m_uiCount'])
            m_pElements = self.val['m_pElements']

            for i in range(count):
                yield f'[{i}]', (m_pElements + i).dereference()
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


class ezSmallArrayPrinter:
    """Pretty printer for ezSmallArrayBase<T, N> and ezSmallArray<T, N>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['m_uiCount'])
        return f"{{ count={count} }}"

    def children(self):
        try:
            count = int(self.val['m_uiCount'])
            capacity = int(self.val['m_uiCapacity'])

            # Get template argument for inline size
            val_type = self.val.type
            if val_type.code == gdb.TYPE_CODE_REF:
                val_type = val_type.target()
        
            # Get element type from template
            element_type = val_type.template_argument(0)
            element_size = element_type.sizeof
            inline_size = val_type.template_argument(1)

            # Check if using inline storage or external
            # When capacity <= inline_size, data is in m_StaticData
            if capacity <= inline_size:
                # Using inline storage - access through m_StaticData
                # m_StaticData is a byte array, we need to cast it to element type
                m_StaticData = self.val['m_StaticData']
                data_ptr = m_StaticData.address.cast(element_type.pointer())
                for i in range(count):
                    yield f'[{i}]', (data_ptr + i).dereference()
            else:
                # Using external storage - access through m_pElements
                m_pElements = self.val['m_pElements']
                for i in range(count):
                    yield f'[{i}]', (m_pElements + i).dereference()
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


class ezStaticArrayPrinter:
    """Pretty printer for ezStaticArray<T, N>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['m_uiCount'])
        return f"{{ count={count} }}"

    def children(self):
        try:
            count = int(self.val['m_uiCount'])
            m_Data = self.val['m_Data']

            # m_Data is a byte array (ezUInt8[N*sizeof(T)]), we need to cast it
            # to the actual element type to access elements
            element_type = self.val.type.template_argument(0)
            data_ptr = m_Data.address.cast(element_type.pointer())

            for i in range(count):
                yield f'[{i}]', (data_ptr + i).dereference()
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


class ezArrayPtrPrinter:
    """Pretty printer for ezArrayPtr<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['m_uiCount'])
        return f"{{ count={count} }}"

    def children(self):
        try:
            count = int(self.val['m_uiCount'])
            m_pPtr = self.val['m_pPtr']

            for i in range(count):
                yield f'[{i}]', (m_pPtr + i).dereference()
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


class ezHashTablePrinter:
    """Pretty printer for ezHashTableBase<K, V> and ezHashSetBase<K>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['m_uiCount'])
        return f"{{ count={count} }}"

    def children(self):
        try:
            count = int(self.val['m_uiCount'])
            capacity = int(self.val['m_uiCapacity'])
            m_pEntries = self.val['m_pEntries']
            m_pEntryFlags = self.val['m_pEntryFlags']

            displayed = 0
            index = 0

            while displayed < count and index < capacity:
                # Get flags for this entry
                # Flags are packed: 2 bits per entry, 16 entries per uint32
                flags_index = index // 16
                flag_shift = (index & 15) * 2
                flags_value = int((m_pEntryFlags + flags_index).dereference())
                entry_flags = (flags_value >> flag_shift) & 3

                # Check if entry is valid (bit 0 set)
                if (entry_flags & 0x01) == 0x01:
                    entry = (m_pEntries + index).dereference()
                    yield f'[{displayed}]', entry
                    displayed += 1

                index += 1
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


class ezListPrinter:
    """Pretty printer for ezListBase<T> and ezList<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['m_uiCount'])
        return f"{{ count={count} }}"

    def children(self):
        try:
            count = int(self.val['m_uiCount'])
            m_First = self.val['m_First']
            current = m_First['m_pNext']

            for i in range(count):
                data = current['m_Data']
                yield f'[{i}]', data
                current = current['m_pNext']
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


class ezDequePrinter:
    """Pretty printer for ezDequeBase<T> and ezDeque<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['m_uiCount'])
        return f"{{ count={count} }}"

    def children(self):
        try:
            count = int(self.val['m_uiCount'])
            m_pChunks = self.val['m_pChunks']
            first_element = int(self.val['m_uiFirstElement'])

            # Calculate chunk size
            element_type = m_pChunks.type.target().target()
            element_size = element_type.sizeof
            chunk_size = max(4096 // element_size, 32)

            for i in range(count):
                real_index = first_element + i
                chunk_index = real_index // chunk_size
                chunk_offset = real_index % chunk_size

                chunk = (m_pChunks + chunk_index).dereference()
                element = (chunk + chunk_offset).dereference()
                yield f'[{i}]', element
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


class ezMapPrinter:
    """Pretty printer for ezMapBase<K, V>, ezMap<K, V>, ezSetBase<K>, and ezSet<K>."""

    # Limit tree traversal to prevent infinite loops
    MAX_STEPS = 1000

    def __init__(self, val):
        self.val = val
        self.m_uiCount = int(val['m_uiCount'])
        self.m_pRoot = val['m_pRoot']
        self.m_NilNodeAddr = int(val['m_NilNode'].address)
        # Determine if this is a set by checking type name
        type_name = str(val.type)
        self.is_set = 'Set' in type_name

    def to_string(self):
        return f"{{ count={self.m_uiCount} }}"

    def GetLink(self, node, index):
        """Get left (0) or right (1) child link."""
        return node['m_pLink'][index]

    def GetParent(self, node):
        """Get parent node."""
        return node['m_pParent']

    def GetLeftMost(self, node):
        """Get the leftmost node in the subtree."""
        steps = self.MAX_STEPS
        while steps > 0:
            left = self.GetLink(node, 0)
            if int(left) == self.m_NilNodeAddr:
                break
            node = left.dereference()
            steps -= 1
        if steps == 0:
            return None
        return node

    def NextNode(self, node):
        """Get the next node in inorder traversal."""
        right = self.GetLink(node, 1)
        right_addr = int(right)

        # If this element has a right child, go there and then search for the leftmost child
        if right_addr != int(self.GetLink(right.dereference(), 1)):
            return self.GetLeftMost(right.dereference())

        parent = self.GetParent(node)
        parent_addr = int(parent)
        parent_parent_addr = int(self.GetParent(parent.dereference()))

        # If this element has a parent and this element is that parent's left child, go to parent
        if parent_addr != parent_parent_addr and int(self.GetLink(parent.dereference(), 0)) == int(node.address):
            return parent.dereference()

        # If this element has a parent and this element is that parent's right child,
        # search for the next parent whose left child this is
        if parent_addr != parent_parent_addr and int(self.GetLink(parent.dereference(), 1)) == int(node.address):
            steps = self.MAX_STEPS
            while steps > 0 and int(self.GetLink(self.GetParent(node).dereference(), 1)) == int(node.address):
                node = self.GetParent(node).dereference()
                steps -= 1

            if steps == 0:
                return None

            # If we are the root node
            parent = self.GetParent(node)
            if int(parent) == 0 or int(parent) == int(self.GetParent(parent.dereference())):
                return None

            return parent.dereference()

        return None

    def children(self):
        try:
            if self.m_uiCount == 0:
                return

            display_count = min(self.m_uiCount, 256)

            # Start from leftmost node
            current = self.GetLeftMost(self.m_pRoot.dereference())
            if current is None:
                return

            for i in range(display_count):
                if self.is_set:
                    # For sets, yield just the key
                    try:
                        yield f'[{i}]', current['m_Key']
                    except:
                        yield f'[{i}]', current
                else:
                    # For maps, yield the full node
                    yield f'[{i}]', current

                current = self.NextNode(current)
                if current is None:
                    break
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


class ezStaticRingBufferPrinter:
    """Pretty printer for ezStaticRingBuffer<T, N>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['m_uiCount'])
        return f"{{ count={count} }}"

    def children(self):
        try:
            count = int(self.val['m_uiCount'])
            first = int(self.val['m_uiFirstElement'])
            m_pElements = self.val['m_pElements']
            capacity = self.val.type.template_argument(1)
            yield 'count', count
            yield 'first', first

            for i in range(count):
                actual_index = (first + i) % capacity
                yield f'[{i}]', (m_pElements + actual_index).dereference()
        except Exception as e:
            yield 'error', str(e)

    def display_hint(self):
        return 'array'


# =============================================================================
# Basic Type Printers
# =============================================================================

class ezEnumPrinter:
    """Pretty printer for ezEnum<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_Value = int(self.val['m_Value'])
            # Get the enum type from the template argument
            try:
                inner_type = self.val.type.template_argument(0)
                enum_type_name = str(inner_type) + "::Enum"
                enum_type = gdb.lookup_type(enum_type_name)
                # Try to find the enum value name
                for field in enum_type.fields():
                    if field.enumval == m_Value:
                        return f"{field.name} ({m_Value})"
            except:
                pass
            return f"? ({m_Value})"
        except Exception as e:
            return f"<error: {e}>"


class ezBitflagsPrinter:
    """Pretty printer for ezBitflags<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_Value = int(self.val['m_Value'])
            if m_Value == 0:
                return "None (0)"

            # Get the enum type from the template argument
            try:
                inner_type = self.val.type.template_argument(0)
                enum_type_name = str(inner_type) + "::Enum"
                enum_type = gdb.lookup_type(enum_type_name)
                flags_list = []
                for field in enum_type.fields():
                    bit_value = field.enumval
                    # Skip if not a single bit flag
                    if bit_value == 0 or (bit_value & (bit_value - 1)) != 0:
                        continue
                    if m_Value & bit_value:
                        flags_list.append(field.name)

                if flags_list:
                    return " | ".join(flags_list) + f" ({m_Value})"
            except:
                pass
            return f"0x{m_Value:x} ({m_Value})"
        except Exception as e:
            return f"<error: {e}>"


class ezVec2Printer:
    """Pretty printer for ezVec2Template<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        x = self.val['x']
        y = self.val['y']
        return f"{{ x={x}, y={y} }}"

    def children(self):
        yield 'x', self.val['x']
        yield 'y', self.val['y']


class ezVec3Printer:
    """Pretty printer for ezVec3Template<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        x = self.val['x']
        y = self.val['y']
        z = self.val['z']
        return f"{{ x={x}, y={y}, z={z} }}"

    def children(self):
        yield 'x', self.val['x']
        yield 'y', self.val['y']
        yield 'z', self.val['z']


class ezVec4Printer:
    """Pretty printer for ezVec4Template<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        x = self.val['x']
        y = self.val['y']
        z = self.val['z']
        w = self.val['w']
        return f"{{ x={x}, y={y}, z={z}, w={w} }}"

    def children(self):
        yield 'x', self.val['x']
        yield 'y', self.val['y']
        yield 'z', self.val['z']
        yield 'w', self.val['w']


class ezQuatPrinter:
    """Pretty printer for ezQuatTemplate<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        x = self.val['x']
        y = self.val['y']
        z = self.val['z']
        w = self.val['w']
        return f"{{ x={x}, y={y}, z={z}, w={w} }}"

    def children(self):
        yield 'x', self.val['x']
        yield 'y', self.val['y']
        yield 'z', self.val['z']
        yield 'w', self.val['w']


class ezPlanePrinter:
    """Pretty printer for ezPlaneTemplate<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        normal = self.val['m_vNormal']
        nx = normal['x']
        ny = normal['y']
        nz = normal['z']
        negDist = self.val['m_fNegDistance']
        return f"{{ nx={nx}, ny={ny}, nz={nz}, negDist={negDist} }}"

    def children(self):
        yield 'normal', self.val['m_vNormal']
        yield 'negDistance', self.val['m_fNegDistance']


class ezColorPrinter:
    """Pretty printer for ezColor."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        r = self.val['r']
        g = self.val['g']
        b = self.val['b']
        a = self.val['a']
        return f"{{ r={r}, g={g}, b={b}, a={a} }}"

    def children(self):
        yield 'r', self.val['r']
        yield 'g', self.val['g']
        yield 'b', self.val['b']
        yield 'a', self.val['a']


class ezColorUBPrinter:
    """Pretty printer for ezColorBaseUB, ezColorLinearUB, ezColorGammaUB."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        r = int(self.val['r'])
        g = int(self.val['g'])
        b = int(self.val['b'])
        a = int(self.val['a'])
        return f"{{ r={r}, g={g}, b={b}, a={a} }}"

    def children(self):
        yield 'r', self.val['r']
        yield 'g', self.val['g']
        yield 'b', self.val['b']
        yield 'a', self.val['a']


class ezTimePrinter:
    """Pretty printer for ezTime."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        seconds = float(self.val['m_fTime'])
        return f"{{ seconds={seconds} }}"

    def children(self):
        seconds = float(self.val['m_fTime'])
        yield 'seconds', seconds
        yield 'milliseconds', seconds * 1000.0
        yield 'microseconds', seconds * 1000000.0
        yield 'nanoseconds', seconds * 1000000000.0


class ezAnglePrinter:
    """Pretty printer for ezAngle."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        radians = float(self.val['m_fRadian'])
        degrees = radians * 180.0 / 3.14159265358979323846
        return f"{{ Degree={degrees:.2f}° }}"

    def children(self):
        radians = float(self.val['m_fRadian'])
        degrees = radians * 180.0 / 3.14159265358979323846
        yield 'radians', radians
        yield 'degrees', degrees


class ezUuidPrinter:
    """Pretty printer for ezUuid."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_uiHigh = int(self.val['m_uiHigh'])
            m_uiLow = int(self.val['m_uiLow'])

            # Convert to bytes (little-endian storage)
            high_bytes = m_uiHigh.to_bytes(8, byteorder='little', signed=False)
            low_bytes = m_uiLow.to_bytes(8, byteorder='little', signed=False)

            # Format as UUID
            time_low = int.from_bytes(high_bytes[0:4], byteorder='little')
            time_mid = int.from_bytes(high_bytes[4:6], byteorder='little')
            time_hi_version = int.from_bytes(high_bytes[6:8], byteorder='little')
            clock_seq = int.from_bytes(low_bytes[0:2], byteorder='big')
            node = int.from_bytes(low_bytes[2:8], byteorder='big')

            return f"{{ {time_low:08x}-{time_mid:04x}-{time_hi_version:04x}-{clock_seq:04x}-{node:012x} }}"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            yield ('m_uiHigh', self.val['m_uiHigh'])
            yield ('m_uiLow', self.val['m_uiLow'])
        except:
            pass


class ezMat3Printer:
    """Pretty printer for ezMat3Template<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            elements = self.val['m_fElementsCM']
            c0 = (float(elements[0]), float(elements[1]), float(elements[2]))
            c1 = (float(elements[3]), float(elements[4]), float(elements[5]))
            c2 = (float(elements[6]), float(elements[7]), float(elements[8]))
            return f"ezMat3(c0={c0}, c1={c1}, c2={c2})"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            elements = self.val['m_fElementsCM']
            # Get the element type and create Vec3 pointers
            elem_type = elements[0].type
            vec3_type = gdb.lookup_type(f'ezVec3Template<{elem_type}>')
            yield 'c0', elements[0].address.cast(vec3_type.pointer()).dereference()
            yield 'c1', elements[3].address.cast(vec3_type.pointer()).dereference()
            yield 'c2', elements[6].address.cast(vec3_type.pointer()).dereference()
        except:
            # Fallback to strings if casting fails
            try:
                elements = self.val['m_fElementsCM']
                yield 'c0', f"({float(elements[0])}, {float(elements[1])}, {float(elements[2])})"
                yield 'c1', f"({float(elements[3])}, {float(elements[4])}, {float(elements[5])})"
                yield 'c2', f"({float(elements[6])}, {float(elements[7])}, {float(elements[8])})"
            except:
                pass


class ezMat4Printer:
    """Pretty printer for ezMat4Template<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            elements = self.val['m_fElementsCM']
            c0 = (float(elements[0]), float(elements[1]), float(elements[2]), float(elements[3]))
            c1 = (float(elements[4]), float(elements[5]), float(elements[6]), float(elements[7]))
            c2 = (float(elements[8]), float(elements[9]), float(elements[10]), float(elements[11]))
            c3 = (float(elements[12]), float(elements[13]), float(elements[14]), float(elements[15]))
            return f"ezMat4(c0={c0}, c1={c1}, c2={c2}, c3={c3})"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            elements = self.val['m_fElementsCM']
            # Get the element type and create Vec4 pointers
            elem_type = elements[0].type
            vec4_type = gdb.lookup_type(f'ezVec4Template<{elem_type}>')
            yield 'c0', elements[0].address.cast(vec4_type.pointer()).dereference()
            yield 'c1', elements[4].address.cast(vec4_type.pointer()).dereference()
            yield 'c2', elements[8].address.cast(vec4_type.pointer()).dereference()
            yield 'c3', elements[12].address.cast(vec4_type.pointer()).dereference()
        except:
            # Fallback to strings if casting fails
            try:
                elements = self.val['m_fElementsCM']
                yield 'c0', f"({float(elements[0])}, {float(elements[1])}, {float(elements[2])}, {float(elements[3])})"
                yield 'c1', f"({float(elements[4])}, {float(elements[5])}, {float(elements[6])}, {float(elements[7])})"
                yield 'c2', f"({float(elements[8])}, {float(elements[9])}, {float(elements[10])}, {float(elements[11])})"
                yield 'c3', f"({float(elements[12])}, {float(elements[13])}, {float(elements[14])}, {float(elements[15])})"
            except:
                pass


class ezTransformPrinter:
    """Pretty printer for ezTransformTemplate<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            pos = self.val['m_vPosition']
            rot = self.val['m_qRotation']
            scale = self.val['m_vScale']
            pos_str = f"({float(pos['x'])}, {float(pos['y'])}, {float(pos['z'])})"
            rot_str = f"({float(rot['x'])}, {float(rot['y'])}, {float(rot['z'])}, {float(rot['w'])})"
            scale_str = f"({float(scale['x'])}, {float(scale['y'])}, {float(scale['z'])})"
            return f"ezTransform(pos={pos_str}, rot={rot_str}, scale={scale_str})"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            yield 'position', self.val['m_vPosition']
            yield 'rotation', self.val['m_qRotation']
            yield 'scale', self.val['m_vScale']
        except:
            pass


class ezTypedPointerPrinter:
    """Pretty printer for ezTypedPointer."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_pObject = self.val['m_pObject']
            m_pType = self.val['m_pType']

            if m_pObject == 0:
                return "ezTypedPointer (nullptr)"

            if m_pType != 0:
                type_name = ezStringViewPrinter(m_pType.dereference()['m_sTypeName']).to_string()
                return f"({type_name}*) {int(m_pObject):#x}"
            else:
                return f"(void*) {int(m_pObject):#x}"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            m_pObject = self.val['m_pObject']
            m_pType = self.val['m_pType']

            if m_pObject != 0 and m_pType != 0:
                type_name = ezStringViewPrinter(m_pType.dereference()['m_sTypeName']).to_string()
                try:
                    target_type = gdb.lookup_type(type_name)
                    yield 'value', m_pObject.cast(target_type.pointer()).dereference()
                except:
                    yield 'ptr', m_pObject
        except:
            pass


class ezVariantPrinter:
    """Pretty printer for ezVariant."""

    # Type mapping from ezVariantType enum values to type names
    # Note: FirstStandardType=1 and FirstExtendedType=64 are markers, not actual types
    # Standard types start at 2 (Bool), extended types start at 65 (VariantArray)
    VARIANT_TYPES = {
        0: ('Invalid', None),
        # Standard types (FirstStandardType=1 is just a marker)
        2: ('Bool', 'bool'),
        3: ('Int8', 'signed char'),
        4: ('UInt8', 'unsigned char'),
        5: ('Int16', 'short'),
        6: ('UInt16', 'unsigned short'),
        7: ('Int32', 'int'),
        8: ('UInt32', 'unsigned int'),
        9: ('Int64', 'long long'),
        10: ('UInt64', 'unsigned long long'),
        11: ('Float', 'float'),
        12: ('Double', 'double'),
        13: ('Color', 'ezColor'),
        14: ('Vector2', 'ezVec2'),
        15: ('Vector3', 'ezVec3'),
        16: ('Vector4', 'ezVec4'),
        17: ('Vector2I', 'ezVec2I32'),
        18: ('Vector3I', 'ezVec3I32'),
        19: ('Vector4I', 'ezVec4I32'),
        20: ('Vector2U', 'ezVec2U32'),
        21: ('Vector3U', 'ezVec3U32'),
        22: ('Vector4U', 'ezVec4U32'),
        23: ('Quaternion', 'ezQuat'),
        24: ('Matrix3', 'ezMat3'),
        25: ('Matrix4', 'ezMat4'),
        26: ('Transform', 'ezTransform'),
        27: ('String', 'ezString'),
        28: ('StringView', 'ezStringView'),
        29: ('DataBuffer', 'ezDataBuffer'),
        30: ('Time', 'ezTime'),
        31: ('Uuid', 'ezUuid'),
        32: ('Angle', 'ezAngle'),
        33: ('ColorGamma', 'ezColorGammaUB'),
        34: ('HashedString', 'ezHashedString'),
        35: ('TempHashedString', 'ezTempHashedString'),
        # Extended types (FirstExtendedType=64 is just a marker)
        65: ('VariantArray', 'ezVariantArray'),
        66: ('VariantDictionary', 'ezVariantDictionary'),
        67: ('TypedPointer', 'ezTypedPointer'),
        68: ('TypedObject', None),
    }

    def __init__(self, val):
        self.val = val

    def _get_value(self):
        """Extract the value from the variant, returns (value, error_msg)."""
        try:
            m_uiType = int(self.val['m_uiType'])
            m_bIsShared = int(self.val['m_bIsShared'])
            m_Data = self.val['m_Data']

            if m_uiType == 0:  # Invalid
                return None, None

            if m_uiType not in self.VARIANT_TYPES:
                return m_Data, None

            type_name, cast_type = self.VARIANT_TYPES[m_uiType]

            if cast_type:
                try:
                    target_type = gdb.lookup_type(cast_type)
                    if m_bIsShared:
                        ptr = m_Data['shared']['m_Ptr']
                        value = ptr.cast(target_type.pointer()).dereference()
                    else:
                        value = m_Data.cast(target_type)
                    return value, None
                except Exception as e:
                    return None, f"<cast error: {e}>"
            elif m_uiType == 68:  # TypedObject
                try:
                    if m_bIsShared:
                        # shared is a pointer to SharedData, dereference to access members
                        shared_ptr = m_Data['shared']
                        rtti = shared_ptr.dereference()['m_pType']
                        ptr = shared_ptr.dereference()['m_Ptr']
                    else:
                        rtti = m_Data['inlined']['m_pType']
                        ptr = m_Data['inlined']['m_Data'].address
                    # rtti is a pointer, dereference to access m_sTypeName (which is ezStringView)
                    type_name_str = ezStringViewPrinter(rtti.dereference()['m_sTypeName']).to_string()
                    # Try to look up the type and cast the pointer to it
                    try:
                        target_type = gdb.lookup_type(type_name_str)
                        value = ptr.cast(target_type.pointer()).dereference()
                        return value, None
                    except:
                        # Fallback to GDB-evaluable string if type lookup fails
                        return f"*({type_name_str}*){ptr}", None
                except Exception as e:
                    return f"<TypedObject: {e}>", None
            else:
                return m_Data, None
        except Exception as e:
            return None, str(e)

    def to_string(self):
        try:
            m_uiType = int(self.val['m_uiType'])

            if m_uiType in self.VARIANT_TYPES:
                type_name, _ = self.VARIANT_TYPES[m_uiType]
            else:
                type_name = f"Unknown({m_uiType})"

            if m_uiType == 0:  # Invalid
                return "(Invalid)"

            # TypedObject: show compact type@address format
            if m_uiType == 68:  # TypedObject
                try:
                    m_bIsShared = int(self.val['m_bIsShared'])
                    m_Data = self.val['m_Data']
                    if m_bIsShared:
                        shared_ptr = m_Data['shared']
                        rtti = shared_ptr.dereference()['m_pType']
                        ptr = shared_ptr.dereference()['m_Ptr']
                    else:
                        rtti = m_Data['inlined']['m_pType']
                        ptr = m_Data['inlined']['m_Data'].address
                    obj_type_name = ezStringViewPrinter(rtti.dereference()['m_sTypeName']).to_string()
                    return f"({type_name}) {obj_type_name}@{ptr}"
                except Exception as e:
                    return f"({type_name}) <error: {e}>"

            # Get the value for inline display
            value, error = self._get_value()
            if error:
                return f"({type_name}) {error}"
            if value is not None:
                if m_uiType == 13:  # Color
                    return f"({type_name}) {ezColorPrinter(value).to_string()}"
                elif m_uiType in (14, 17, 20):  # Vector2, Vector2I, Vector2U
                    return f"({type_name}) {ezVec2Printer(value).to_string()}"
                elif m_uiType in (15, 18, 21):  # Vector3, Vector3I, Vector3U
                    return f"({type_name}) {ezVec3Printer(value).to_string()}"
                elif m_uiType in (16, 19, 22):  # Vector4, Vector4I, Vector4U
                    return f"({type_name}) {ezVec4Printer(value).to_string()}"
                elif m_uiType == 23:  # Quaternion
                    return f"({type_name}) {ezQuatPrinter(value).to_string()}"
                elif m_uiType == 24:  # Matrix3
                    return f"({type_name}) {ezMat3Printer(value).to_string()}"
                elif m_uiType == 25:  # Matrix4
                    return f"({type_name}) {ezMat4Printer(value).to_string()}"
                elif m_uiType == 26:  # Transform
                    return f"({type_name}) {ezTransformPrinter(value).to_string()}"
                elif m_uiType == 29:  # DataBuffer
                    return f"({type_name}) {ezDynamicArrayPrinter(value).to_string()}"
                elif m_uiType == 30:  # Time
                    return f"({type_name}) {ezTimePrinter(value).to_string()}"
                elif m_uiType == 31:  # Uuid
                    return f"({type_name}) {ezUuidPrinter(value).to_string()}"
                elif m_uiType == 32:  # Angle
                    return f"({type_name}) {ezAnglePrinter(value).to_string()}"
                elif m_uiType == 33:  # ColorGamma
                    return f"({type_name}) {ezColorUBPrinter(value).to_string()}"
                elif m_uiType == 65:  # VariantArray
                    return f"({type_name}) {ezDynamicArrayPrinter(value).to_string()}"
                elif m_uiType == 66:  # VariantDictionary
                    return f"({type_name}) {ezHashTablePrinter(value).to_string()}"
                elif m_uiType == 67:  # TypedPointer
                    return f"({type_name}) {ezTypedPointerPrinter(value).to_string()}"
                return f"({type_name}) {value}"
            return f"({type_name})"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            m_uiType = int(self.val['m_uiType'])
            m_bIsShared = int(self.val['m_bIsShared'])

            yield 'Type', m_uiType

            if m_uiType == 0:  # Invalid
                return  # Don't yield Value for Invalid

            value, error = self._get_value()
            if error:
                yield 'Value', error
            elif value is not None:
                
                # For TypedPointer (67) and other types, yield the raw value so it can be expanded
                yield 'Value', value

            yield 'IsShared', bool(m_bIsShared)
        except Exception as e:
            yield 'error', str(e)


# =============================================================================
# Smart Pointer Printers
# =============================================================================

class ezUniquePtrPrinter:
    """Pretty printer for ezUniquePtr<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_pInstance = self.val['m_pInstance']
            if m_pInstance == 0:
                return "ezUniquePtr (nullptr)"
            ptr_type = m_pInstance.type
            ptr_addr = int(m_pInstance)
            return f"({ptr_type}) {ptr_addr:#x}"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            m_pInstance = self.val['m_pInstance']
            if m_pInstance != 0:
                yield 'ptr', m_pInstance
        except Exception as e:
            yield 'error', str(e)


class ezSharedPtrPrinter:
    """Pretty printer for ezSharedPtr<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_pInstance = self.val['m_pInstance']
            if m_pInstance == 0:
                return "ezSharedPtr (nullptr)"
            ptr_type = m_pInstance.type
            ptr_addr = int(m_pInstance)
            return f"({ptr_type}) {ptr_addr:#x}"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            m_pInstance = self.val['m_pInstance']
            if m_pInstance != 0:
                yield 'ptr', m_pInstance
        except Exception as e:
            yield 'error', str(e)


class ezScopedRefPointerPrinter:
    """Pretty printer for ezScopedRefPointer<T>."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_pReferencedObject = self.val['m_pReferencedObject']
            if m_pReferencedObject == 0:
                return "ezScopedRefPointer (nullptr)"
            ptr_type = m_pReferencedObject.type
            ptr_addr = int(m_pReferencedObject)
            return f"({ptr_type}) {ptr_addr:#x}"
        except Exception as e:
            return f"<error: {e}>"

    def children(self):
        try:
            m_pReferencedObject = self.val['m_pReferencedObject']
            if m_pReferencedObject != 0:
                yield 'ptr', m_pReferencedObject
        except Exception as e:
            yield 'error', str(e)


# =============================================================================
# Registration
# =============================================================================

def register_ezengine_printers(objfile):
    """Register pretty printers with the given objfile (or globally if None)."""
    gdb.printing.register_pretty_printer(objfile, build_pretty_printers())


# Auto-register when this module is loaded
register_ezengine_printers(gdb.current_objfile())

print("ezEngine GDB pretty printers loaded.")
