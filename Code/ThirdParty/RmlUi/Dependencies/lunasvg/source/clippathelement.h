#ifndef CLIPPATHELEMENT_H
#define CLIPPATHELEMENT_H

#include "graphicselement.h"

namespace lunasvg {

class LayoutClipPath;

class ClipPathElement final : public GraphicsElement {
public:
    ClipPathElement();

    Units clipPathUnits() const;
    std::unique_ptr<LayoutClipPath> getClipper(LayoutContext* context);
};

} // namespace lunasvg

#endif // CLIPPATHELEMENT_H
