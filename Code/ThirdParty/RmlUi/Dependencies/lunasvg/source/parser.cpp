#include "parser.h"
#include "parserutils.h"
#include "layoutcontext.h"
#include "svgelement.h"

namespace lunasvg {

Length Parser::parseLength(const std::string& string, LengthNegativeValuesMode mode, const Length& defaultValue)
{
    if(string.empty())
        return defaultValue;

    auto ptr = string.data();
    auto end = ptr + string.size();

    double value;
    LengthUnits units;
    if(!parseLength(ptr, end, value, units, mode))
        return defaultValue;

    return Length{value, units};
}

LengthList Parser::parseLengthList(const std::string& string, LengthNegativeValuesMode mode)
{
    if(string.empty())
        return LengthList{};

    auto ptr = string.data();
    auto end = ptr + string.size();

    double value;
    LengthUnits units;

    LengthList values;
    while(ptr < end) {
        if(!parseLength(ptr, end, value, units, mode))
            break;
        values.emplace_back(value, units);
        Utils::skipWsComma(ptr, end);
    }

    return values;
}

double Parser::parseNumber(const std::string& string, double defaultValue)
{
    if(string.empty())
        return defaultValue;

    auto ptr = string.data();
    auto end = ptr + string.size();

    double value;
    if(!Utils::parseNumber(ptr, end, value))
        return defaultValue;

    return value;
}

double Parser::parseNumberPercentage(const std::string& string, double defaultValue)
{
    if(string.empty())
        return defaultValue;

    auto ptr = string.data();
    auto end = ptr + string.size();

    double value;
    if(!Utils::parseNumber(ptr, end, value))
        return defaultValue;

    if(Utils::skipDesc(ptr, end, '%'))
        value /= 100.0;
    return value < 0.0 ? 0.0 : value > 1.0 ? 1.0 : value;
}

PointList Parser::parsePointList(const std::string& string)
{
    if(string.empty())
        return PointList{};

    auto ptr = string.data();
    auto end = ptr + string.size();

    double x;
    double y;

    PointList values;
    while(ptr < end) {
        if(!Utils::parseNumber(ptr, end, x)
            || !Utils::skipWsComma(ptr, end)
            || !Utils::parseNumber(ptr, end, y)) {
            break;
        }

        values.emplace_back(x, y);
        Utils::skipWsComma(ptr, end);
    }

    return values;
}

Transform Parser::parseTransform(const std::string& string)
{
    if(string.empty())
        return Transform{};

    auto ptr = string.data();
    auto end = ptr + string.size();

    TransformType type;
    double values[6];
    int count;

    Transform transform;
    while(ptr < end) {
        if(!parseTransform(ptr, end, type, values, count))
            break;
        Utils::skipWsComma(ptr, end);
        switch(type) {
        case TransformType::Matrix:
            transform.transform(values[0], values[1], values[2], values[3], values[4], values[5]);
            break;
        case TransformType::Rotate:
            if(count == 1)
                transform.rotate(values[0], 0, 0);
            else
                transform.rotate(values[0], values[1], values[2]);
            break;
        case TransformType::Scale:
            if(count == 1)
                transform.scale(values[0], values[0]);
            else
                transform.scale(values[0], values[1]);
            break;
        case TransformType::SkewX:
            transform.shear(values[0], 0);
            break;
        case TransformType::SkewY:
            transform.shear(0, values[0]);
            break;
        case TransformType::Translate:
            if(count == 1)
                transform.translate(values[0], 0);
            else
                transform.translate(values[0], values[1]);
            break;
        }
    }

    return transform;
}

Path Parser::parsePath(const std::string& string)
{
    auto ptr = string.data();
    auto end = ptr + string.size();
    if(ptr >= end || !(*ptr == 'M' || *ptr == 'm'))
        return Path{};

    auto command = *ptr++;
    auto lastCommand = command;
    double c[6];
    bool f[2];

    Point startPoint;
    Point currentPoint;
    Point controlPoint;

    Path path;
    while(true) {
        Utils::skipWs(ptr, end);
        if(command == 'M' || command == 'm') {
            if(!parseNumberList(ptr, end, c, 2))
                return path;

            if(command == 'm') {
                c[0] += currentPoint.x;
                c[1] += currentPoint.y;
            }

            path.moveTo(c[0], c[1]);
            startPoint.x = currentPoint.x = c[0];
            startPoint.y = currentPoint.y = c[1];
            command = command == 'm' ? 'l' : 'L';
        } else if(command == 'L' || command == 'l') {
            if(!parseNumberList(ptr, end, c, 2))
                return path;

            if(command == 'l') {
                c[0] += currentPoint.x;
                c[1] += currentPoint.y;
            }

            path.lineTo(c[0], c[1]);
            currentPoint.x = c[0];
            currentPoint.y = c[1];
        } else if(command == 'H' || command == 'h') {
            if(!parseNumberList(ptr, end, c, 1))
                return path;

            if(command == 'h')
               c[0] += currentPoint.x;

            path.lineTo(c[0], currentPoint.y);
            currentPoint.x = c[0];
        } else if(command == 'V' || command == 'v') {
            if(!parseNumberList(ptr, end, c + 1, 1))
                return path;

            if(command == 'v')
               c[1] += currentPoint.y;

            path.lineTo(currentPoint.x, c[1]);
            currentPoint.y = c[1];
        } else if(command == 'Q' || command == 'q') {
            if(!parseNumberList(ptr, end, c, 4))
                return path;

            if(command == 'q') {
                c[0] += currentPoint.x;
                c[1] += currentPoint.y;
                c[2] += currentPoint.x;
                c[3] += currentPoint.y;
            }

            path.quadTo(currentPoint.x, currentPoint.y, c[0], c[1], c[2], c[3]);
            controlPoint.x = c[0];
            controlPoint.y = c[1];
            currentPoint.x = c[2];
            currentPoint.y = c[3];
        } else if(command == 'C' || command == 'c') {
            if(!parseNumberList(ptr, end, c, 6))
                return path;

            if(command == 'c') {
                c[0] += currentPoint.x;
                c[1] += currentPoint.y;
                c[2] += currentPoint.x;
                c[3] += currentPoint.y;
                c[4] += currentPoint.x;
                c[5] += currentPoint.y;
            }

            path.cubicTo(c[0], c[1], c[2], c[3], c[4], c[5]);
            controlPoint.x = c[2];
            controlPoint.y = c[3];
            currentPoint.x = c[4];
            currentPoint.y = c[5];
        } else if(command == 'T' || command == 't') {
            if(lastCommand != 'Q' && lastCommand != 'q' && lastCommand != 'T' && lastCommand != 't') {
                c[0] = currentPoint.x;
                c[1] = currentPoint.y;
            } else {
                c[0] = 2 * currentPoint.x - controlPoint.x;
                c[1] = 2 * currentPoint.y - controlPoint.y;
            }

            if(!parseNumberList(ptr, end, c + 2, 2))
                return path;

            if(command == 't') {
                c[2] += currentPoint.x;
                c[3] += currentPoint.y;
            }

            path.quadTo(currentPoint.x, currentPoint.y, c[0], c[1], c[2], c[3]);
            controlPoint.x = c[0];
            controlPoint.y = c[1];
            currentPoint.x = c[2];
            currentPoint.y = c[3];
        } else if(command == 'S' || command == 's') {
            if(lastCommand != 'C' && lastCommand != 'c' && lastCommand != 'S' && lastCommand != 's') {
                c[0] = currentPoint.x;
                c[1] = currentPoint.y;
            } else {
                c[0] = 2 * currentPoint.x - controlPoint.x;
                c[1] = 2 * currentPoint.y - controlPoint.y;
            }

            if(!parseNumberList(ptr, end, c + 2, 4))
                return path;

            if(command == 's') {
                c[2] += currentPoint.x;
                c[3] += currentPoint.y;
                c[4] += currentPoint.x;
                c[5] += currentPoint.y;
            }

            path.cubicTo(c[0], c[1], c[2], c[3], c[4], c[5]);
            controlPoint.x = c[2];
            controlPoint.y = c[3];
            currentPoint.x = c[4];
            currentPoint.y = c[5];
        } else if(command == 'A' || command == 'a') {
            if(!parseNumberList(ptr, end, c, 3)
                || !parseArcFlag(ptr, end, f[0])
                || !parseArcFlag(ptr, end, f[1])
                || !parseNumberList(ptr, end, c + 3, 2)) {
                return path;
            }

            if(command == 'a') {
               c[3] += currentPoint.x;
               c[4] += currentPoint.y;
            }

            path.arcTo(currentPoint.x, currentPoint.y, c[0], c[1], c[2], f[0], f[1], c[3], c[4]);
            currentPoint.x = c[3];
            currentPoint.y = c[4];
        } else if(command == 'Z' || command == 'z') {
            if(lastCommand == 'Z' || lastCommand == 'z')
               return path;
            path.close();
            currentPoint.x = startPoint.x;
            currentPoint.y = startPoint.y;
        } else {
            return path;
        }

        Utils::skipWsComma(ptr, end);
        if(ptr >= end)
            break;

        lastCommand = command;
        if(IS_ALPHA(*ptr)) {
            command = *ptr++;
        }
    }

    return path;
}

std::string Parser::parseUrl(const std::string& string)
{
    if(string.empty())
        return std::string{};

    auto ptr = string.data();
    auto end = ptr + string.size();

    std::string value;
    parseUrlFragment(ptr, end, value);
    return value;
}

std::string Parser::parseHref(const std::string& string)
{
    if(string.size() > 1 && string.front() == '#')
        return string.substr(1);
    return std::string{};
}

Rect Parser::parseViewBox(const std::string& string)
{
    if(string.empty())
        return Rect::Invalid;

    auto ptr = string.data();
    auto end = ptr + string.size();

    double x;
    double y;
    double w;
    double h;
    if(!Utils::parseNumber(ptr, end, x)
        || !Utils::skipWsComma(ptr, end)
        || !Utils::parseNumber(ptr, end, y)
        || !Utils::skipWsComma(ptr, end)
        || !Utils::parseNumber(ptr, end, w)
        || !Utils::skipWsComma(ptr, end)
        || !Utils::parseNumber(ptr, end, h)) {
        return Rect::Invalid;
    }

    if(w < 0.0 || h < 0.0)
        return Rect::Invalid;
    return Rect{x, y, w, h};
}

PreserveAspectRatio Parser::parsePreserveAspectRatio(const std::string& string)
{
    if(string.empty())
        return PreserveAspectRatio{};

    auto ptr = string.data();
    auto end = ptr + string.size();

    Align align{Align::xMidYMid};
    MeetOrSlice scale{MeetOrSlice::Meet};
    if(Utils::skipDesc(ptr, end, "none"))
        align = Align::None;
    else if(Utils::skipDesc(ptr, end, "xMinYMin"))
        align = Align::xMinYMin;
    else if(Utils::skipDesc(ptr, end, "xMidYMin"))
        align = Align::xMidYMin;
    else if(Utils::skipDesc(ptr, end, "xMaxYMin"))
        align = Align::xMaxYMin;
    else if(Utils::skipDesc(ptr, end, "xMinYMid"))
        align = Align::xMinYMid;
    else if(Utils::skipDesc(ptr, end, "xMidYMid"))
        align = Align::xMidYMid;
    else if(Utils::skipDesc(ptr, end, "xMaxYMid"))
        align = Align::xMaxYMid;
    else if(Utils::skipDesc(ptr, end, "xMinYMax"))
        align = Align::xMinYMax;
    else if(Utils::skipDesc(ptr, end, "xMidYMax"))
        align = Align::xMidYMax;
    else if(Utils::skipDesc(ptr, end, "xMaxYMax"))
        align = Align::xMaxYMax;
    else
        return PreserveAspectRatio{};

    Utils::skipWs(ptr, end);
    if(Utils::skipDesc(ptr, end, "slice"))
        scale = MeetOrSlice::Slice;
    else
        scale = MeetOrSlice::Meet;

    return PreserveAspectRatio{align, scale};
}

static const double pi = 3.14159265358979323846;

Angle Parser::parseAngle(const std::string& string)
{
    if(string.empty())
        return Angle{};

    auto ptr = string.data();
    auto end = ptr + string.size();

    if(Utils::skipDesc(ptr, end, "auto"))
        return MarkerOrient::Auto;

    double value;
    if(!Utils::parseNumber(ptr, end, value))
        return Angle{};

    if(Utils::skipDesc(ptr, end, "rad"))
        value *= 180.0 / pi;
    else if(Utils::skipDesc(ptr, end, "grad"))
        value *= 360.0 / 400.0;

    return Angle{value, MarkerOrient::Angle};
}

MarkerUnits Parser::parseMarkerUnits(const std::string& string)
{
    if(string.empty())
        return MarkerUnits::StrokeWidth;

    if(string.compare("userSpaceOnUse") == 0)
        return MarkerUnits::UserSpaceOnUse;
    return MarkerUnits::StrokeWidth;
}

SpreadMethod Parser::parseSpreadMethod(const std::string& string)
{
    if(string.empty())
        return SpreadMethod::Pad;

    if(string.compare("repeat") == 0)
        return SpreadMethod::Repeat;
    if(string.compare("reflect") == 0)
        return SpreadMethod::Reflect;
    return SpreadMethod::Pad;
}

Units Parser::parseUnits(const std::string& string, Units defaultValue)
{
    if(string.empty())
        return defaultValue;

    if(string.compare("userSpaceOnUse") == 0)
        return Units::UserSpaceOnUse;
    if(string.compare("objectBoundingBox") == 0)
        return Units::ObjectBoundingBox;
    return defaultValue;
}

Color Parser::parseColor(const std::string& string, const StyledElement* element, const Color& defaultValue)
{
    if(string.empty())
        return defaultValue;

    auto ptr = string.data();
    auto end = ptr + string.size();

    if(Utils::skipDesc(ptr, end, '#')) {
        auto start = ptr;
        unsigned int value;
        if(!Utils::parseInteger(ptr, end, value, 16))
            return defaultValue;

        auto n = ptr - start;
        if(n != 3 && n != 6)
            return defaultValue;

        if(n == 3) {
            value = ((value&0xf00) << 8) | ((value&0x0f0) << 4) | (value&0x00f);
            value |= value << 4;
        }

        return Color(value | 0xFF000000);
    }

    if(Utils::skipDesc(ptr, end, "rgb(")) {
        uint8_t r, g, b;
        if(!Utils::skipWs(ptr, end)
            || !parseColorComponent(ptr, end, r)
            || !Utils::skipWsComma(ptr, end)
            || !parseColorComponent(ptr, end, g)
            || !Utils::skipWsComma(ptr, end)
            || !parseColorComponent(ptr, end, b)
            || !Utils::skipWs(ptr, end)
            || !Utils::skipDesc(ptr, end, ')')) {
            return defaultValue;
        }

        return Color(r, g, b, 255);
    }

    if(Utils::skipDesc(ptr, end, "none"))
        return Color::Transparent;

    if(Utils::skipDesc(ptr, end, "currentColor"))
        return element->color();

    static const std::map<std::string, unsigned int> colormap = {
        {"aliceblue", 0xF0F8FF},
        {"antiquewhite", 0xFAEBD7},
        {"aqua", 0x00FFFF},
        {"aquamarine", 0x7FFFD4},
        {"azure", 0xF0FFFF},
        {"beige", 0xF5F5DC},
        {"bisque", 0xFFE4C4},
        {"black", 0x000000},
        {"blanchedalmond", 0xFFEBCD},
        {"blue", 0x0000FF},
        {"blueviolet", 0x8A2BE2},
        {"brown", 0xA52A2A},
        {"burlywood", 0xDEB887},
        {"cadetblue", 0x5F9EA0},
        {"chartreuse", 0x7FFF00},
        {"chocolate", 0xD2691E},
        {"coral", 0xFF7F50},
        {"cornflowerblue", 0x6495ED},
        {"cornsilk", 0xFFF8DC},
        {"crimson", 0xDC143C},
        {"cyan", 0x00FFFF},
        {"darkblue", 0x00008B},
        {"darkcyan", 0x008B8B},
        {"darkgoldenrod", 0xB8860B},
        {"darkgray", 0xA9A9A9},
        {"darkgreen", 0x006400},
        {"darkgrey", 0xA9A9A9},
        {"darkkhaki", 0xBDB76B},
        {"darkmagenta", 0x8B008B},
        {"darkolivegreen", 0x556B2F},
        {"darkorange", 0xFF8C00},
        {"darkorchid", 0x9932CC},
        {"darkred", 0x8B0000},
        {"darksalmon", 0xE9967A},
        {"darkseagreen", 0x8FBC8F},
        {"darkslateblue", 0x483D8B},
        {"darkslategray", 0x2F4F4F},
        {"darkslategrey", 0x2F4F4F},
        {"darkturquoise", 0x00CED1},
        {"darkviolet", 0x9400D3},
        {"deeppink", 0xFF1493},
        {"deepskyblue", 0x00BFFF},
        {"dimgray", 0x696969},
        {"dimgrey", 0x696969},
        {"dodgerblue", 0x1E90FF},
        {"firebrick", 0xB22222},
        {"floralwhite", 0xFFFAF0},
        {"forestgreen", 0x228B22},
        {"fuchsia", 0xFF00FF},
        {"gainsboro", 0xDCDCDC},
        {"ghostwhite", 0xF8F8FF},
        {"gold", 0xFFD700},
        {"goldenrod", 0xDAA520},
        {"gray", 0x808080},
        {"green", 0x008000},
        {"greenyellow", 0xADFF2F},
        {"grey", 0x808080},
        {"honeydew", 0xF0FFF0},
        {"hotpink", 0xFF69B4},
        {"indianred", 0xCD5C5C},
        {"indigo", 0x4B0082},
        {"ivory", 0xFFFFF0},
        {"khaki", 0xF0E68C},
        {"lavender", 0xE6E6FA},
        {"lavenderblush", 0xFFF0F5},
        {"lawngreen", 0x7CFC00},
        {"lemonchiffon", 0xFFFACD},
        {"lightblue", 0xADD8E6},
        {"lightcoral", 0xF08080},
        {"lightcyan", 0xE0FFFF},
        {"lightgoldenrodyellow", 0xFAFAD2},
        {"lightgray", 0xD3D3D3},
        {"lightgreen", 0x90EE90},
        {"lightgrey", 0xD3D3D3},
        {"lightpink", 0xFFB6C1},
        {"lightsalmon", 0xFFA07A},
        {"lightseagreen", 0x20B2AA},
        {"lightskyblue", 0x87CEFA},
        {"lightslategray", 0x778899},
        {"lightslategrey", 0x778899},
        {"lightsteelblue", 0xB0C4DE},
        {"lightyellow", 0xFFFFE0},
        {"lime", 0x00FF00},
        {"limegreen", 0x32CD32},
        {"linen", 0xFAF0E6},
        {"magenta", 0xFF00FF},
        {"maroon", 0x800000},
        {"mediumaquamarine", 0x66CDAA},
        {"mediumblue", 0x0000CD},
        {"mediumorchid", 0xBA55D3},
        {"mediumpurple", 0x9370DB},
        {"mediumseagreen", 0x3CB371},
        {"mediumslateblue", 0x7B68EE},
        {"mediumspringgreen", 0x00FA9A},
        {"mediumturquoise", 0x48D1CC},
        {"mediumvioletred", 0xC71585},
        {"midnightblue", 0x191970},
        {"mintcream", 0xF5FFFA},
        {"mistyrose", 0xFFE4E1},
        {"moccasin", 0xFFE4B5},
        {"navajowhite", 0xFFDEAD},
        {"navy", 0x000080},
        {"oldlace", 0xFDF5E6},
        {"olive", 0x808000},
        {"olivedrab", 0x6B8E23},
        {"orange", 0xFFA500},
        {"orangered", 0xFF4500},
        {"orchid", 0xDA70D6},
        {"palegoldenrod", 0xEEE8AA},
        {"palegreen", 0x98FB98},
        {"paleturquoise", 0xAFEEEE},
        {"palevioletred", 0xDB7093},
        {"papayawhip", 0xFFEFD5},
        {"peachpuff", 0xFFDAB9},
        {"peru", 0xCD853F},
        {"pink", 0xFFC0CB},
        {"plum", 0xDDA0DD},
        {"powderblue", 0xB0E0E6},
        {"purple", 0x800080},
        {"rebeccapurple", 0x663399},
        {"red", 0xFF0000},
        {"rosybrown", 0xBC8F8F},
        {"royalblue", 0x4169E1},
        {"saddlebrown", 0x8B4513},
        {"salmon", 0xFA8072},
        {"sandybrown", 0xF4A460},
        {"seagreen", 0x2E8B57},
        {"seashell", 0xFFF5EE},
        {"sienna", 0xA0522D},
        {"silver", 0xC0C0C0},
        {"skyblue", 0x87CEEB},
        {"slateblue", 0x6A5ACD},
        {"slategray", 0x708090},
        {"slategrey", 0x708090},
        {"snow", 0xFFFAFA},
        {"springgreen", 0x00FF7F},
        {"steelblue", 0x4682B4},
        {"tan", 0xD2B48C},
        {"teal", 0x008080},
        {"thistle", 0xD8BFD8},
        {"tomato", 0xFF6347},
        {"turquoise", 0x40E0D0},
        {"violet", 0xEE82EE},
        {"wheat", 0xF5DEB3},
        {"white", 0xFFFFFF},
        {"whitesmoke", 0xF5F5F5},
        {"yellow", 0xFFFF00},
        {"yellowgreen", 0x9ACD32}
    };

    auto it = colormap.find(string);
    if(it == colormap.end())
        return defaultValue;
    return Color(it->second | 0xFF000000);
}

Paint Parser::parsePaint(const std::string& string, const StyledElement* element, const Color& defaultValue)
{
    if(string.empty())
        return defaultValue;

    auto ptr = string.data();
    auto end = ptr + string.size();

    std::string ref;
    if(!parseUrlFragment(ptr, end, ref))
        return parseColor(string, element, defaultValue);

    std::string fallback{ptr, end};
    if(fallback.empty())
        return Paint{ref, Color::Transparent};
    return Paint{ref, parseColor(fallback, element, defaultValue)};
}

WindRule Parser::parseWindRule(const std::string& string)
{
    if(string.empty())
        return WindRule::NonZero;

    if(string.compare("evenodd") == 0)
        return WindRule::EvenOdd;
    return WindRule::NonZero;
}

LineCap Parser::parseLineCap(const std::string& string)
{
    if(string.empty())
        return LineCap::Butt;

    if(string.compare("round") == 0)
        return LineCap::Round;
    if(string.compare("square") == 0)
        return LineCap::Square;
    return LineCap::Butt;
}

LineJoin Parser::parseLineJoin(const std::string& string)
{
    if(string.empty())
        return LineJoin::Miter;

    if(string.compare("bevel") == 0)
        return LineJoin::Bevel;
    if(string.compare("round") == 0)
        return LineJoin::Round;
    return LineJoin::Miter;
}

Display Parser::parseDisplay(const std::string& string)
{
    if(string.empty())
        return Display::Inline;

    if(string.compare("none") == 0)
        return Display::None;
    return Display::Inline;
}

Visibility Parser::parseVisibility(const std::string& string)
{
    if(string.empty())
        return Visibility::Visible;

    if(string.compare("visible") == 0)
        return Visibility::Visible;
    return Visibility::Hidden;
}

Overflow Parser::parseOverflow(const std::string& string, Overflow defaultValue)
{
    if(string.empty())
        return defaultValue;

    if(string.compare("visible") == 0)
        return Overflow::Visible;
    if(string.compare("hidden") == 0)
        return Overflow::Hidden;
    return defaultValue;
}

bool Parser::parseLength(const char*& ptr, const char* end, double& value, LengthUnits& units, LengthNegativeValuesMode mode)
{
    if(!Utils::parseNumber(ptr, end, value))
        return false;

    if(mode == ForbidNegativeLengths && value < 0.0)
        return false;

    char c[2] = {0, 0};
    if(ptr + 0 < end) c[0] = ptr[0];
    if(ptr + 1 < end) c[1] = ptr[1];

    switch(c[0]) {
    case '%':
        units = LengthUnits::Percent;
        ptr += 1;
        break;
    case 'p':
        if(c[1] == 'x')
            units = LengthUnits::Px;
        else if(c[1] == 'c')
            units = LengthUnits::Pc;
        else if(ptr[1] == 't')
            units = LengthUnits::Pt;
        else
            return false;
        ptr += 2;
        break;
    case 'i':
        if(c[1] == 'n')
            units = LengthUnits::In;
        else
            return false;
        ptr += 2;
        break;
    case 'c':
        if(c[1] == 'm')
            units = LengthUnits::Cm;
        else
            return false;
        ptr += 2;
        break;
    case 'm':
        if(c[1] == 'm')
            units = LengthUnits::Mm;
        else
            return false;
        ptr += 2;
        break;
    case 'e':
        if(c[1] == 'm')
            units = LengthUnits::Em;
        else if(c[1] == 'x')
            units = LengthUnits::Ex;
        else
            return false;
        ptr += 2;
        break;
    default:
        units = LengthUnits::Number;
        break;
    }

    return true;
}

bool Parser::parseNumberList(const char*& ptr, const char* end, double* values, int count)
{
    for(int i = 0; i < count; i++) {
        if(!Utils::parseNumber(ptr, end, values[i]))
            return false;
        Utils::skipWsComma(ptr, end);
    }

    return true;
}

bool Parser::parseArcFlag(const char*& ptr, const char* end, bool& flag)
{
    if(ptr < end && *ptr == '0')
        flag = false;
    else if(ptr < end && *ptr == '1')
        flag = true;
    else
        return false;

    ++ptr;
    Utils::skipWsComma(ptr, end);
    return true;
}

bool Parser::parseColorComponent(const char*& ptr, const char* end, uint8_t& component)
{
    double value = 0;
    if(!Utils::parseNumber(ptr, end, value))
        return false;

    if(Utils::skipDesc(ptr, end, '%'))
        value *= 2.55;

    value = clamp(value, 0.0, 255.0);
    component = static_cast<uint8_t>(std::round(value));
    return true;
}

bool Parser::parseUrlFragment(const char*& ptr, const char* end, std::string& ref)
{
    if(!Utils::skipDesc(ptr, end, "url(")
        || !Utils::skipWs(ptr, end)) {
        return false;
    }

    switch(*ptr) {
    case '\'':
    case '"': {
        auto delim = *ptr;
        ++ptr; // delim
        Utils::skipWs(ptr, end);
        if(ptr >= end || *ptr != '#')
            return false;
        ++ptr; // #
        if(!Utils::readUntil(ptr, end, delim, ref))
            return false;
        ++ptr; // delim
        break;
    }

    case '#':
        ++ptr; // #
        Utils::readUntil(ptr, end, ')', ref);
        break;
    default:
        return false;
    }

    if(ptr >= end || *ptr != ')')
        return false;

    ++ptr; // )
    Utils::skipWs(ptr, end);
    return true;
}

bool Parser::parseTransform(const char*& ptr, const char* end, TransformType& type, double* values, int& count)
{
    int required = 0;
    int optional = 0;
    if(Utils::skipDesc(ptr, end, "matrix")) {
        type = TransformType::Matrix;
        required = 6;
        optional = 0;
    } else if(Utils::skipDesc(ptr, end, "rotate")) {
        type = TransformType::Rotate;
        required = 1;
        optional = 2;
    } else if(Utils::skipDesc(ptr, end, "scale")) {
        type = TransformType::Scale;
        required = 1;
        optional = 1;
    } else if(Utils::skipDesc(ptr, end, "skewX")) {
        type = TransformType::SkewX;
        required = 1;
        optional = 0;
    } else if(Utils::skipDesc(ptr, end, "skewY")) {
        type = TransformType::SkewY;
        required = 1;
        optional = 0;
    } else if(Utils::skipDesc(ptr, end, "translate")) {
        type = TransformType::Translate;
        required = 1;
        optional = 1;
    } else {
        return false;
    }

    Utils::skipWs(ptr, end);
    if(ptr >= end || *ptr != '(')
        return false;
    ++ptr;

    int maxCount = required + optional;
    count = 0;
    Utils::skipWs(ptr, end);
    while(count < maxCount) {
        if(!Utils::parseNumber(ptr, end, values[count]))
            break;
        ++count;
        Utils::skipWsComma(ptr, end);
    }

    if(ptr >= end || *ptr != ')' || !(count == required || count == maxCount))
        return false;
    ++ptr;

    return true;
}

ElementID elementid(const std::string& name)
{
    static const std::map<std::string, ElementID> elementmap = {
        {"a", ElementID::G},
        {"circle", ElementID::Circle},
        {"clipPath", ElementID::ClipPath},
        {"defs", ElementID::Defs},
        {"ellipse", ElementID::Ellipse},
        {"g", ElementID::G},
        {"line", ElementID::Line},
        {"linearGradient", ElementID::LinearGradient},
        {"marker", ElementID::Marker},
        {"mask", ElementID::Mask},
        {"path", ElementID::Path},
        {"pattern", ElementID::Pattern},
        {"polygon", ElementID::Polygon},
        {"polyline", ElementID::Polyline},
        {"radialGradient", ElementID::RadialGradient},
        {"rect", ElementID::Rect},
        {"stop", ElementID::Stop},
        {"style", ElementID::Style},
        {"solidColor", ElementID::SolidColor},
        {"svg", ElementID::Svg},
        {"symbol", ElementID::Symbol},
        {"use", ElementID::Use}
    };

    auto it = elementmap.find(name);
    if(it == elementmap.end())
        return ElementID::Unknown;
    return it->second;
}

PropertyID csspropertyid(const std::string& name)
{
    static const std::map<std::string, PropertyID> csspropertymap = {
        {"clip-path", PropertyID::Clip_Path},
        {"clip-rule", PropertyID::Clip_Rule},
        {"color", PropertyID::Color},
        {"display", PropertyID::Display},
        {"fill", PropertyID::Fill},
        {"fill-opacity", PropertyID::Fill_Opacity},
        {"fill-rule", PropertyID::Fill_Rule},
        {"marker-end", PropertyID::Marker_End},
        {"marker-mid", PropertyID::Marker_Mid},
        {"marker-start", PropertyID::Marker_Start},
        {"mask", PropertyID::Mask},
        {"opacity", PropertyID::Opacity},
        {"overflow", PropertyID::Overflow},
        {"solid-color", PropertyID::Solid_Color},
        {"solid-opacity", PropertyID::Solid_Opacity},
        {"stop-color", PropertyID::Stop_Color},
        {"stop-opacity", PropertyID::Stop_Opacity},
        {"stroke", PropertyID::Stroke},
        {"stroke-dasharray", PropertyID::Stroke_Dasharray},
        {"stroke-dashoffset", PropertyID::Stroke_Dashoffset},
        {"stroke-linecap", PropertyID::Stroke_Linecap},
        {"stroke-linejoin", PropertyID::Stroke_Linejoin},
        {"stroke-miterlimit", PropertyID::Stroke_Miterlimit},
        {"stroke-opacity", PropertyID::Stroke_Opacity},
        {"stroke-width", PropertyID::Stroke_Width},
        {"visibility", PropertyID::Visibility}
    };

    auto it = csspropertymap.find(name);
    if(it == csspropertymap.end())
        return PropertyID::Unknown;
    return it->second;
}

PropertyID propertyid(const std::string& name)
{
    static const std::map<std::string, PropertyID> propertymap = {
        {"class", PropertyID::Class},
        {"clipPathUnits", PropertyID::ClipPathUnits},
        {"cx", PropertyID::Cx},
        {"cy", PropertyID::Cy},
        {"d", PropertyID::D},
        {"fx", PropertyID::Fx},
        {"fy", PropertyID::Fy},
        {"gradientTransform", PropertyID::GradientTransform},
        {"gradientUnits", PropertyID::GradientUnits},
        {"height", PropertyID::Height},
        {"href", PropertyID::Href},
        {"id", PropertyID::Id},
        {"markerHeight", PropertyID::MarkerHeight},
        {"markerUnits", PropertyID::MarkerUnits},
        {"markerWidth", PropertyID::MarkerWidth},
        {"maskContentUnits", PropertyID::MaskContentUnits},
        {"maskUnits", PropertyID::MaskUnits},
        {"offset", PropertyID::Offset},
        {"orient", PropertyID::Orient},
        {"patternContentUnits", PropertyID::PatternContentUnits},
        {"patternTransform", PropertyID::PatternTransform},
        {"patternUnits", PropertyID::PatternUnits},
        {"points", PropertyID::Points},
        {"preserveAspectRatio", PropertyID::PreserveAspectRatio},
        {"r", PropertyID::R},
        {"refX", PropertyID::RefX},
        {"refY", PropertyID::RefY},
        {"rx", PropertyID::Rx},
        {"ry", PropertyID::Ry},
        {"spreadMethod", PropertyID::SpreadMethod},
        {"style", PropertyID::Style},
        {"transform", PropertyID::Transform},
        {"viewBox", PropertyID::ViewBox},
        {"width", PropertyID::Width},
        {"x", PropertyID::X},
        {"x1", PropertyID::X1},
        {"x2", PropertyID::X2},
        {"xlink:href", PropertyID::Href},
        {"y", PropertyID::Y},
        {"y1", PropertyID::Y1},
        {"y2", PropertyID::Y2}
    };

    auto it = propertymap.find(name);
    if(it == propertymap.end())
        return csspropertyid(name);
    return it->second;
}

bool RuleData::match(const Element* element) const
{
    if(m_selector.empty())
        return false;

    if(m_selector.size() == 1)
        return matchSimpleSelector(m_selector.front(), element);

    auto it = m_selector.rbegin();
    auto end = m_selector.rend();
    if(!matchSimpleSelector(*it, element))
        return false;
    ++it;

    while(it != end) {
        switch(it->combinator) {
        case SimpleSelector::Combinator::Child:
        case SimpleSelector::Combinator::Descendant:
            element = element->parent();
            break;
        case SimpleSelector::Combinator::DirectAdjacent:
        case SimpleSelector::Combinator::InDirectAdjacent:
            element = element->previousElement();
            break;
        }

        if(element == nullptr)
            return false;

        if(matchSimpleSelector(*it, element)) {
            ++it;
        } else if(it->combinator != SimpleSelector::Combinator::Descendant
            && it->combinator != SimpleSelector::Combinator::InDirectAdjacent) {
            return false;
        }
    }

    return true;
}

bool RuleData::matchSimpleSelector(const SimpleSelector& selector, const Element* element)
{
    if(selector.id != ElementID::Star && selector.id != element->id())
        return false;

    for(auto& sel : selector.attributeSelectors) {
        if(!matchAttributeSelector(sel, element)) {
            return false;
        }
    }

    for(auto& sel : selector.pseudoClassSelectors) {
        if(!matchPseudoClassSelector(sel, element)) {
            return false;
        }
    }

    return true;
}

bool RuleData::matchAttributeSelector(const AttributeSelector& selector, const Element* element)
{
    auto& value = element->get(selector.id);
    if(value.empty())
        return false;

    if(selector.matchType == AttributeSelector::MatchType::None)
        return true;

    if(selector.matchType == AttributeSelector::MatchType::Equal)
        return selector.value == value;

    if(selector.matchType == AttributeSelector::MatchType::Includes) {
        auto ptr = value.data();
        auto end = ptr + value.size();
        while(ptr < end) {
            auto start = ptr;
            while(ptr < end && !IS_WS(*ptr))
                ++ptr;
            if(selector.value == std::string(start, ptr))
                return true;
            Utils::skipWs(ptr, end);
        }

        return false;
    }

    auto starts_with = [](const std::string& string, const std::string& prefix) {
        if(prefix.empty() || prefix.size() > string.size())
            return false;
        return string.compare(0, prefix.size(), prefix) == 0;
    };

    auto ends_with = [](const std::string& string, const std::string& suffix) {
        if(suffix.empty() || suffix.size() > string.size())
            return false;
        return string.compare(string.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    if(selector.matchType == AttributeSelector::MatchType::DashMatch) {
        if(selector.value == value)
            return true;
        return starts_with(value, selector.value + '-');
    }

    if(selector.matchType == AttributeSelector::MatchType::StartsWith)
        return starts_with(value, selector.value);

    if(selector.matchType == AttributeSelector::MatchType::EndsWith)
        return ends_with(value, selector.value);

    if(selector.matchType == AttributeSelector::MatchType::Contains)
        return value.find(selector.value) != std::string::npos;

    return false;
}

bool RuleData::matchPseudoClassSelector(const PseudoClassSelector& selector, const Element* element)
{
    if(selector.type == PseudoClassSelector::Type::Empty)
        return element->children().empty();
    if(selector.type == PseudoClassSelector::Type::Root)
        return element->parent() == nullptr;
    if(selector.type == PseudoClassSelector::Type::Is) {
        for(auto& subselector : selector.subSelectors) {
            for(auto& sel : subselector) {
                if(!matchSimpleSelector(sel, element)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::Not) {
        for(auto& subselector : selector.subSelectors) {
            for(auto& sel : subselector) {
                if(matchSimpleSelector(sel, element)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::FirstChild)
        return !element->previousElement();

    if(selector.type == PseudoClassSelector::Type::LastChild)
        return !element->nextElement();

    if(selector.type == PseudoClassSelector::Type::OnlyChild)
        return !(element->previousElement() || element->nextElement());

    if(selector.type == PseudoClassSelector::Type::FirstOfType) {
        auto sibling = element->previousElement();
        while(sibling) {
            if(sibling->id() == element->id())
                return false;
            sibling = element->previousElement();
        }

        return true;
    }

    if(selector.type == PseudoClassSelector::Type::LastOfType) {
        auto sibling = element->nextElement();
        while(sibling) {
            if(sibling->id() == element->id())
                return false;
            sibling = element->nextElement();
        }

        return true;
    }

    return false;
}

#define IS_STARTNAMECHAR(c) (IS_ALPHA(c) ||  (c) == '_' || (c) == ':')
#define IS_NAMECHAR(c) (IS_STARTNAMECHAR(c) || IS_NUM(c) || (c) == '-' || (c) == '.')
static inline bool readIdentifier(const char*& ptr, const char* end, std::string& value)
{
    if(ptr >= end || !IS_STARTNAMECHAR(*ptr))
        return false;

    auto start = ptr;
    ++ptr;
    while(ptr < end && IS_NAMECHAR(*ptr))
        ++ptr;

    value.assign(start, ptr);
    return true;
}

#define IS_CSS_STARTNAMECHAR(c) (IS_ALPHA(c) || (c) == '_' || (c) == '-')
#define IS_CSS_NAMECHAR(c) (IS_CSS_STARTNAMECHAR(c) || IS_NUM(c))
static inline bool readCSSIdentifier(const char*& ptr, const char* end, std::string& value)
{
    if(ptr >= end || !IS_CSS_STARTNAMECHAR(*ptr))
        return false;

    auto start = ptr;
    ++ptr;
    while(ptr < end && IS_CSS_NAMECHAR(*ptr))
        ++ptr;

    value.assign(start, ptr);
    return true;
}

bool StyleSheet::parse(const std::string& value)
{
    auto ptr = value.data();
    auto end = ptr + value.size();

    while(ptr < end) {
        Utils::skipWs(ptr, end);
        if(Utils::skipDesc(ptr, end, '@')) {
            if(!parseAtRule(ptr, end))
                return false;
            continue;
        }

        Rule rule;
        if(!parseRule(ptr, end, rule))
            return false;
        add(rule);
    }

    return true;
}

void StyleSheet::add(const Rule& rule)
{
    for(auto& selector : rule.selectors) {
        uint32_t specificity = 0;
        for(auto& simpleSelector : selector) {
            specificity += (simpleSelector.id == ElementID::Star) ? 0x0 : 0x1;
            for(auto& attributeSelector : simpleSelector.attributeSelectors) {
                specificity += (attributeSelector.id == PropertyID::Id) ? 0x10000 : 0x100;
            }
        }

        m_rules.emplace(selector, rule.declarations, specificity, m_position);
    }

    m_position += 1;
}

bool StyleSheet::parseAtRule(const char*& ptr, const char* end)
{
    int depth = 0;
    while(ptr < end) {
        auto ch = *ptr;
        ++ptr;
        if(ch == ';' && depth == 0)
            break;
        if(ch == '{') ++depth;
        else if(ch == '}' && depth > 0) {
            if(depth == 1)
                break;
            --depth;
        }
    }

    return true;
}

bool StyleSheet::parseRule(const char*& ptr, const char* end, Rule& rule)
{
    if(!parseSelectors(ptr, end, rule.selectors))
        return false;

    if(!parseDeclarations(ptr, end, rule.declarations))
        return false;

    return true;
}

bool StyleSheet::parseSelectors(const char*& ptr, const char* end, SelectorList& selectors)
{
    Selector selector;
    if(!parseSelector(ptr, end, selector))
        return false;
    selectors.push_back(std::move(selector));

    while(Utils::skipDesc(ptr, end, ',')) {
        Utils::skipWs(ptr, end);
        if(!parseSelector(ptr, end, selector))
            return false;
        selectors.push_back(std::move(selector));
    }

    return true;
}

bool StyleSheet::parseDeclarations(const char*& ptr, const char* end, DeclarationList& declarations)
{
    if(!Utils::skipDesc(ptr, end, '{'))
        return false;

    Utils::skipWs(ptr, end);
    do {
        std::string name;
        if(!readCSSIdentifier(ptr, end, name))
            return false;
        Utils::skipWs(ptr, end);
        if(!Utils::skipDesc(ptr, end, ':'))
            return false;
        Utils::skipWs(ptr, end);
        auto start = ptr;
        while(ptr < end && !(*ptr == '!' || *ptr == ';' || *ptr == '}'))
            ++ptr;

        Declaration declaration;
        declaration.specificity = 0x10;
        declaration.id = csspropertyid(name);
        declaration.value.assign(start, Utils::rtrim(start, ptr));
        if(Utils::skipDesc(ptr, end, '!')) {
            if(!Utils::skipDesc(ptr, end, "important"))
                return false;
            declaration.specificity = 0x1000;
        }

        if(declaration.id != PropertyID::Unknown)
            declarations.push_back(std::move(declaration));
        Utils::skipWsDelimiter(ptr, end, ';');
    } while(ptr < end && *ptr != '}');

    return Utils::skipDesc(ptr, end, '}');
}

#define IS_SELECTOR_STARTNAMECHAR(c) (IS_CSS_STARTNAMECHAR(c) || (c) == '*' || (c) == '#' || (c) == '.' || (c) == '[' || (c) == ':')
bool StyleSheet::parseSelector(const char*& ptr, const char* end, Selector& selector)
{
    do {
        SimpleSelector simpleSelector;
        if(!parseSimpleSelector(ptr, end, simpleSelector))
            return false;
        selector.push_back(std::move(simpleSelector));
        Utils::skipWs(ptr, end);
    } while(ptr < end && IS_SELECTOR_STARTNAMECHAR(*ptr));

    return true;
}

bool StyleSheet::parseSimpleSelector(const char*& ptr, const char* end, SimpleSelector& simpleSelector)
{
    std::string name;
    if(Utils::skipDesc(ptr, end, '*'))
        simpleSelector.id = ElementID::Star;
    else if(readCSSIdentifier(ptr, end, name))
        simpleSelector.id = elementid(name);

    while(ptr < end) {
        if(Utils::skipDesc(ptr, end, '#')) {
            AttributeSelector a;
            a.id = PropertyID::Id;
            a.matchType = AttributeSelector::MatchType::Equal;
            if(!readCSSIdentifier(ptr, end, a.value))
                return false;
            simpleSelector.attributeSelectors.push_back(std::move(a));
            continue;
        }

        if(Utils::skipDesc(ptr, end, '.')) {
            AttributeSelector a;
            a.id = PropertyID::Class;
            a.matchType = AttributeSelector::MatchType::Includes;
            if(!readCSSIdentifier(ptr, end, a.value))
                return false;
            simpleSelector.attributeSelectors.push_back(std::move(a));
            continue;
        }

        if(Utils::skipDesc(ptr, end, '[')) {
            Utils::skipWs(ptr, end);
            if(!readCSSIdentifier(ptr, end, name))
                return false;
            AttributeSelector a;
            a.id = propertyid(name);
            if(Utils::skipDesc(ptr, end, '='))
                a.matchType = AttributeSelector::MatchType::Equal;
            else if(Utils::skipDesc(ptr, end, "~="))
                a.matchType = AttributeSelector::MatchType::Includes;
            else if(Utils::skipDesc(ptr, end, "|="))
                a.matchType = AttributeSelector::MatchType::DashMatch;
            else if(Utils::skipDesc(ptr, end, "^="))
                a.matchType = AttributeSelector::MatchType::StartsWith;
            else if(Utils::skipDesc(ptr, end, "$="))
                a.matchType = AttributeSelector::MatchType::EndsWith;
            else if(Utils::skipDesc(ptr, end, "*="))
                a.matchType = AttributeSelector::MatchType::Contains;
            if(a.matchType != AttributeSelector::MatchType::None) {
                Utils::skipWs(ptr, end);
                if(!readCSSIdentifier(ptr, end, a.value)) {
                    if(ptr >= end || !(*ptr == '\"' || *ptr == '\''))
                        return false;

                    auto quote = *ptr;
                    ++ptr;
                    if(!Utils::readUntil(ptr, end, quote, a.value))
                        return false;
                    ++ptr;
                }
            }

            Utils::skipWs(ptr, end);
            if(!Utils::skipDesc(ptr, end, ']'))
                return false;
            simpleSelector.attributeSelectors.push_back(std::move(a));
            continue;
        }

        if(Utils::skipDesc(ptr, end, ':')) {
            if(!readCSSIdentifier(ptr, end, name))
                return false;
            PseudoClassSelector selector;
            if(name.compare("empty") == 0)
                selector.type = PseudoClassSelector::Type::Empty;
            else if(name.compare("root") == 0)
                selector.type = PseudoClassSelector::Type::Root;
            else if(name.compare("not") == 0)
                selector.type = PseudoClassSelector::Type::Not;
            else if(name.compare("first-child") == 0)
                selector.type = PseudoClassSelector::Type::FirstChild;
            else if(name.compare("last-child") == 0)
                selector.type = PseudoClassSelector::Type::LastChild;
            else if(name.compare("only-child") == 0)
                selector.type = PseudoClassSelector::Type::OnlyChild;
            else if(name.compare("first-of-type") == 0)
                selector.type = PseudoClassSelector::Type::FirstOfType;
            else if(name.compare("last-of-type") == 0)
                selector.type = PseudoClassSelector::Type::LastOfType;
            else if(name.compare("only-of-type") == 0)
                selector.type = PseudoClassSelector::Type::OnlyOfType;
            if(selector.type == PseudoClassSelector::Type::Is || selector.type == PseudoClassSelector::Type::Not) {
                if(!Utils::skipDesc(ptr, end, '('))
                    return false;

                Utils::skipWs(ptr, end);
                if(!parseSelectors(ptr, end, selector.subSelectors))
                    return false;

                Utils::skipWs(ptr, end);
                if(!Utils::skipDesc(ptr, end, ')')) {
                    return false;
                }
            }

            simpleSelector.pseudoClassSelectors.push_back(std::move(selector));
            continue;
        }

        break;
    }

    Utils::skipWs(ptr, end);
    if(Utils::skipDesc(ptr, end, '>'))
        simpleSelector.combinator = SimpleSelector::Combinator::Child;
    else if(Utils::skipDesc(ptr, end, '+'))
        simpleSelector.combinator = SimpleSelector::Combinator::DirectAdjacent;
    else if(Utils::skipDesc(ptr, end, '~'))
        simpleSelector.combinator = SimpleSelector::Combinator::InDirectAdjacent;

    return true;
}

static inline bool decodeText(const char* ptr, const char* end, std::string& value)
{
    value.clear();
    while(ptr < end) {
        auto ch = *ptr;
        ++ptr;
        if(ch != '&') {
            value.push_back(ch);
            continue;
        }

        if(Utils::skipDesc(ptr, end, '#')) {
            int base = 10;
            if(Utils::skipDesc(ptr, end, 'x'))
                base = 16;

            unsigned int cp;
            if(!Utils::parseInteger(ptr, end, cp, base))
                return false;

            char c[5] = {0, 0, 0, 0, 0};
            if(cp < 0x80) {
                c[1] = 0;
                c[0] = static_cast<char>(cp);
            } else if(cp < 0x800) {
                c[2] = 0;
                c[1] = static_cast<char>((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[0] = static_cast<char>(cp | 0xC0);
            } else if(cp < 0x10000) {
                c[3] = 0;
                c[2] = static_cast<char>((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[1] = static_cast<char>((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[0] = static_cast<char>(cp | 0xE0);
            } else if(cp < 0x200000) {
                c[4] = 0;
                c[3] = static_cast<char>((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[2] = static_cast<char>((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[1] = static_cast<char>((cp & 0x3F) | 0x80);
                cp >>= 6;
                c[0] = static_cast<char>(cp | 0xF0);
            }

            value.append(c);
        } else {
            if(Utils::skipDesc(ptr, end, "amp"))
                value.push_back('&');
            else if(Utils::skipDesc(ptr, end, "lt"))
                value.push_back('<');
            else if(Utils::skipDesc(ptr, end, "gt"))
                value.push_back('>');
            else if(Utils::skipDesc(ptr, end, "quot"))
                value.push_back('\"');
            else if(Utils::skipDesc(ptr, end, "apos"))
                value.push_back('\'');
            else {
                return false;
            }
        }

        if(!Utils::skipDesc(ptr, end, ';')) {
            return false;
        }
    }

    return true;
}

static inline void parseStyle(const std::string& string, Element* element)
{
    auto ptr = string.data();
    auto end = ptr + string.size();

    std::string name;
    std::string value;
    Utils::skipWs(ptr, end);
    while(ptr < end && readCSSIdentifier(ptr, end, name)) {
        Utils::skipWs(ptr, end);
        if(!Utils::skipDesc(ptr, end, ':'))
            return;
        Utils::skipWs(ptr, end);
        auto start = ptr;
        while(ptr < end && *ptr != ';')
            ++ptr;
        value.assign(start, Utils::rtrim(start, ptr));
        auto id = csspropertyid(name);
        if(id != PropertyID::Unknown)
            element->set(id, value, 0x100);
        Utils::skipWsDelimiter(ptr, end, ';');
    }
}

static inline void removeComments(std::string& value)
{
    auto start = value.find("/*");
    while(start != std::string::npos) {
        auto end = value.find("*/", start + 2);
        value.erase(start, end - start + 2);
        start = value.find("/*");
    }
}

bool Document::parse(const char* data, std::size_t size)
{
    auto ptr = data;
    auto end = ptr + size;

    StyleSheet styleSheet;
    Element* current = nullptr;
    std::string name;
    std::string value;
    int ignoring = 0;
    auto handleText = [&](const char* start, const char* end, bool in_cdata) {
        if(ignoring > 0 || current == nullptr || current->id() != ElementID::Style)
            return;

        if(in_cdata)
            value.assign(start, end);
        else
            decodeText(start, end, value);

        removeComments(value);
        styleSheet.parse(value);
    };

    while(ptr < end) {
        auto start = ptr;
        if(!Utils::skipUntil(ptr, end, '<'))
            break;

        handleText(start, ptr, false);
        ptr += 1;

        if(ptr < end && *ptr == '/') {
            if(current == nullptr && ignoring == 0)
                return false;

            ++ptr;
            if(!readIdentifier(ptr, end, name))
                return false;

            Utils::skipWs(ptr, end);
            if(ptr >= end || *ptr != '>')
                return false;

            if(ignoring > 0)
                --ignoring;
            else
                current = current->parent();
            ++ptr;
            continue;
        }

        if(ptr < end && *ptr == '?') {
            ++ptr;
            if(!readIdentifier(ptr, end, name))
                return false;

            if(!Utils::skipUntil(ptr, end, "?>"))
                return false;

            ptr += 2;
            continue;
        }

        if(ptr < end && *ptr == '!') {
            ++ptr;
            if(Utils::skipDesc(ptr, end, "--")) {
                start = ptr;
                if(!Utils::skipUntil(ptr, end, "-->"))
                    return false;

                handleText(start, ptr, false);
                ptr += 3;
                continue;
            }

            if(Utils::skipDesc(ptr, end, "[CDATA[")) {
                start = ptr;
                if(!Utils::skipUntil(ptr, end, "]]>"))
                    return false;

                handleText(start, ptr, true);
                ptr += 3;
                continue;
            }

            if(Utils::skipDesc(ptr, end, "DOCTYPE")) {
                while(ptr < end && *ptr != '>') {
                    if(*ptr == '[') {
                        ++ptr;
                        int depth = 1;
                        while(ptr < end && depth > 0) {
                            if(*ptr == '[') ++depth;
                            else if(*ptr == ']') --depth;
                            ++ptr;
                        }
                    } else {
                        ++ptr;
                    }
                }

                if(ptr >= end || *ptr != '>')
                    return false;

                ptr += 1;
                continue;
            }

            return false;
        }

        if(!readIdentifier(ptr, end, name))
            return false;

        auto id = ElementID::Unknown;
        if(ignoring == 0)
            id = elementid(name);
        if(id == ElementID::Unknown)
            ++ignoring;

        Element* element = nullptr;
        if(ignoring == 0) {
            if(m_rootElement && current == nullptr)
                return false;

            if(m_rootElement == nullptr) {
                if(id != ElementID::Svg)
                    return false;
                m_rootElement = makeUnique<SVGElement>();
                element = m_rootElement.get();
            } else {
                auto child = Element::create(id);
                element = child.get();
                current->addChild(std::move(child));
            }
        }

        Utils::skipWs(ptr, end);
        while(ptr < end && readIdentifier(ptr, end, name)) {
            Utils::skipWs(ptr, end);
            if(ptr >= end || *ptr != '=')
                return false;
            ++ptr;

            Utils::skipWs(ptr, end);
            if(ptr >= end || !(*ptr == '\"' || *ptr == '\''))
                return false;

            auto quote = *ptr;
            ++ptr;
            Utils::skipWs(ptr, end);
            start = ptr;
            while(ptr < end && *ptr != quote)
                ++ptr;

            if(ptr >= end || *ptr != quote)
                return false;

            auto attrId = PropertyID::Unknown;
            if(element != nullptr)
                attrId = propertyid(name);
            if(attrId != PropertyID::Unknown) {
                decodeText(start, Utils::rtrim(start, ptr), value);
                if(attrId == PropertyID::Style) {
                    removeComments(value);
                    parseStyle(value, element);
                } else {
                    if(attrId == PropertyID::Id)
                        m_idCache.emplace(value, element);
                    element->set(attrId, value, 0x1);
                }
            }

            ++ptr;
            Utils::skipWs(ptr, end);
        }

        if(ptr < end && *ptr == '>') {
            if(element != nullptr)
                current = element;

            ++ptr;
            continue;
        }

        if(ptr < end && *ptr == '/') {
            ++ptr;
            if(ptr >= end || *ptr != '>')
                return false;

            if(ignoring > 0)
                --ignoring;

            ++ptr;
            continue;
        }

        return false;
    }

    if(!m_rootElement || ptr < end || ignoring > 0)
        return false;
    if(!styleSheet.empty()) {
        m_rootElement->transverse([&styleSheet](Node* node) {
            if(node->isText())
                return true;

            auto element = static_cast<Element*>(node);
            for(auto& rule : styleSheet.rules()) {
                if(rule.match(element)) {
                    for(auto& declaration : rule.declarations()) {
                        element->set(declaration.id, declaration.value, declaration.specificity);
                    }
                }
            }

            return true;
        });
    }

    m_rootElement->build(this);
    return true;
}

} // namespace lunasvg
