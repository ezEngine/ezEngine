#include "symbolelement.h"
#include "parser.h"

namespace lunasvg {

SymbolElement::SymbolElement()
    : StyledElement(ElementID::Symbol)
{
}

Length SymbolElement::x() const
{
    auto& value = get(PropertyID::X);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length SymbolElement::y() const
{
    auto& value = get(PropertyID::Y);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length SymbolElement::width() const
{
    auto& value = get(PropertyID::Width);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::HundredPercent);
}

Length SymbolElement::height() const
{
    auto& value = get(PropertyID::Height);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::HundredPercent);
}

Rect SymbolElement::viewBox() const
{
    auto& value = get(PropertyID::ViewBox);
    return Parser::parseViewBox(value);
}

PreserveAspectRatio SymbolElement::preserveAspectRatio() const
{
    auto& value = get(PropertyID::PreserveAspectRatio);
    return Parser::parsePreserveAspectRatio(value);
}

} // namespace lunasvg
