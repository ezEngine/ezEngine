#ifndef STOPELEMENT_H
#define STOPELEMENT_H

#include "styledelement.h"

namespace lunasvg {

class StopElement final : public StyledElement {
public:
    StopElement();

    double offset() const;
    Color stopColorWithOpacity() const;
};

} // namespace lunasvg

#endif // STOPELEMENT_H
