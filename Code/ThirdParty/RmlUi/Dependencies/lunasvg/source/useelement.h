#ifndef USEELEMENT_H
#define USEELEMENT_H

#include "graphicselement.h"

namespace lunasvg {

class UseElement final : public GraphicsElement {
public:
    UseElement();

    Length x() const;
    Length y() const;
    Length width() const;
    Length height() const;
    std::string href() const;
    void transferWidthAndHeight(Element* element) const;

    void layout(LayoutContext* context, LayoutContainer* current) final;
    std::unique_ptr<Element> cloneTargetElement(const Element* targetElement) const;
    void build(const Document* document) final;
};

} // namespace lunasvg

#endif // USEELEMENT_H
