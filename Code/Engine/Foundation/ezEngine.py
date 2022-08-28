import lldb.formatters.Logger
import lldb

def __lldb_init_module(debugger, internal_dict):
    # comment this in for debug output
    lldb.formatters.Logger._lldb_formatters_debug_level = 2
    debugger.HandleCommand('type synthetic add -x "^ezDynamicArray<" --python-class ezEngine.ezDynamicArraySynthProvider')
    debugger.HandleCommand('type synthetic add -x "^ezHybridArray<" --python-class ezEngine.ezHybridArraySynthProvider')
    debugger.HandleCommand('type synthetic add -x "^ezHybridString<" --python-class ezEngine.ezHybridStringSynthProvider')
    debugger.HandleCommand('type synthetic add ezStringBuilder --python-class ezEngine.ezHybridStringSynthProvider')
    debugger.HandleCommand('type synthetic add ezStringView --python-class ezEngine.ezStringViewSynthProvider')
    debugger.HandleCommand('type synthetic add -x "^ezEnum<" --python-class ezEngine.ezEnumSynthProvider')
    debugger.HandleCommand('type synthetic add -x "^ezArrayPtr<" --python-class ezEngine.ezArrayPtrSynthProvider')
    debugger.HandleCommand('type synthetic add ezByteArrayPtr --python-class ezEngine.ezArrayPtrSynthProvider')
    debugger.HandleCommand('type synthetic add ezConstByteArrayPtr --python-class ezEngine.ezArrayPtrSynthProvider')

    debugger.HandleCommand('type summary add -x "^ezHybridString<" --python-function ezEngine.ezHybridString_SummaryProvider')
    debugger.HandleCommand('type summary add ezStringBuilder --python-function ezEngine.ezHybridString_SummaryProvider')
    debugger.HandleCommand('type summary add ezStringView --python-function ezEngine.ezStringView_SummaryProvider')
    debugger.HandleCommand('type summary add -x "^ezEnum<" --python-function ezEngine.ezEnum_SummaryProvider')

def make_string(F):
    return bytearray(F.GetData().uint8[:-1]).decode("utf-8")

class ezDynamicArraySynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        try:
            self.m_pElements = self.valobj.GetChildMemberWithName('m_pElements')
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.m_uiCapacity = self.valobj.GetChildMemberWithName('m_uiCapacity')
            self.m_pAllocator = self.valobj.GetChildMemberWithName('m_pAllocator')
            self.element_type = self.m_pElements.GetType().GetPointeeType()
            self.element_size = self.element_type.GetByteSize()
        except Exception as inst:
            logger >> inst

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        logger >> "num_children" + str(self.m_uiCount)
        numElements = self.m_uiCount.GetValueAsUnsigned(0)
        if numElements > 0xff000000:
            return 3
        numElements = min(numElements, 256)
        return numElements + 3

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        logger >> "Getting child" + str(index)
        if index < 3:
            if index == 0:
                return self.m_uiCount
            if index == 1:
                return self.m_uiCapacity
            if index == 2:
                return self.m_pAllocator
        else:
            index = index - 3

        if index >= self.num_children() - 3:
            return None
        try:
            offset = index * self.element_size
            return self.m_pElements.CreateChildAtOffset('[' + str(index) + ']', offset, self.element_type)
        except exception as inst:
            logger >> inst
            return None

class ezHybridArraySynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            self.m_pElements = self.valobj.GetChildMemberWithName('m_pElements')
            self.m_StaticData = self.valobj.GetChildAtIndex(1).GetChildAtIndex(0)
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.m_uiCapacity = self.valobj.GetChildMemberWithName('m_uiCapacity')
            self.m_pAllocator = self.valobj.GetChildMemberWithName('m_pAllocator')
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)
            self.element_size = self.element_type.GetByteSize()
            self.local_storage_size = self.m_StaticData.GetType().GetByteSize() // self.element_size
            logger >> "m_StaticData " + str(self.m_StaticData.GetType().GetDisplayTypeName())
            logger >> "local_storage_size " + str(self.local_storage_size)
        except Exception as inst:
            logger >> inst

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        logger >> "num_children" + str(self.m_uiCount)
        numElements = self.m_uiCount.GetValueAsUnsigned(0)
        if numElements > 0x10000000:
            return 3
        numElements = min(numElements, 256)
        return numElements + 3

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        logger >> "Getting child" + str(index)
        if index < 3:
            if index == 0:
                return self.m_uiCount
            if index == 1:
                return self.m_uiCapacity
            if index == 2:
                return self.m_pAllocator
        else:
            index = index - 3

        if index >= self.num_children() - 3:
            return None
        try:
            offset = index * self.element_size
            if self.m_uiCount.GetValueAsUnsigned(0) <= self.local_storage_size:
                return self.m_StaticData.CreateChildAtOffset('[' + str(index) + ']', offset, self.element_type)
            else:
                return self.m_pElements.CreateChildAtOffset('[' + str(index) + ']', offset, self.element_type)
        except exception as inst:
            logger >> inst
            return None

class ezHybridStringSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            m_Data = self.valobj.GetChildMemberWithName('m_Data')
            self.m_uiCharacterCount = self.valobj.GetChildMemberWithName('m_uiCharacterCount')
            self.m_pElements = m_Data.GetChildMemberWithName('m_pElements')
            self.m_StaticData = m_Data.GetChildAtIndex(1).GetChildAtIndex(0)
            self.m_uiCount = m_Data.GetChildMemberWithName('m_uiCount')
            self.m_uiCapacity = m_Data.GetChildMemberWithName('m_uiCapacity')
            self.m_pAllocator = m_Data.GetChildMemberWithName('m_pAllocator')
            self.local_storage_size = self.m_StaticData.GetType().GetByteSize()
            logger >> "local_storage_size " + str(self.local_storage_size)
        except Exception as inst:
            logger >> inst

        return False

    def num_children(self):
        return 4

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        logger >> "Getting child" + str(index)

        try:
            if index == 0:
                count = self.m_uiCount.GetValueAsUnsigned(0) 
                if count > 0x10000000:
                    count = 0
                if count <= self.local_storage_size:
                    return self.m_StaticData.CreateValueFromData('contents', self.m_StaticData.GetData(), self.m_pElements.GetType().GetPointeeType().GetArrayType(count))
                else:
                    return self.m_pElements.CreateValueFromData('contents', self.m_pElements.GetPointeeData(0, count), self.m_pElements.GetType().GetPointeeType().GetArrayType(count))
            elif index == 1:
                return self.m_uiCount
            elif index == 2:
                return self.m_uiCharacterCount
            elif index == 3:
                return self.m_pAllocator
        except Exception as inst:
            logger >> str(inst)
            return None


def ezHybridString_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        logger >> "ezHybridString_SummaryProvider"
        content = valobj.GetChildAtIndex(0)
        count = valobj.GetChildAtIndex(1).GetValueAsUnsigned(0)
        if count > 0x10000000:
            count = 0
        if count == 0:
            return "<empty>"
        if count > 1024:
            count = 1024
        return make_string(content)
    except Exception as inst:
        logger >> str(inst)
        return "<error>"

class ezStringViewSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        logger >> "ezStringViewSynthProvider::update"
        self.valid = False
        try:
            self.m_pStart = self.valobj.GetChildMemberWithName('m_pStart')
            self.m_pEnd = self.valobj.GetChildMemberWithName('m_pEnd')

            start = self.m_pStart.GetValueAsUnsigned(0)
            end = self.m_pEnd.GetValueAsUnsigned(0)
            logger >> str(start) + " " + str(end)
            if start == 0 or end == 0 or start > end:
                self.valid = False
            else:
                self.valid = True
                self.count = end - start
                logger >> "count " + str(self.count)
            
        except Exception as inst:
            logger >> inst

        return False

    def num_children(self):
        return 1

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        logger >> "Getting child" + str(index)

        if not self.valid:
            return None

        try:
            count = min(self.count, 256)
            return self.m_pStart.CreateValueFromData('contents', self.m_pStart.GetPointeeData(0, count), self.m_pStart.GetType().GetPointeeType().GetArrayType(count))
        except Exception as inst:
            logger >> str(inst)
            return None

def ezStringView_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        logger >> "ezStringView_SummaryProvider"
        content = valobj.GetChildAtIndex(0)
        return '"' + make_string(content) + '"'
    except Exception as inst:
        logger >> str(inst)
        return "<error>"

class ezEnumSynthProvider:
    def __init__(self, valobj, dict):
        self.valobj = valobj


    def update(self):
        self.value = "?"
        logger = lldb.formatters.Logger.Logger()
        logger >> "ezEnumSynthProvider::Update"
        try:
            enum_name = self.valobj.GetType().GetTemplateArgumentType(0).GetName() + "::Enum"
            logger >> enum_name
            enum_type = self.valobj.GetTarget().FindTypes(enum_name).GetTypeAtIndex(0)
            logger >> str(enum_type)
            value = self.valobj.GetChildMemberWithName("m_value").GetValueAsUnsigned()
            self.value = "? ({})".format(value)
            for v in enum_type.GetEnumMembers():
                if value == v.GetValueAsUnsigned():
                    self.value = "{} ({})".format(v.GetName(), v.GetValueAsUnsigned())
                    break
        except Exception as inst:
            logger >> str(inst)
        return False

    def num_children(self):
        return 1

    def get_child_at_index(self, index):
        return self.valobj.CreateValueFromExpression('value', '"' + self.value + '"')

def ezEnum_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        logger >> "ezEnum_SummaryProvider"
        content = valobj.GetChildAtIndex(0)
        return make_string(content)
    except Exception as inst:
        logger >> str(inst)
        return "<error>"

class ezArrayPtrSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        try:
            self.m_ptr = self.valobj.GetChildMemberWithName('m_ptr')
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.element_type = self.m_ptr.GetType().GetPointeeType()
            self.element_size = self.element_type.GetByteSize()
        except Exception as inst:
            logger >> inst

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        logger >> "num_children" + str(self.m_uiCount)
        numElements = self.m_uiCount.GetValueAsUnsigned(0)
        if numElements > 0xff000000:
            return 1
        numElements = min(numElements, 256)
        return numElements + 1

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        logger >> "Getting child" + str(index)
        if index < 1:
            if index == 0:
                return self.m_uiCount
        else:
            index = index - 1

        if index >= self.num_children() - 1:
            return None
        try:
            offset = index * self.element_size
            return self.m_ptr.CreateChildAtOffset('[' + str(index) + ']', offset, self.element_type)
        except exception as inst:
            logger >> str(inst)
            return None

class ezMapSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        try:
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.m_pRoot = self.valobj.GetChildMemberWithName('m_pRoot')
            self.m_NilNodeAddr = self.valobj.GetChildMemberWithName('m_NilNode').GetAddress()
            self.element_type = self.m_ptr.GetType().GetPointeeType()
            self.element_size = self.element_type.GetByteSize()
        except Exception as inst:
            logger >> inst

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        logger >> "num_children" + str(self.m_uiCount)
        numElements = self.m_uiCount.GetValueAsUnsigned(0)
        if numElements > 0xff000000:
            return 1
        numElements = min(numElements, 256)
        return numElements + 1

    def GetLink(self, node, index):
        return node.Dereference().GetChildMemberWithName('m_pLink').GetChildAtIndex(index)

    def GetParent(self, node):
        return node.Dereference().GetChildMemberWithName('m_pParent')

    def GetLeftMost(self):
        node = self.m_pRoot

        while self.GetLink(node, 0).GetAddress() != self.m_NilNodeAddr:
            node = self.GetLink(node, 0)

        return node

    def NextNode(self, node):
        rightNode = self.GetLink(node, 1)

        # if this element has a right child, go there and then search for the left most child of that
        if rightNode.GetAddress() != self.GetLink(rightNode, 1).GetAddress():
            node = rightNode

            while self.GetLink(node, 0).GetAddress() != self.GetLink(self.GetNode(node, 0), 0).GetAddress():
                node = self.GetLink(node, 0)

            return node

        parent = self.GetParent(node)
        parentParent = self.GetParent(parent)

        # if this element has a parent and this element is that parents left child, go directly to the parent
        if parent.GetAddress() != parentParent.GetAddress() and self.GetLink(parent, 0).GetAddress() == node.GetAddress():
            return parent

        # if this element has a parent and this element is that parents right child, search for the next parent, whose left child this is
        if parent.GetAddress() != parentParent.GetAddress() and self.GetLink(parent, 1).GetAddress() == node.GetAddress():
            while self.GetLink(self.GetParent(node), 1).GetAddress() == node.GetAddress():
                node = self.GetParent(node)

            # TODO continue

            

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        logger >> "Getting child" + str(index)
        if index < 1:
            if index == 0:
                return self.m_uiCount
        else:
            index = index - 1

        if index >= self.num_children() - 1:
            return None
        try:
            offset = index * self.element_size
            return self.m_ptr.CreateChildAtOffset('[' + str(index) + ']', offset, self.element_type)
        except exception as inst:
            logger >> str(inst)
            return None