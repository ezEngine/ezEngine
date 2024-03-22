

inline ezDataDirPath::ezDataDirPath() = default;

inline ezDataDirPath::ezDataDirPath(ezStringView sAbsPath, ezArrayPtr<ezString> dataDirRoots, ezUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  EZ_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = sAbsPath;
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline ezDataDirPath::ezDataDirPath(const ezStringBuilder& sAbsPath, ezArrayPtr<ezString> dataDirRoots, ezUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  EZ_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = sAbsPath;
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline ezDataDirPath::ezDataDirPath(ezString&& sAbsPath, ezArrayPtr<ezString> dataDirRoots, ezUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  EZ_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = std::move(sAbsPath);
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline ezDataDirPath::operator ezStringView() const
{
  return m_sAbsolutePath;
}

inline bool ezDataDirPath::operator==(ezStringView rhs) const
{
  return m_sAbsolutePath == rhs;
}

inline bool ezDataDirPath::operator!=(ezStringView rhs) const
{
  return m_sAbsolutePath != rhs;
}

inline bool ezDataDirPath::IsValid() const
{
  return m_uiDataDirParent != 0;
}

inline void ezDataDirPath::Clear()
{
  m_sAbsolutePath.Clear();
  m_uiDataDirParent = 0;
  m_uiDataDirLength = 0;
  m_uiDataDirIndex = 0;
}

inline const ezString& ezDataDirPath::GetAbsolutePath() const
{
  return m_sAbsolutePath;
}

inline ezStringView ezDataDirPath::GetDataDirParentRelativePath() const
{
  EZ_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  const ezUInt32 uiOffset = m_uiDataDirParent + 1;
  return ezStringView(m_sAbsolutePath.GetData() + uiOffset, m_sAbsolutePath.GetElementCount() - uiOffset);
}

inline ezStringView ezDataDirPath::GetDataDirRelativePath() const
{
  EZ_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  const ezUInt32 uiOffset = ezMath::Min(m_sAbsolutePath.GetElementCount(), m_uiDataDirParent + m_uiDataDirLength + 1u);
  return ezStringView(m_sAbsolutePath.GetData() + uiOffset, m_sAbsolutePath.GetElementCount() - uiOffset);
}

inline ezStringView ezDataDirPath::GetDataDir() const
{
  EZ_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  return ezStringView(m_sAbsolutePath.GetData(), m_uiDataDirParent + m_uiDataDirLength);
}

inline ezUInt8 ezDataDirPath::GetDataDirIndex() const
{
  EZ_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  return m_uiDataDirIndex;
}

inline ezStreamWriter& ezDataDirPath::Write(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sAbsolutePath;
  inout_stream << m_uiDataDirParent;
  inout_stream << m_uiDataDirLength;
  inout_stream << m_uiDataDirIndex;
  return inout_stream;
}

inline ezStreamReader& ezDataDirPath::Read(ezStreamReader& inout_stream)
{
  inout_stream >> m_sAbsolutePath;
  inout_stream >> m_uiDataDirParent;
  inout_stream >> m_uiDataDirLength;
  inout_stream >> m_uiDataDirIndex;
  return inout_stream;
}

bool ezCompareDataDirPath::Less(ezStringView lhs, ezStringView rhs)
{
  int res = lhs.Compare_NoCase(rhs);
  if (res == 0)
  {
    return lhs.Compare(rhs) < 0;
  }

  return res < 0;
}

bool ezCompareDataDirPath::Equal(ezStringView lhs, ezStringView rhs)
{
  return lhs.IsEqual(rhs);
}

inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezDataDirPath& value)
{
  return value.Write(inout_stream);
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezDataDirPath& out_value)
{
  return out_value.Read(inout_stream);
}
