#include "graphicselement.h"
#include "parser.h"

namespace lunasvg {

GraphicsElement::GraphicsElement(ElementID id)
    : StyledElement(id)
{
}

Transform GraphicsElement::transform() const
{
    auto& value = get(PropertyID::Transform);
    return Parser::parseTransform(value);
}

} // namespace lunasvg
