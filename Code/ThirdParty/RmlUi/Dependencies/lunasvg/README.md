# LunaSVG

lunasvg is a standalone SVG rendering library in C++

![svg2png generated PNG](luna.png)

## Example

```cpp
#include <lunasvg/document.h>

using namespace lunasvg;

int main()
{
    auto document = Document::loadFromFile("tiger.svg");
    auto bitmap = document->renderToBitmap();

    // do something useful with the bitmap here.

    return 0;
}

```

## Features

- Basic Shapes
- Document Structures
- Coordinate Systems, Transformations and Units
- SolidColors
- Gradients
- Patterns
- Masks
- ClipPaths
- Markers

## TODO

- Texts
- Filters
- Images
- StyleSheet

## Build

```
git clone https://github.com/sammycage/lunasvg.git
cd lunasvg
mkdir build
cd build
cmake ..
make -j 2
```

To install lunasvg library.

```
make install
```

## Demo

While building lunasvg example it generates a simple SVG to PNG converter which can be used to convert SVG file to PNG file.

Run Demo.
```
svg2png [filename] [resolution] [bgColor]
```

## Support

If you like the work lunasvg is doing please consider a small donation:

<a href="https://www.buymeacoffee.com/sammycage"><img src="https://img.buymeacoffee.com/button-api/?text=Buy me a coffee&emoji=&slug=sammycage&button_colour=FFDD00&font_colour=000000&font_family=Arial&outline_colour=000000&coffee_colour=ffffff"></a>
