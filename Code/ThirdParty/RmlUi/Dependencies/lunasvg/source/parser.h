#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <set>

#include "property.h"
#include "element.h"

namespace lunasvg {

class SVGElement;
class StyledElement;

enum LengthNegativeValuesMode {
    AllowNegativeLengths,
    ForbidNegativeLengths
};

enum class TransformType {
    Matrix,
    Rotate,
    Scale,
    SkewX,
    SkewY,
    Translate
};

class Parser {
public:
    static Length parseLength(const std::string& string, LengthNegativeValuesMode mode, const Length& defaultValue);
    static LengthList parseLengthList(const std::string& string, LengthNegativeValuesMode mode);
    static double parseNumber(const std::string& string, double defaultValue);
    static double parseNumberPercentage(const std::string& string, double defaultValue);
    static PointList parsePointList(const std::string& string);
    static Transform parseTransform(const std::string& string);
    static Path parsePath(const std::string& string);
    static std::string parseUrl(const std::string& string);
    static std::string parseHref(const std::string& string);
    static Rect parseViewBox(const std::string& string);
    static PreserveAspectRatio parsePreserveAspectRatio(const std::string& string);
    static Angle parseAngle(const std::string& string);
    static MarkerUnits parseMarkerUnits(const std::string& string);
    static SpreadMethod parseSpreadMethod(const std::string& string);
    static Units parseUnits(const std::string& string, Units defaultValue);
    static Color parseColor(const std::string& string, const StyledElement* element, const Color& defaultValue);
    static Paint parsePaint(const std::string& string, const StyledElement* element, const Color& defaultValue);
    static WindRule parseWindRule(const std::string& string);
    static LineCap parseLineCap(const std::string& string);
    static LineJoin parseLineJoin(const std::string& string);
    static Display parseDisplay(const std::string& string);
    static Visibility parseVisibility(const std::string& string);
    static Overflow parseOverflow(const std::string& string, Overflow defaultValue);

private:
    static bool parseLength(const char*& ptr, const char* end, double& value, LengthUnits& units, LengthNegativeValuesMode mode);
    static bool parseNumberList(const char*& ptr, const char* end, double* values, int count);
    static bool parseArcFlag(const char*& ptr, const char* end, bool& flag);
    static bool parseColorComponent(const char*& ptr, const char* end, uint8_t& component);
    static bool parseUrlFragment(const char*& ptr, const char* end, std::string& ref);
    static bool parseTransform(const char*& ptr, const char* end, TransformType& type, double* values, int& count);
};

struct SimpleSelector;

using Selector = std::vector<SimpleSelector>;
using SelectorList = std::vector<Selector>;

struct AttributeSelector {
    enum class MatchType {
        None,
        Equal,
        Includes,
        DashMatch,
        StartsWith,
        EndsWith,
        Contains
    };

    MatchType matchType{MatchType::None};
    PropertyID id{PropertyID::Unknown};
    std::string value;
};

struct PseudoClassSelector {
    enum class Type {
        Unknown,
        Empty,
        Root,
        Is,
        Not,
        FirstChild,
        LastChild,
        OnlyChild,
        FirstOfType,
        LastOfType,
        OnlyOfType
    };

    Type type{Type::Unknown};
    SelectorList subSelectors;
};

struct SimpleSelector {
    enum class Combinator {
        Descendant,
        Child,
        DirectAdjacent,
        InDirectAdjacent
    };

    Combinator combinator{Combinator::Descendant};
    ElementID id{ElementID::Star};
    std::vector<AttributeSelector> attributeSelectors;
    std::vector<PseudoClassSelector> pseudoClassSelectors;
};

struct Declaration {
    int specificity;
    PropertyID id;
    std::string value;
};

using DeclarationList = std::vector<Declaration>;

struct Rule {
    SelectorList selectors;
    DeclarationList declarations;
};

class RuleData {
public:
    RuleData(const Selector& selector, const DeclarationList& declarations, uint32_t specificity, uint32_t position)
        : m_selector(selector), m_declarations(declarations), m_specificity(specificity), m_position(position)
    {}

    const Selector& selector() const { return m_selector; }
    const DeclarationList& declarations() const { return m_declarations; }
    const uint32_t& specificity() const { return m_specificity; }
    const uint32_t& position() const { return m_position; }

    bool match(const Element* element) const;

private:
    static bool matchSimpleSelector(const SimpleSelector& selector, const Element* element);
    static bool matchAttributeSelector(const AttributeSelector& selector, const Element* element);
    static bool matchPseudoClassSelector(const PseudoClassSelector& selector, const Element* element);

    Selector m_selector;
    DeclarationList m_declarations;
    uint32_t m_specificity;
    uint32_t m_position;
};

inline bool operator<(const RuleData& a, const RuleData& b) { return std::tie(a.specificity(), a.position()) < std::tie(b.specificity(), b.position()); }
inline bool operator>(const RuleData& a, const RuleData& b) { return std::tie(a.specificity(), a.position()) > std::tie(b.specificity(), b.position()); }

class StyleSheet {
public:
    StyleSheet() = default;

    bool parse(const std::string& content);
    void add(const Rule& rule);
    bool empty() const { return m_rules.empty(); }

    const std::multiset<RuleData>& rules() const { return m_rules; }

private:
    static bool parseAtRule(const char*& ptr, const char* end);
    static bool parseRule(const char*& ptr, const char* end, Rule& rule);
    static bool parseSelectors(const char*& ptr, const char* end, SelectorList& selectors);
    static bool parseDeclarations(const char*& ptr, const char* end, DeclarationList& declarations);
    static bool parseSelector(const char*& ptr, const char* end, Selector& selector);
    static bool parseSimpleSelector(const char*& ptr, const char* end, SimpleSelector& simpleSelector);

    std::multiset<RuleData> m_rules;
    uint32_t m_position{0};
};

} // namespace lunasvg

#endif // PARSER_H
