#ifndef GRAPHICSELEMENT_H
#define GRAPHICSELEMENT_H

#include "styledelement.h"

namespace lunasvg {

class GraphicsElement : public StyledElement {
public:
    GraphicsElement(ElementID id);

    Transform transform() const;
};

} // namespace lunasvg

#endif // GRAPHICSELEMENT_H
