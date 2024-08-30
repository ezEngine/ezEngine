#include "layoutcontext.h"
#include "parser.h"

#include "maskelement.h"
#include "clippathelement.h"
#include "paintelement.h"
#include "markerelement.h"
#include "geometryelement.h"

#include <cmath>

namespace lunasvg {

LayoutObject::LayoutObject(Node* node, LayoutId id)
    : m_node(node), m_id(id)
{
    node->setBox(this);
}

LayoutContainer::LayoutContainer(Node* node, LayoutId id)
    : LayoutObject(node, id)
{
}

const Rect& LayoutContainer::fillBoundingBox() const
{
    if(m_fillBoundingBox.valid())
        return m_fillBoundingBox;
    for(const auto& child : m_children) {
        if(child->isHidden())
            continue;
        m_fillBoundingBox.unite(child->map(child->fillBoundingBox()));
    }

    return m_fillBoundingBox;
}

const Rect& LayoutContainer::strokeBoundingBox() const
{
    if(m_strokeBoundingBox.valid())
        return m_strokeBoundingBox;
    for(const auto& child : m_children) {
        if(child->isHidden())
            continue;
        m_strokeBoundingBox.unite(child->map(child->strokeBoundingBox()));
    }

    return m_strokeBoundingBox;
}

LayoutObject* LayoutContainer::addChild(std::unique_ptr<LayoutObject> child)
{
    m_children.push_back(std::move(child));
    return &*m_children.back();
}

LayoutObject* LayoutContainer::addChildIfNotEmpty(std::unique_ptr<LayoutContainer> child)
{
    if(child->children().empty())
        return nullptr;
    return addChild(std::move(child));
}

void LayoutContainer::renderChildren(RenderState& state) const
{
    for(const auto& child : m_children) {
        child->render(state);
    }
}

LayoutClipPath::LayoutClipPath(Node* node)
    : LayoutContainer(node, LayoutId::ClipPath)
{
}

void LayoutClipPath::apply(RenderState& state) const
{
    RenderState newState(this, RenderMode::Clipping);
    newState.canvas = Canvas::create(state.canvas->rect());
    newState.transform = transform * state.transform;
    if(units == Units::ObjectBoundingBox) {
        const auto& box = state.objectBoundingBox();
        newState.transform.translate(box.x, box.y);
        newState.transform.scale(box.w, box.h);
    }

    renderChildren(newState);
    if(clipper) clipper->apply(newState);
    state.canvas->blend(newState.canvas.get(), BlendMode::Dst_In, 1.0);
}

LayoutMask::LayoutMask(Node* node)
    : LayoutContainer(node, LayoutId::Mask)
{
}

void LayoutMask::apply(RenderState& state) const
{
    Rect rect{x, y, width, height};
    if(units == Units::ObjectBoundingBox) {
        const auto& box = state.objectBoundingBox();
        rect.x = rect.x * box.w + box.x;
        rect.y = rect.y * box.h + box.y;
        rect.w = rect.w * box.w;
        rect.h = rect.h * box.h;
    }

    RenderState newState(this, state.mode());
    newState.canvas = Canvas::create(state.canvas->rect());
    newState.transform = state.transform;
    if(contentUnits == Units::ObjectBoundingBox) {
        const auto& box = state.objectBoundingBox();
        newState.transform.translate(box.x, box.y);
        newState.transform.scale(box.w, box.h);
    }

    renderChildren(newState);
    if(clipper) clipper->apply(newState);
    if(masker) masker->apply(newState);
    newState.canvas->mask(rect, state.transform);
    newState.canvas->luminance();
    state.canvas->blend(newState.canvas.get(), BlendMode::Dst_In, opacity);
}

LayoutSymbol::LayoutSymbol(Node* node)
    : LayoutContainer(node, LayoutId::Symbol)
{
}

void LayoutSymbol::render(RenderState& state) const
{
    BlendInfo info{clipper, masker, opacity, clip};
    RenderState newState(this, state.mode());
    newState.transform = transform * state.transform;
    newState.beginGroup(state, info);
    renderChildren(newState);
    newState.endGroup(state, info);
}

LayoutGroup::LayoutGroup(Node* node)
    : LayoutContainer(node, LayoutId::Group)
{
}

void LayoutGroup::render(RenderState& state) const
{
    BlendInfo info{clipper, masker, opacity, Rect::Invalid};
    RenderState newState(this, state.mode());
    newState.transform = transform * state.transform;
    newState.beginGroup(state, info);
    renderChildren(newState);
    newState.endGroup(state, info);
}

LayoutMarker::LayoutMarker(Node* node)
    : LayoutContainer(node, LayoutId::Marker)
{
}

Transform LayoutMarker::markerTransform(const Point& origin, double angle, double strokeWidth) const
{
    auto markerTransformation = Transform::translated(origin.x, origin.y);
    if(orient.type() == MarkerOrient::Auto)
        markerTransformation.rotate(angle);
    else
        markerTransformation.rotate(orient.value());

    if(units == MarkerUnits::StrokeWidth)
        markerTransformation.scale(strokeWidth, strokeWidth);

    markerTransformation.translate(-refX, -refY);
    return markerTransformation;
}

Rect LayoutMarker::markerBoundingBox(const Point& origin, double angle, double strokeWidth) const
{
    return markerTransform(origin, angle, strokeWidth).map(transform.map(strokeBoundingBox()));
}

void LayoutMarker::renderMarker(RenderState& state, const Point& origin, double angle, double strokeWidth) const
{
    BlendInfo info{clipper, masker, opacity, clip};
    RenderState newState(this, state.mode());
    newState.transform = transform * markerTransform(origin, angle, strokeWidth) * state.transform;
    newState.beginGroup(state, info);
    renderChildren(newState);
    newState.endGroup(state, info);
}

LayoutPattern::LayoutPattern(Node* node)
    : LayoutContainer(node, LayoutId::Pattern)
{
}

void LayoutPattern::apply(RenderState& state) const
{
    Rect rect{x, y, width, height};
    if(units == Units::ObjectBoundingBox) {
        const auto& box = state.objectBoundingBox();
        rect.x = rect.x * box.w + box.x;
        rect.y = rect.y * box.h + box.y;
        rect.w = rect.w * box.w;
        rect.h = rect.h * box.h;
    }

    auto ctm = state.transform * transform;
    auto scalex = std::sqrt(ctm.m00 * ctm.m00 + ctm.m01 * ctm.m01);
    auto scaley = std::sqrt(ctm.m10 * ctm.m10 + ctm.m11 * ctm.m11);

    RenderState newState(this, RenderMode::Display);
    newState.canvas = Canvas::create(0, 0, rect.w * scalex, rect.h * scaley);
    newState.transform = Transform::scaled(scalex, scaley);

    if(viewBox.valid()) {
        auto viewTransform = preserveAspectRatio.getMatrix(rect.w, rect.h, viewBox);
        newState.transform.premultiply(viewTransform);
    } else if(contentUnits == Units::ObjectBoundingBox) {
        const auto& box = state.objectBoundingBox();
        newState.transform.scale(box.w, box.h);
    }

    auto patternTransform = this->transform;
    patternTransform.translate(rect.x, rect.y);
    patternTransform.scale(1.0/scalex, 1.0/scaley);

    renderChildren(newState);
    state.canvas->setTexture(newState.canvas.get(), TextureType::Tiled, patternTransform);
}

LayoutGradient::LayoutGradient(Node* node, LayoutId id)
    : LayoutObject(node, id)
{
}

LayoutLinearGradient::LayoutLinearGradient(Node* node)
    : LayoutGradient(node, LayoutId::LinearGradient)
{
}

void LayoutLinearGradient::apply(RenderState& state) const
{
    auto gradientTransform = this->transform;
    if(units == Units::ObjectBoundingBox) {
        const auto& box = state.objectBoundingBox();
        gradientTransform *= Transform(box.w, 0, 0, box.h, box.x, box.y);
    }

    state.canvas->setLinearGradient(x1, y1, x2, y2, stops, spreadMethod, gradientTransform);
}

LayoutRadialGradient::LayoutRadialGradient(Node* node)
    : LayoutGradient(node, LayoutId::RadialGradient)
{
}

void LayoutRadialGradient::apply(RenderState& state) const
{
    auto gradientTransform = this->transform;
    if(units == Units::ObjectBoundingBox) {
        const auto& box = state.objectBoundingBox();
        gradientTransform *= Transform(box.w, 0, 0, box.h, box.x, box.y);
    }

    state.canvas->setRadialGradient(cx, cy, r, fx, fy, stops, spreadMethod, gradientTransform);
}

LayoutSolidColor::LayoutSolidColor(Node* node)
    : LayoutObject(node, LayoutId::SolidColor)
{
}

void LayoutSolidColor::apply(RenderState& state) const
{
    state.canvas->setColor(color);
}

void FillData::fill(RenderState& state, const Path& path) const
{
    if(opacity == 0.0 || (painter == nullptr && color.isNone()))
        return;

    if(painter == nullptr)
        state.canvas->setColor(color);
    else
        painter->apply(state);

    state.canvas->fill(path, state.transform, fillRule, BlendMode::Src_Over, opacity);
}

void StrokeData::stroke(RenderState& state, const Path& path) const
{
    if(opacity == 0.0 || (painter == nullptr && color.isNone()))
        return;

    if(painter == nullptr)
        state.canvas->setColor(color);
    else
        painter->apply(state);

    state.canvas->stroke(path, state.transform, width, cap, join, miterlimit, dash, BlendMode::Src_Over, opacity);
}

static const double sqrt2 = 1.41421356237309504880;

void StrokeData::inflate(Rect& box) const
{
    if(opacity == 0.0 || (painter == nullptr && color.isNone()))
        return;

    double caplimit = width / 2.0;
    if(cap == LineCap::Square)
        caplimit *= sqrt2;

    double joinlimit = width / 2.0;
    if(join == LineJoin::Miter)
        joinlimit *= miterlimit;

    double delta = std::max(caplimit, joinlimit);
    box.x -= delta;
    box.y -= delta;
    box.w += delta * 2.0;
    box.h += delta * 2.0;
}

MarkerPosition::MarkerPosition(const LayoutMarker* marker, const Point& origin, double angle)
    : marker(marker), origin(origin), angle(angle)
{
}

void MarkerData::add(const LayoutMarker* marker, const Point& origin, double angle)
{
    positions.emplace_back(marker, origin, angle);
}

void MarkerData::render(RenderState& state) const
{
    for(const auto& position : positions) {
        position.marker->renderMarker(state, position.origin, position.angle, strokeWidth);
    }
}

void MarkerData::inflate(Rect& box) const
{
    for(const auto& position : positions) {
        box.unite(position.marker->markerBoundingBox(position.origin, position.angle, strokeWidth));
    }
}

LayoutShape::LayoutShape(Node* node)
    : LayoutObject(node, LayoutId::Shape)
{
}

void LayoutShape::render(RenderState& state) const
{
    if(visibility == Visibility::Hidden)
        return;

    BlendInfo info{clipper, masker, opacity, Rect::Invalid};
    RenderState newState(this, state.mode());
    newState.transform = transform * state.transform;
    newState.beginGroup(state, info);

    if(newState.mode() == RenderMode::Display) {
        fillData.fill(newState, path);
        strokeData.stroke(newState, path);
        markerData.render(newState);
    } else {
        newState.canvas->setColor(Color::Black);
        newState.canvas->fill(path, newState.transform, clipRule, BlendMode::Src, 1.0);
    }

    newState.endGroup(state, info);
}

const Rect& LayoutShape::fillBoundingBox() const
{
    if(m_fillBoundingBox.valid())
        return m_fillBoundingBox;

    m_fillBoundingBox = path.box();
    return m_fillBoundingBox;
}

const Rect& LayoutShape::strokeBoundingBox() const
{
    if(m_strokeBoundingBox.valid())
        return m_strokeBoundingBox;

    m_strokeBoundingBox = fillBoundingBox();
    strokeData.inflate(m_strokeBoundingBox);
    markerData.inflate(m_strokeBoundingBox);
    return m_strokeBoundingBox;
}

RenderState::RenderState(const LayoutObject* object, RenderMode mode)
    : m_object(object), m_mode(mode)
{
}

void RenderState::beginGroup(RenderState& state, const BlendInfo& info)
{
    if(!info.clipper && !info.clip.valid()
        && (m_mode == RenderMode::Display && !(info.masker || info.opacity < 1.0))) {
        canvas = state.canvas;
        return;
    }

    auto box = transform.map(m_object->strokeBoundingBox());
    box.intersect(transform.map(info.clip));
    box.intersect(state.canvas->rect());
    canvas = Canvas::create(box);
}

void RenderState::endGroup(RenderState& state, const BlendInfo& info)
{
    if(state.canvas == canvas)
        return;

    if(info.clipper)
        info.clipper->apply(*this);

    if(info.masker && m_mode == RenderMode::Display)
        info.masker->apply(*this);

    if(info.clip.valid())
        canvas->mask(info.clip, transform);

    state.canvas->blend(canvas.get(), BlendMode::Src_Over, m_mode == RenderMode::Display ? info.opacity : 1.0);
}

LayoutContext::LayoutContext(const Document* document, LayoutSymbol* root)
    : m_document(document), m_root(root)
{
}

Element* LayoutContext::getElementById(const std::string& id) const
{
    auto element = m_document->getElementById(id);
    if(element.isNull())
        return nullptr;
    return element.get();
}

LayoutObject* LayoutContext::getResourcesById(const std::string& id) const
{
    auto it = m_resourcesCache.find(id);
    if(it == m_resourcesCache.end())
        return nullptr;
    return it->second;
}

LayoutObject* LayoutContext::addToResourcesCache(const std::string& id, std::unique_ptr<LayoutObject> resources)
{
    if(resources == nullptr)
        return nullptr;

    m_resourcesCache.emplace(id, resources.get());
    return m_root->addChild(std::move(resources));
}

LayoutMask* LayoutContext::getMasker(const std::string& id)
{
    if(id.empty())
        return nullptr;

    auto ref = getResourcesById(id);
    if(ref && ref->id() == LayoutId::Mask)
        return static_cast<LayoutMask*>(ref);

    auto element = getElementById(id);
    if(element == nullptr || element->id() != ElementID::Mask)
        return nullptr;

    auto masker = static_cast<MaskElement*>(element)->getMasker(this);
    return static_cast<LayoutMask*>(addToResourcesCache(id, std::move(masker)));
}

LayoutClipPath* LayoutContext::getClipper(const std::string& id)
{
    if(id.empty())
        return nullptr;

    auto ref = getResourcesById(id);
    if(ref && ref->id() == LayoutId::ClipPath)
        return static_cast<LayoutClipPath*>(ref);

    auto element = getElementById(id);
    if(element == nullptr || element->id() != ElementID::ClipPath)
        return nullptr;

    auto clipper = static_cast<ClipPathElement*>(element)->getClipper(this);
    return static_cast<LayoutClipPath*>(addToResourcesCache(id, std::move(clipper)));
}

LayoutMarker* LayoutContext::getMarker(const std::string& id)
{
    if(id.empty())
        return nullptr;

    auto ref = getResourcesById(id);
    if(ref && ref->id() == LayoutId::Marker)
        return static_cast<LayoutMarker*>(ref);

    auto element = getElementById(id);
    if(element == nullptr || element->id() != ElementID::Marker)
        return nullptr;

    auto marker = static_cast<MarkerElement*>(element)->getMarker(this);
    return static_cast<LayoutMarker*>(addToResourcesCache(id, std::move(marker)));
}

LayoutObject* LayoutContext::getPainter(const std::string& id)
{
    if(id.empty())
        return nullptr;

    auto ref = getResourcesById(id);
    if(ref && ref->isPaint())
        return ref;

    auto element = getElementById(id);
    if(element == nullptr || !element->isPaint())
        return nullptr;

    auto painter = static_cast<PaintElement*>(element)->getPainter(this);
    return addToResourcesCache(id, std::move(painter));
}

FillData LayoutContext::fillData(const StyledElement* element)
{
    auto fill = element->fill();
    if(fill.isNone())
        return FillData{};

    FillData fillData;
    fillData.painter = getPainter(fill.ref());
    fillData.color = fill.color();
    fillData.opacity = element->fill_opacity();
    fillData.fillRule = element->fill_rule();
    return fillData;
}

DashData LayoutContext::dashData(const StyledElement* element)
{
    auto dasharray = element->stroke_dasharray();
    if(dasharray.empty())
        return DashData{};

    LengthContext lengthContex(element);
    DashArray dashes;
    for(auto& dash : dasharray) {
        auto value = lengthContex.valueForLength(dash, LengthMode::Both);
        dashes.push_back(value);
    }

    auto num_dash = dashes.size();
    if(num_dash % 2)
        num_dash *= 2;

    DashData dashData;
    dashData.array.resize(num_dash);
    double sum = 0.0;
    for(std::size_t i = 0; i < num_dash; i++) {
        dashData.array[i] = dashes[i % dashes.size()];
        sum += dashData.array[i];
    }

    if(sum == 0.0)
        return DashData{};

    auto offset = lengthContex.valueForLength(element->stroke_dashoffset(), LengthMode::Both);
    dashData.offset = std::fmod(offset, sum);
    if(dashData.offset < 0.0)
        dashData.offset += sum;
    return dashData;
}

StrokeData LayoutContext::strokeData(const StyledElement* element)
{
    auto stroke = element->stroke();
    if(stroke.isNone())
        return StrokeData{};

    LengthContext lengthContex(element);
    StrokeData strokeData;
    strokeData.painter = getPainter(stroke.ref());
    strokeData.color = stroke.color();
    strokeData.opacity = element->stroke_opacity();
    strokeData.width = lengthContex.valueForLength(element->stroke_width(), LengthMode::Both);
    strokeData.miterlimit = element->stroke_miterlimit();
    strokeData.cap = element->stroke_linecap();
    strokeData.join = element->stroke_linejoin();
    strokeData.dash = dashData(element);
    return strokeData;
}

static const double pi = 3.14159265358979323846;

MarkerData LayoutContext::markerData(const GeometryElement* element, const Path& path)
{
    auto markerStart = getMarker(element->marker_start());
    auto markerMid = getMarker(element->marker_mid());
    auto markerEnd = getMarker(element->marker_end());

    if(markerStart == nullptr && markerMid == nullptr && markerEnd == nullptr)
        return MarkerData{};

    LengthContext lengthContex(element);
    MarkerData markerData;
    markerData.strokeWidth = lengthContex.valueForLength(element->stroke_width(), LengthMode::Both);

    PathIterator it(path);
    Point origin;
    Point startPoint;
    Point inslopePoints[2];
    Point outslopePoints[2];

    int index = 0;
    std::array<Point, 3> points;
    while(!it.isDone()) {
        switch(it.currentSegment(points)) {
        case PathCommand::MoveTo:
            startPoint = points[0];
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case PathCommand::LineTo:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case PathCommand::CubicTo:
            inslopePoints[0] = points[1];
            inslopePoints[1] = points[2];
            origin = points[2];
            break;
        case PathCommand::Close:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = startPoint;
            startPoint = Point{};
            break;
        }

        index += 1;
        it.next();

        if(!it.isDone() && (markerStart || markerMid)) {
            it.currentSegment(points);
            outslopePoints[0] = origin;
            outslopePoints[1] = points[0];

            if(index == 1 && markerStart) {
                Point slope{outslopePoints[1].x - outslopePoints[0].x, outslopePoints[1].y - outslopePoints[0].y};
                auto angle = 180.0 * std::atan2(slope.y, slope.x) / pi;

                markerData.add(markerStart, origin, angle);
            }

            if(index > 1 && markerMid) {
                Point inslope{inslopePoints[1].x - inslopePoints[0].x, inslopePoints[1].y - inslopePoints[0].y};
                Point outslope{outslopePoints[1].x - outslopePoints[0].x, outslopePoints[1].y - outslopePoints[0].y};
                auto inangle = 180.0 * std::atan2(inslope.y, inslope.x) / pi;
                auto outangle = 180.0 * std::atan2(outslope.y, outslope.x) / pi;
                auto angle = (inangle + outangle) * 0.5;

                markerData.add(markerMid, origin, angle);
            }
        }

        if(it.isDone() && markerEnd) {
            Point slope{inslopePoints[1].x - inslopePoints[0].x, inslopePoints[1].y - inslopePoints[0].y};
            auto angle = 180.0 * std::atan2(slope.y, slope.x) / pi;

            markerData.add(markerEnd, origin, angle);
        }
    }

    return markerData;
}

void LayoutContext::addReference(const Element* element)
{
    m_references.insert(element);
}

void LayoutContext::removeReference(const Element* element)
{
    m_references.erase(element);
}

bool LayoutContext::hasReference(const Element* element) const
{
    return m_references.count(element);
}

LayoutBreaker::LayoutBreaker(LayoutContext* context, const Element* element)
    : m_context(context), m_element(element)
{
    context->addReference(element);
}

LayoutBreaker::~LayoutBreaker()
{
    m_context->removeReference(m_element);
}

} // namespace lunasvg
