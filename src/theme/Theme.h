/**********************************************************************

  Audacity: A Digital Audio Editor

  Theme.h

  James Crook

  Audacity is free software.
  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#ifndef __AUDACITY_THEME__
#define __AUDACITY_THEME__

#include <vector>
#include <wx/defs.h>
#include <wx/window.h> // to inherit
#include "ComponentInterfaceSymbol.h"

#include "ThemeFlags.h"
#include "FlowPacker.h"

// Tenacity libraries
#include <lib-preferences/Prefs.h>

//! A choice of theme such as "Light", "Dark", ...
using teThemeType = Identifier;

class wxArrayString;
class wxBitmap;
class wxColour;
class wxImage;
class wxPen;

class ChoiceSetting;

class TENACITY_DLL_API ThemeBase /* not final */
{
public:
   ThemeBase(void);
   ThemeBase ( const ThemeBase & ) = delete;
   ThemeBase &operator =( const ThemeBase & ) = delete;
public:
   virtual ~ThemeBase(void);

public:
   virtual void EnsureInitialised()=0;

   // Typically statically constructed:
   struct TENACITY_DLL_API RegisteredTheme {
      using InfoPair = std::pair<
         const std::vector<unsigned char>&, // A reference to this vector is stored, not a copy of it!
         bool
      >;
      RegisteredTheme(EnumValueSymbol symbol, InfoPair info);
      ~RegisteredTheme();
      const EnumValueSymbol symbol;
   };

   void LoadTheme( teThemeType Theme );
   void RegisterImage( int &flags, int &iIndex,char const** pXpm, const wxString & Name);
   void RegisterImage( int &flags, int &iIndex, const wxImage &Image, const wxString & Name );
   void RegisterColour( int &iIndex, const wxColour &Clr, const wxString & Name );

   teThemeType GetFallbackThemeType();
   void CreateImageCache(bool bBinarySave = true);
   bool ReadImageCache( teThemeType type = {}, bool bOkIfNotFound=false);
   void LoadComponents( bool bOkIfNotFound =false);
   void SaveComponents();
   void SaveThemeAsCode();
   void WriteImageDefs( );
   void WriteImageMap( );
   static bool LoadPreferredTheme();
   bool IsUsingSystemTextColour(){ return bIsUsingSystemTextColour; }
   void RecolourBitmap( int iIndex, wxColour From, wxColour To );
   void RecolourTheme();

   int ColourDistance( wxColour & From, wxColour & To );
   wxColour & Colour( int iIndex );
   wxBitmap & Bitmap( int iIndex );
   wxImage  & Image( int iIndex );
   wxSize ImageSize( int iIndex );
   bool bRecolourOnLoad;  // Request to recolour.
   bool bIsUsingSystemTextColour;

   void ReplaceImage( int iIndex, wxImage * pImage );
   void RotateImageInto( int iTo, int iFrom, bool bClockwise );

   void SetBrushColour( wxBrush & Brush, int iIndex );
   void SetPenColour(   wxPen & Pen, int iIndex );

   // Utility function that combines a bitmap and a mask, both in XPM format.
   wxImage MaskedImage( char const ** pXpm, char const ** pMask );
   // Utility function that takes a 32 bit bitmap and makes it into an image.
   wxImage MakeImageWithAlpha( wxBitmap & Bmp );

protected:
   // wxImage, wxBitmap copy cheaply using reference counting
   std::vector<wxImage> mImages;
   std::vector<wxBitmap> mBitmaps;
   wxArrayString mBitmapNames;
   std::vector<int> mBitmapFlags;

   std::vector<wxColour> mColours;
   wxArrayString mColourNames;
};


class TENACITY_DLL_API Theme final : public ThemeBase
{
public:
   Theme(void);
public:
   ~Theme(void);
public:
   void EnsureInitialised() override;
   void RegisterImages();
   void RegisterColours();
   bool mbInitialised;
};

// A bit cheeky - putting a themable wxStaticText control into
// theme, rather than in a new file.  Saves sorting out makefiles (for now).
class wxWindow;
class wxString;
class wxPaintEvent;

extern TENACITY_DLL_API Theme theTheme;

extern TENACITY_DLL_API BoolSetting
     GUIBlendThemes
;

extern TENACITY_DLL_API ChoiceSetting &GUITheme();

#endif // __AUDACITY_THEME__
