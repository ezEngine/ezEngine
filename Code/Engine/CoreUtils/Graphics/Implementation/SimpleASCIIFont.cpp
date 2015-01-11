#include <CoreUtils/PCH.h>
#include <CoreUtils/Graphics/SimpleASCIIFont.h>

static const char* a = "\
          \
          \
          \
  /ooo-   \
      o   \
  /oooo   \
  o   o   \
  /oooo";

static const char* b = "\
  o       \
  o       \
  o       \
  ooo+-   \
  o  -+   \
  o   o   \
  o  -+   \
  ooo+-";

static const char* c = "\
          \
          \
          \
  -+ooo   \
  +-      \
  o       \
  +-      \
  -+ooo";

static const char* d = "\
      o   \
      o   \
      o   \
  -+ooo   \
  +-  o   \
  o   o   \
  +-  o   \
  -+ooo";

static const char* e = "\
          \
          \
          \
  -ooo-   \
  o   o   \
  ooooo   \
  o       \
  -oooo";

static const char* f = "\
          \
    /oo   \
    o     \
    o     \
  ooooo   \
    o     \
    o     \
    o     \
    /";

static const char* g = "\
          \
          \
          \
  -ooo-   \
  o   o   \
  o   o   \
  o   o   \
  -oooo   \
      o   \
  /ooo-   ";

static const char* h = "\
  o       \
  o       \
  o       \
  ooo+-   \
  o  -+   \
  o   o   \
  o   o   \
  o   o";

static const char* i = "\
          \
    o     \
          \
    o     \
    o     \
    o     \
    o     \
    o";

static const char* j = "\
          \
    o     \
          \
    o     \
    o     \
    o     \
    o     \
   -+     \
  o+-";

static const char* k = "\
          \
  o       \
  o       \
  o  -o-  \
  o -o-   \
  oo+-    \
  o -o-   \
  o  -o-";

static const char* l = "\
          \
    o     \
    o     \
    o     \
    o     \
    o     \
    +-    \
    -+";

static const char* m = "\
          \
          \
          \
 -oo-oo-  \
 o  o  o  \
 o  o  o  \
 o  o  o  \
 o  o  o";

static const char* n = "\
          \
          \
          \
  -+o+-   \
  +- -+   \
  o   o   \
  o   o   \
  o   o";

static const char* o = "\
          \
          \
          \
  -+o+-   \
  +- -+   \
  o   o   \
  +- -+   \
  -+o+-";

static const char* p = "\
          \
          \
          \
  -+o+-   \
  +- -+   \
  o   o   \
  o  -+   \
  ooo+-   \
  o       \
  o       ";

static const char* q = "\
          \
          \
          \
  -+o+-   \
  +- -+   \
  o   o   \
  +-  +   \
  -+ooo   \
      o   \
      o   ";

static const char* r = "\
          \
          \
          \
  -+o+-   \
  +- -+   \
  o       \
  o       \
  o";

static const char* s = "\
          \
          \
          \
  -+ooo   \
  +-      \
  -+o+-   \
     -+   \
  ooo+-";

static const char* t = "\
    +     \
    o     \
  +ooo+   \
    o     \
    o     \
    o     \
    o     \
    o";

static const char* u = "\
          \
          \
          \
  o   o   \
  o   o   \
  o   o   \
  +-  o   \
  -+ooo";

static const char* v = "\
          \
          \
          \
  o   o   \
  o   o   \
  o- -o   \
  -+-+-   \
   -+-";

static const char* w = "\
          \
          \
          \
 o  o  o  \
 o  o  o  \
 o  o  o  \
 o  o  o  \
 -oo-oo-";

static const char* x = "\
          \
          \
          \
  o- -o   \
  -o-o-   \
   -o-    \
  -o-o-   \
  o- -o";

static const char* y = "\
          \
          \
          \
  o   o   \
  o   o   \
  o   o   \
  +-  o   \
  -+ooo   \
      o   \
  +ooo-   ";

static const char* z = "\
          \
          \
          \
  oooo+   \
    -+-   \
   -+-    \
  -+-     \
  +oooo";

static const char* A = "\
  -+o+-   \
  +- -+   \
  o   o   \
  o   o   \
  ooooo   \
  o   o   \
  o   o   \
  o   o";

static const char* B = "\
  ooo+-   \
  o  -+   \
  o   +   \
  ooo+-   \
  o  -+   \
  o   o   \
  o  -+   \
  ooo+-";

static const char* C = "\
  -ooo-   \
  o   o   \
  o   -   \
  o       \
  o       \
  o   -   \
  o   o   \
  -ooo-";

static const char* D = "\
  ooo+-   \
  o  -+   \
  o   o   \
  o   o   \
  o   o   \
  o   o   \
  o  -+   \
  ooo+-";

static const char* E = "\
  ooooo   \
  o       \
  o       \
  ooooo   \
  o       \
  o       \
  o       \
  ooooo";

static const char* F = "\
  ooooo   \
  o       \
  o       \
  ooooo   \
  o       \
  o       \
  o       \
  o";

static const char* G = "\
  -+o+-   \
  +- -o   \
  o       \
  o       \
  o +oo   \
  o   o   \
  +- -+   \
  -+o+-";

static const char* H = "\
  o   o   \
  o   o   \
  o   o   \
  ooooo   \
  o   o   \
  o   o   \
  o   o   \
  o   o";

static const char* I = "\
   ooo    \
    o     \
    o     \
    o     \
    o     \
    o     \
    o     \
   ooo";

static const char* J = "\
  ooooo   \
      o   \
      o   \
      o   \
      o   \
  o   o   \
  +- -+   \
  -+o+-";

static const char* K = "\
  o    o- \
  o   o-  \
  o  o-   \
  o-o-    \
  o+o-    \
  o  o-   \
  o   o-  \
  o    o-";

static const char* L = "\
  o       \
  o       \
  o       \
  o       \
  o       \
  o       \
  o       \
  oooooo";

static const char* M = "\
o/     /o \
oo-   -oo \
o-o- -o-o \
o -o/o- o \
o  -o-  o \
o       o \
o       o \
o       o";

static const char* N = "\
  o/   o  \
  o+-  o  \
  o//  o  \
  o-+- o  \
  o -+-o  \
  o  //o  \
  o  -+o  \
  o   /o";

static const char* O = "\
  -+oo+-  \
  +-  -+  \
  o    o  \
  o    o  \
  o    o  \
  o    o  \
  +-  -+  \
  -+oo+-";

static const char* P = "\
  oooo+-  \
  o   -+  \
  o    o  \
  o   -+  \
  oooo+-  \
  o       \
  o       \
  o";

static const char* Q = "\
  -+oo+-  \
  +-  -+  \
  o    o  \
  o    o  \
  o    o  \
  o  +-o  \
  +- -+o  \
  -+oooo";

static const char* R = "\
  oooo+-  \
  o   -+  \
  o    o  \
  o   -+  \
  oooo+-  \
  o -o-   \
  o  -o-  \
  o   -o-";

static const char* S = "\
  -+ooo+  \
  +-      \
  +-      \
  -+oo+-  \
      -+  \
       o  \
      -+  \
  +ooo+-";

static const char* T = "\
 +oooooo+ \
    o     \
    o     \
    o     \
    o     \
    o     \
    o     \
    o";

static const char* U = "\
  o    o  \
  o    o  \
  o    o  \
  o    o  \
  o    o  \
  o    o  \
  +-  -+  \
  -+oo+-";

static const char* V = "\
  o    o  \
  o    o  \
  o    o  \
  o    o  \
  //  //  \
   o  o   \
   o--o   \
    oo";

static const char* W = "\
o   o   o \
o   o   o \
o   o   o \
o   o   o \
+-  o  -+ \
-o -o- o- \
 +-o-o-+  \
 -o- -o-";

static const char* X = "\
  o    o  \
  +-  -+  \
  -o  o-  \
   -++-   \
   -++-   \
  -o  o-  \
  +-  -+  \
  o    o";

static const char* Y = "\
  o   o   \
  o   o   \
  +- -+   \
  -+-+-   \
   -+-    \
    o     \
    o     \
    o";

static const char* Z = "\
 ooooooo+ \
      -+- \
     -+-  \
    -+-   \
   -+-    \
  -+-     \
 -+-      \
 +ooooooo";

static const char* _0 = "\
  -oooo-  \
  oo- -o  \
  o-o  o  \
  o o- o  \
  o -o o  \
  o  o-o  \
  o- -oo  \
  -oooo-";

static const char* _1 = "\
    -o    \
   -oo    \
  -o-o    \
 -o- o    \
     o    \
     o    \
     o    \
     o";

static const char* _2 = "\
  -+oo+-  \
  +-  -+  \
      -o  \
     -+-  \
    -+-   \
   -+-    \
  -+-     \
  oooooo";

static const char* _3 = "\
  +ooo+-  \
      -+  \
       o  \
   -oo+-  \
      -+  \
       o  \
      -+  \
  +ooo+-";

static const char* _4 = "\
  o    o  \
  o    o  \
  o    o  \
  +-   o  \
  -+oooo  \
       o  \
       o  \
       o";

static const char* _5 = "\
  oooooo  \
  o       \
  o       \
  oooo+-  \
      -+  \
       o  \
      -+  \
  +ooo+-";

static const char* _6 = "\
  -+ooo+  \
  +-      \
  o       \
  oooo+-  \
  o   -+  \
  o    o  \
  +-  -+  \
  -+oo+-";

static const char* _7 = "\
   ooooo  \
       o  \
      -+  \
      +-  \
     -+   \
     +-   \
     o    \
     o";

static const char* _8 = "\
  /oooo/  \
  o    o  \
  o    o  \
  oooooo  \
  o    o  \
  o    o  \
  o    o  \
  /oooo/";

static const char* _9 = "\
  /oooo/  \
  o    o  \
  o    o  \
  o    o  \
  /ooooo  \
       o  \
       o  \
  /oooo/";

static const char* excl = "\
    o     \
    o     \
    o     \
    o     \
    o     \
          \
    o     \
    o";

static const char* dqu = "\
   o o    \
   o o    \
   o o";

static const char* squ = "\
    o     \
    o     \
    o";

static const char* hash = "\
          \
          \
          \
    o o   \
   ooooo  \
    o o   \
   ooooo  \
    o o";

static const char* dollar = "\
     o    \
  -+oooo+ \
  +- o    \
  +- o    \
  -+ooo+- \
     o -+ \
     o -+ \
  +oooo+- \
     o";

static const char* percent = "\
          \
          \
  +o  -o  \
  o+ -o-  \
    -o-   \
   -o-    \
  -o- +o  \
  o-  o+";

static const char* andsign = "\
          \
   -oo-   \
   o  o   \
   o  o   \
   -oo-   \
   o  +o- \
   o  o   \
   -oo-o-";

static const char* po = "\
    -+o   \
    +-    \
    o     \
    o     \
    o     \
    o     \
    +-    \
    -+o";

static const char* pc = "\
    o+-   \
     -+   \
      o   \
      o   \
      o   \
      o   \
     -+   \
    o+-";

static const char* star = "\
          \
          \
     o    \
   o/o/o  \
    o/o   \
   o/ /o";

static const char* plus = "\
          \
     o    \
     o    \
   ooooo  \
     o    \
     o";

static const char* minus = "\
          \
          \
          \
   ooooo";

static const char* comma = "\
          \
          \
          \
          \
          \
          \
  o       \
 -+       \
 +-";

static const char* dot = "\
          \
          \
          \
          \
          \
          \
   +-     \
   +-";

static const char* equal = "\
          \
          \
  ooooooo \
          \
  ooooooo";

static const char* slash = "\
     o    \
     o    \
    -+    \
    +-    \
   -+     \
   +-     \
   o      \
   o";

static const char* backslash = "\
   o      \
   o      \
   +-     \
   -+     \
    +-    \
    -+    \
     o    \
     o";

static const char* colon = "\
          \
          \
  +-      \
  +-      \
          \
          \
  +-      \
  +-";

static const char* semicolon = "\
          \
          \
  +-      \
  +-      \
          \
          \
  +-      \
 -+       \
 +-";

static const char* less = "\
          \
      -o- \
     -o-  \
    -o-   \
   -o-    \
    -o-   \
     -o-  \
      -o-";

static const char* more = "\
          \
 -o-      \
  -o-     \
   -o-    \
    -o-   \
   -o-    \
  -o-     \
 -o-";

static const char* abo = "\
    ooo   \
    o     \
    o     \
    o     \
    o     \
    o     \
    o     \
    o     \
    ooo";

static const char* abc = "\
   ooo    \
     o    \
     o    \
     o    \
     o    \
     o    \
     o    \
     o    \
   ooo";

static const char* underscore = "\
          \
          \
          \
          \
          \
          \
          \
 /oooooo/";

static const char* pipesign = "\
    o     \
    o     \
    o     \
    o     \
    o     \
    o     \
    o     \
    o     \
    o     \
    o     ";

static const char* dach = "\
   -o-    \
  -o-o-   \
 -o- -o-";

static const char* cbo = "\
     -+o  \
     +-   \
     o    \
    -+    \
   -o-    \
    -+    \
     o    \
     +-   \
     -+o";

static const char* cbc = "\
  o+-     \
   -+     \
    o     \
    +-    \
    -o-   \
    +-    \
    o     \
   -+     \
  o+-";

static const char* tilde = "\
          \
          \
  -++-  + \
  +--+--+ \
  +  -++-";

static const char* apostrophe = "\
    +-    \
    -+    \
     +-";

static const char* question = "\
  -ooo-   \
  o- -o   \
  +  -o   \
    -o-   \
    o-    \
    o     \
          \
    o     \
    +";

static const char* unknown = "\
          \
    o     \
    +     \
          \
    o     \
   -o     \
  -o-     \
  o-  +   \
  o- -o   \
  -ooo-   ";

static const char* at = "\
          \
 -+oooo+- \
 +-    -+ \
 o -ooo o \
 o o  o-+ \
 o -ooo+- \
 +-       \
 -+ooooo+";

// copies one 10x10 character into the 16x16 cell at offset x,y, adding the 3 pixel boundary and converting the bitmap into greyscale values
static void CopyCharacter(ezUInt32* pImage, ezInt32 c, const char* szChar)
{
  ezInt32 x = ((ezInt32) c % 16) * 16;
  ezInt32 y = ((ezInt32) c / 16) * 16;

  pImage += (y + 3) * 256; // 3 pixel boundary at the top
  pImage += x + 3; // 3 pixel boundary at the left side

  for (ezUInt32 i1 = 0; i1 < 10; ++i1)
  {
    for (ezUInt32 i2 = 0; i2 < 10; ++i2)
    {
      // convert character to greyscale value
      switch (*szChar)
      {
      case 'o':
        *pImage = 0xFFFFFFFF;  // white
        break;
      case '+':
        *pImage = 0xFFFFFFFF / 4 * 3; // 75% grey
        break;
      case '/':
        *pImage = 0xFFFFFFFF / 2; // 50% grey
        break;
      case '-':
        *pImage = 0xFFFFFFFF / 4; // 25% grey
        break;
      default:
        *pImage = 0; // black
        break;
      }

      // not used, but characters could be terminated early
      if (*szChar != '\0')
        szChar += 1;

      pImage += 1;
    }

    // one row down, 10 pixels back
    pImage += 246;
  }
}

void ezGraphicsUtils::CreateSimpleASCIIFontTexture(ezImage& Img, bool bSetEmptyToUnknown)
{
  Img.SetWidth(256);
  Img.SetHeight(128);
  Img.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  Img.AllocateImageData();

  // pre-fill with black
  {
    ezUInt32* pPixelData = Img.GetPixelPointer<ezUInt32>();

    for (ezUInt32 y = 0; y < Img.GetHeight(); ++y)
    {
      for (ezUInt32 x = 0; x < Img.GetWidth(); ++x)
      {
        *pPixelData = 0;

        pPixelData++;
      }
    }
  }

  ezUInt32* pPixelData = Img.GetPixelPointer<ezUInt32>();

  // fill all cells with the upside down question mark
  // all known cells will overwrite the cells again
  if (bSetEmptyToUnknown)
  {
    for (ezUInt32 i = 0; i < 128; ++i)
    {
      CopyCharacter(pPixelData, i, unknown);
    }
  }

// Lower Case Letters
  {
    CopyCharacter(pPixelData, 'a', a);
    CopyCharacter(pPixelData, 'b', b);
    CopyCharacter(pPixelData, 'c', c);
    CopyCharacter(pPixelData, 'd', d);
    CopyCharacter(pPixelData, 'e', e);
    CopyCharacter(pPixelData, 'f', f);
    CopyCharacter(pPixelData, 'g', g);
    CopyCharacter(pPixelData, 'h', h);
    CopyCharacter(pPixelData, 'i', i);
    CopyCharacter(pPixelData, 'j', j);
    CopyCharacter(pPixelData, 'k', k);
    CopyCharacter(pPixelData, 'l', l);
    CopyCharacter(pPixelData, 'm', m);
    CopyCharacter(pPixelData, 'n', n);
    CopyCharacter(pPixelData, 'o', o);
    CopyCharacter(pPixelData, 'p', p);
    CopyCharacter(pPixelData, 'q', q);
    CopyCharacter(pPixelData, 'r', r);
    CopyCharacter(pPixelData, 's', s);
    CopyCharacter(pPixelData, 't', t);
    CopyCharacter(pPixelData, 'u', u);
    CopyCharacter(pPixelData, 'v', v);
    CopyCharacter(pPixelData, 'w', w);
    CopyCharacter(pPixelData, 'x', x);
    CopyCharacter(pPixelData, 'y', y);
    CopyCharacter(pPixelData, 'z', z);
  }

  // Capitable Letters
  {
    CopyCharacter(pPixelData, 'A', A);
    CopyCharacter(pPixelData, 'B', B);
    CopyCharacter(pPixelData, 'C', C);
    CopyCharacter(pPixelData, 'D', D);
    CopyCharacter(pPixelData, 'E', E);
    CopyCharacter(pPixelData, 'F', F);
    CopyCharacter(pPixelData, 'G', G);
    CopyCharacter(pPixelData, 'H', H);
    CopyCharacter(pPixelData, 'I', I);
    CopyCharacter(pPixelData, 'J', J);
    CopyCharacter(pPixelData, 'K', K);
    CopyCharacter(pPixelData, 'L', L);
    CopyCharacter(pPixelData, 'M', M);
    CopyCharacter(pPixelData, 'N', N);
    CopyCharacter(pPixelData, 'O', O);
    CopyCharacter(pPixelData, 'P', P);
    CopyCharacter(pPixelData, 'Q', Q);
    CopyCharacter(pPixelData, 'R', R);
    CopyCharacter(pPixelData, 'S', S);
    CopyCharacter(pPixelData, 'T', T);
    CopyCharacter(pPixelData, 'U', U);
    CopyCharacter(pPixelData, 'V', V);
    CopyCharacter(pPixelData, 'W', W);
    CopyCharacter(pPixelData, 'X', X);
    CopyCharacter(pPixelData, 'Y', Y);
    CopyCharacter(pPixelData, 'Z', Z);
  }

  // Digits
  {
    CopyCharacter(pPixelData, '0', _0);
    CopyCharacter(pPixelData, '1', _1);
    CopyCharacter(pPixelData, '2', _2);
    CopyCharacter(pPixelData, '3', _3);
    CopyCharacter(pPixelData, '4', _4);
    CopyCharacter(pPixelData, '5', _5);
    CopyCharacter(pPixelData, '6', _6);
    CopyCharacter(pPixelData, '7', _7);
    CopyCharacter(pPixelData, '8', _8);
    CopyCharacter(pPixelData, '9', _9);
  }

  // Special Characters
  {
    CopyCharacter(pPixelData, 0, unknown);

    CopyCharacter(pPixelData, ' ', "");

    CopyCharacter(pPixelData, '!', excl);
    CopyCharacter(pPixelData, '\"', dqu);
    CopyCharacter(pPixelData, '#', hash);
    CopyCharacter(pPixelData, '$', dollar);
    CopyCharacter(pPixelData, '%', percent);
    CopyCharacter(pPixelData, '&', andsign);
    CopyCharacter(pPixelData, '\'', squ);
    CopyCharacter(pPixelData, '(', po);
    CopyCharacter(pPixelData, ')', pc);
    CopyCharacter(pPixelData, '*', star);
    CopyCharacter(pPixelData, '+', plus);
    CopyCharacter(pPixelData, ',', comma);
    CopyCharacter(pPixelData, '-', minus);
    CopyCharacter(pPixelData, '.', dot);
    CopyCharacter(pPixelData, '/', slash);

    CopyCharacter(pPixelData, ':', colon);
    CopyCharacter(pPixelData, ';', semicolon);
    CopyCharacter(pPixelData, '<', less);
    CopyCharacter(pPixelData, '=', equal);
    CopyCharacter(pPixelData, '>', more);
    CopyCharacter(pPixelData, '?', question);

    CopyCharacter(pPixelData, '@', at);
    CopyCharacter(pPixelData, '[', abo);
    CopyCharacter(pPixelData, '\\', backslash);
    CopyCharacter(pPixelData, ']', abc);
    CopyCharacter(pPixelData, '^', dach);
    CopyCharacter(pPixelData, '_', underscore);

    CopyCharacter(pPixelData, '`', apostrophe);

    CopyCharacter(pPixelData, '{', cbo);
    CopyCharacter(pPixelData, '|', pipesign);
    CopyCharacter(pPixelData, '}', cbc);
    CopyCharacter(pPixelData, '~', tilde);
  }
}




EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Graphics_Implementation_SimpleASCIIFont);

