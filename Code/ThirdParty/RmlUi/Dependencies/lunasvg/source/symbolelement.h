#ifndef SYMBOLELEMENT_H
#define SYMBOLELEMENT_H

#include "styledelement.h"

namespace lunasvg {

class SymbolElement final : public StyledElement {
public:
    SymbolElement();

    Length x() const;
    Length y() const;
    Length width() const;
    Length height() const;
    Rect viewBox() const;
    PreserveAspectRatio preserveAspectRatio() const;
};

} // namespace lunasvg

#endif // SYMBOLELEMENT_H
