#include "lunasvg.h"
#include "layoutcontext.h"
#include "parser.h"
#include "svgelement.h"

#include <fstream>
#include <cstring>
#include <cmath>

namespace lunasvg {

Box::Box(double x, double y, double w, double h)
    : x(x), y(y), w(w), h(h)
{
}

Box::Box(const Rect& rect)
    : x(rect.x), y(rect.y), w(rect.w), h(rect.h)
{
}

Box& Box::transform(const Matrix &matrix)
{
    *this = transformed(matrix);
    return *this;
}

Box Box::transformed(const Matrix& matrix) const
{
    return Transform(matrix).map(*this);
}

Matrix::Matrix(double a, double b, double c, double d, double e, double f)
    : a(a), b(b), c(c), d(d), e(e), f(f)
{
}

Matrix::Matrix(const Transform& transform)
    : a(transform.m00), b(transform.m10), c(transform.m01), d(transform.m11), e(transform.m02), f(transform.m12)
{
}

Matrix& Matrix::rotate(double angle)
{
    *this = rotated(angle) * *this;
    return *this;
}

Matrix& Matrix::rotate(double angle, double cx, double cy)
{
    *this = rotated(angle, cx, cy) * *this;
    return *this;
}

Matrix& Matrix::scale(double sx, double sy)
{
    *this = scaled(sx, sy) * *this;
    return *this;
}

Matrix& Matrix::shear(double shx, double shy)
{
    *this = sheared(shx, shy) * *this;
    return *this;
}

Matrix& Matrix::translate(double tx, double ty)
{
   *this = translated(tx, ty) * *this;
    return *this;
}

Matrix& Matrix::transform(double _a, double _b, double _c, double _d, double _e, double _f)
{
    *this = Matrix{_a, _b, _c, _d, _e, _f} * *this;
    return *this;
}

Matrix& Matrix::identity()
{
    *this = Matrix{1, 0, 0, 1, 0, 0};
    return *this;
}

Matrix& Matrix::invert()
{
    *this = inverted();
    return *this;
}

Matrix& Matrix::operator*=(const Matrix& matrix)
{
    *this = *this * matrix;
    return *this; 
}

Matrix& Matrix::premultiply(const Matrix& matrix)
{
    *this = matrix * *this;
    return *this; 
}

Matrix& Matrix::postmultiply(const Matrix& matrix)
{
    *this = *this * matrix;
    return *this; 
}

Matrix Matrix::inverted() const
{
    return Transform(*this).inverted();
}

Matrix Matrix::operator*(const Matrix& matrix) const
{
    return Transform(*this) * Transform(matrix);
}

Matrix Matrix::rotated(double angle)
{
    return Transform::rotated(angle);
}

Matrix Matrix::rotated(double angle, double cx, double cy)
{
    return Transform::rotated(angle, cx, cy);
}

Matrix Matrix::scaled(double sx, double sy)
{
    return Transform::scaled(sx, sy);
}

Matrix Matrix::sheared(double shx, double shy)
{
    return Transform::sheared(shx, shy);
}

Matrix Matrix::translated(double tx, double ty)
{
    return Transform::translated(tx, ty);
}

struct Bitmap::Impl {
    Impl(std::uint8_t* data, std::uint32_t width, std::uint32_t height, std::uint32_t stride);
    Impl(std::uint32_t width, std::uint32_t height);

    std::unique_ptr<std::uint8_t[]> ownData;
    std::uint8_t* data;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t stride;
};

Bitmap::Impl::Impl(std::uint8_t* data, std::uint32_t width, std::uint32_t height, std::uint32_t stride)
    : data(data), width(width), height(height), stride(stride)
{
}

Bitmap::Impl::Impl(std::uint32_t width, std::uint32_t height)
    : ownData(new std::uint8_t[width*height*4]), data(nullptr), width(width), height(height), stride(width * 4)
{
}

Bitmap::Bitmap()
{
}

Bitmap::Bitmap(std::uint8_t* data, std::uint32_t width, std::uint32_t height, std::uint32_t stride)
    : m_impl(new Impl(data, width, height, stride))
{
}

Bitmap::Bitmap(std::uint32_t width, std::uint32_t height)
    : m_impl(new Impl(width, height))
{
}

void Bitmap::reset(std::uint8_t* data, std::uint32_t width, std::uint32_t height, std::uint32_t stride)
{
    m_impl.reset(new Impl(data, width, height, stride));
}

void Bitmap::reset(std::uint32_t width, std::uint32_t height)
{
    m_impl.reset(new Impl(width, height));
}

std::uint8_t* Bitmap::data() const
{
    if(m_impl == nullptr)
        return nullptr;
    if(m_impl->data == nullptr)
        return m_impl->ownData.get();
    return m_impl->data;
}

std::uint32_t Bitmap::width() const
{
    return m_impl ? m_impl->width : 0;
}

std::uint32_t Bitmap::height() const
{
    return m_impl ? m_impl->height : 0;
}

std::uint32_t Bitmap::stride() const
{
    return m_impl ? m_impl->stride : 0;
}

void Bitmap::clear(std::uint32_t color)
{
    auto r = (color >> 24) & 0xFF;
    auto g = (color >> 16) & 0xFF;
    auto b = (color >> 8) & 0xFF;
    auto a = (color >> 0) & 0xFF;

    auto pr = (r * a) / 255;
    auto pg = (g * a) / 255;
    auto pb = (b * a) / 255;

    auto width = this->width();
    auto height = this->height();
    auto stride = this->stride();
    auto rowData = this->data();

    for(std::uint32_t y = 0; y < height; y++) {
        auto data = rowData;
        for(std::uint32_t x = 0; x < width; x++) {
            data[0] = static_cast<std::uint8_t>(pb);
            data[1] = static_cast<std::uint8_t>(pg);
            data[2] = static_cast<std::uint8_t>(pr);
            data[3] = static_cast<std::uint8_t>(a);
            data += 4;
        }

        rowData += stride;
    }
}

void Bitmap::convert(int ri, int gi, int bi, int ai, bool unpremultiply)
{
    auto width = this->width();
    auto height = this->height();
    auto stride = this->stride();
    auto rowData = this->data();

    for(std::uint32_t y = 0; y < height; y++) {
        auto data = rowData;
        for(std::uint32_t x = 0; x < width; x++) {
            auto b = data[0];
            auto g = data[1];
            auto r = data[2];
            auto a = data[3];

            if(unpremultiply && a != 0) {
                r = (r * 255) / a;
                g = (g * 255) / a;
                b = (b * 255) / a;
            }

            data[ri] = r;
            data[gi] = g;
            data[bi] = b;
            data[ai] = a;
            data += 4;
        }

        rowData += stride;
    }
}

DomElement::DomElement(Element* element)
    : m_element(element)
{
}

void DomElement::setAttribute(const std::string& name, const std::string& value)
{
    if(m_element) {
        auto id = propertyid(name);
        if(id != PropertyID::Unknown) {
            m_element->set(id, value, 0x1000);
        }
    }
}

std::string DomElement::getAttribute(const std::string& name) const
{
    if(m_element) {
        auto id = propertyid(name);
        if(id != PropertyID::Unknown) {
            return m_element->get(id);
        }
    }

    return std::string();
}

void DomElement::removeAttribute(const std::string& name)
{
    setAttribute(name, std::string());
}

bool DomElement::hasAttribute(const std::string& name) const
{
    if(m_element) {
        auto id = propertyid(name);
        if(id != PropertyID::Unknown) {
            return m_element->has(id);
        }
    }

    return false;
}

Box DomElement::getBBox() const
{
    if(m_element && m_element->box())
        return m_element->box()->strokeBoundingBox();
    return Box();
}

Matrix DomElement::getLocalTransform() const
{
    if(m_element && m_element->box())
        return m_element->box()->localTransform();
    return Matrix();
}

Matrix DomElement::getAbsoluteTransform() const
{
    if(m_element == nullptr || !m_element->box())
        return Matrix();
    auto transform = m_element->box()->localTransform();
    for(auto currentElement = m_element->parent(); currentElement; currentElement = currentElement->parent()) {
        if(auto box = currentElement->box()) {
            transform.postmultiply(box->localTransform());
        }
    }

    return transform;
}

void DomElement::render(Bitmap bitmap, const Matrix& matrix) const
{
    if(m_element == nullptr || !m_element->box())
        return;
    RenderState state(nullptr, RenderMode::Display);
    state.canvas = Canvas::create(bitmap.data(), bitmap.width(), bitmap.height(), bitmap.stride());
    state.transform = Transform(matrix);
    m_element->box()->render(state);
}

Bitmap DomElement::renderToBitmap(std::uint32_t width, std::uint32_t height, std::uint32_t backgroundColor) const
{
    if(m_element == nullptr || !m_element->box())
        return Bitmap();
    auto elementBounds = m_element->box()->map(m_element->box()->strokeBoundingBox());
    if(elementBounds.empty())
        return Bitmap();
    if(width == 0 && height == 0) {
        width = static_cast<std::uint32_t>(std::ceil(elementBounds.w));
        height = static_cast<std::uint32_t>(std::ceil(elementBounds.h));
    } else if(width != 0 && height == 0) {
        height = static_cast<std::uint32_t>(std::ceil(width * elementBounds.h / elementBounds.w));
    } else if(height != 0 && width == 0) {
        width = static_cast<std::uint32_t>(std::ceil(height * elementBounds.w / elementBounds.h));
    }

    const auto xScale = width / elementBounds.w;
    const auto yScale = height / elementBounds.h;

    Matrix matrix(xScale, 0, 0, yScale, -elementBounds.x * xScale, -elementBounds.y * yScale);
    Bitmap bitmap(width, height);
    bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

std::unique_ptr<Document> Document::loadFromFile(const std::string& filename)
{
    std::ifstream fs;
    fs.open(filename);
    if(!fs.is_open())
        return nullptr;

    std::string content;
    std::getline(fs, content, '\0');
    fs.close();

    return loadFromData(content);
}

std::unique_ptr<Document> Document::loadFromData(const std::string& string)
{
    return loadFromData(string.data(), string.size());
}

std::unique_ptr<Document> Document::loadFromData(const char* data, std::size_t size)
{
    std::unique_ptr<Document> document(new Document);
    if(!document->parse(data, size))
        return nullptr;
    document->updateLayout();
    return document;
}

std::unique_ptr<Document> Document::loadFromData(const char* data)
{
    return loadFromData(data, std::strlen(data));
}

void Document::setMatrix(const Matrix& matrix)
{
    if(m_rootBox) {
        Box bbox(0, 0, m_rootBox->width, m_rootBox->height);
        bbox.transform(matrix);
        m_rootBox->width = bbox.w;
        m_rootBox->height = bbox.h;
        m_rootBox->transform = Transform(matrix);
    }
}

Matrix Document::matrix() const
{
    if(m_rootBox == nullptr)
        return Matrix();
    return m_rootBox->transform;
}

Box Document::box() const
{
    if(m_rootBox == nullptr)
        return Box();
    return m_rootBox->map(m_rootBox->strokeBoundingBox());
}

double Document::width() const
{
    if(m_rootBox == nullptr)
        return 0.0;
    return m_rootBox->width;
}

double Document::height() const
{
    if(m_rootBox == nullptr)
        return 0.0;
    return m_rootBox->height;
}

void Document::render(Bitmap bitmap, const Matrix& matrix) const
{
    if(m_rootBox == nullptr)
        return;
    RenderState state(nullptr, RenderMode::Display);
    state.canvas = Canvas::create(bitmap.data(), bitmap.width(), bitmap.height(), bitmap.stride());
    state.transform = Transform(matrix);
    m_rootBox->render(state);
}

Bitmap Document::renderToBitmap(std::uint32_t width, std::uint32_t height, std::uint32_t backgroundColor) const
{
    if(m_rootBox == nullptr || m_rootBox->width == 0.0 || m_rootBox->height == 0.0)
        return Bitmap();
    if(width == 0 && height == 0) {
        width = static_cast<std::uint32_t>(std::ceil(m_rootBox->width));
        height = static_cast<std::uint32_t>(std::ceil(m_rootBox->height));
    } else if(width != 0 && height == 0) {
        height = static_cast<std::uint32_t>(std::ceil(width * m_rootBox->height / m_rootBox->width));
    } else if(height != 0 && width == 0) {
        width = static_cast<std::uint32_t>(std::ceil(height * m_rootBox->width / m_rootBox->height));
    }

    Matrix matrix(width / m_rootBox->width, 0, 0, height / m_rootBox->height, 0, 0);
    Bitmap bitmap(width, height);
    bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

void Document::updateLayout()
{
    m_rootBox = m_rootElement->layoutTree(this);
}

DomElement Document::getElementById(const std::string& id) const
{
    auto it = m_idCache.find(id);
    if(it == m_idCache.end())
        return nullptr;
    return it->second;
}

DomElement Document::rootElement() const
{
    return m_rootElement.get();
}

Document::Document(Document&&) = default;
Document::~Document() = default;
Document::Document() = default;

} // namespace lunasvg
