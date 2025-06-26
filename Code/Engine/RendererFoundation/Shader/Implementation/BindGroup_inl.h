
ezGALBindGroupItem::ezGALBindGroupItem()
{
}

ezGALBindGroupItem::ezGALBindGroupItem(const ezGALBindGroupItem& rhs)
{
  *this = rhs;
}

void ezGALBindGroupItem::operator=(const ezGALBindGroupItem& rhs)
{
  ezHashableStruct<ezGALBindGroupItem>& thisBase = *this;
  const ezHashableStruct<ezGALBindGroupItem>& rhsBase = rhs;
  thisBase = rhsBase;
}
