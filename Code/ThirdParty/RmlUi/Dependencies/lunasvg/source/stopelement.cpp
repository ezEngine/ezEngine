#include "stopelement.h"
#include "parser.h"

namespace lunasvg {

StopElement::StopElement()
    : StyledElement(ElementID::Stop)
{
}

double StopElement::offset() const
{
    auto& value = get(PropertyID::Offset);
    return Parser::parseNumberPercentage(value, 0.0);
}

Color StopElement::stopColorWithOpacity() const
{
    auto color = stop_color();
    color.combine(stop_opacity());
    return color;
}

} // namespace lunasvg
