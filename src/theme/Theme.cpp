/**********************************************************************

  Audacity: A Digital Audio Editor

  Theme.cpp

  James Crook

  Audacity is free software.
  This file is licensed under the wxWidgets license, see License.txt

********************************************************************//**

\class Theme
\brief Based on ThemeBase, Theme manages image and icon resources.

   Theme is a class which manages theme resources.
   It maps sets of ids to the resources and to names of the resources,
   so that they can be loaded/saved from files.

   Theme adds the Audacity specific images to ThemeBase.

\see \ref Themability

*//*****************************************************************//**

\class ThemeBase
\brief Theme management - Image loading and saving.

   Base for the Theme class. ThemeBase is a generic
   non-Audacity specific class.

\see \ref Themability

*//*****************************************************************/

#include "Theme.h"
#include "ThemeFlags.h"
#include "SourceOutputStream.h"

#include <wx/wxprec.h>
#include <wx/dcclient.h>
#include <wx/image.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/mstream.h>
#include <wx/settings.h>

#include "theme/AllThemeResources.h"  // can remove this later, only needed for 'XPMS_RETIRED'.
#include "ImageManipulation.h"
#include "widgets/AudacityMessageBox.h"

// Tenacity libraries
#include <lib-basic-ui/BasicUI.h>
#include <lib-files/FileNames.h>
#include <lib-preferences/Prefs.h>
#include <lib-strings/Internat.h>
#include <lib-utility/MemoryX.h>

///////////// ImageCache Includes /////////////////////////////////////////////

static const unsigned char LightImageCacheAsData[] = {
#include "LightThemeAsCeeCode.h"
};

static const unsigned char DarkImageCacheAsData[] = {
#include "DarkThemeAsCeeCode.h"
};

static const unsigned char SaucedacityImageCacheAsData[] = {
#include "SaucedacityThemeAsCeeCode.h"
};

static const unsigned char AudacityImageCacheAsData[] = {
#include "AudacityThemeAsCeeCode.h"
};

static const unsigned char AudacityClassicImageCacheAsData[] = {
#include "AudacityClassicThemeAsCeeCode.h"
};

static const unsigned char HiContrastImageCacheAsData[] = {
#include "HiContrastThemeAsCeeCode.h"
};

static const unsigned char ProToolsImageCacheAsData[] = {
#include "ProToolsThemeAsCeeCode.h"
};

// Audacium light themes
static const unsigned char AudaciumLightBlueImageCacheAsData[] = {
#include "AudaciumLightBlueThemeAsCeeCode.h"
};
static const unsigned char AudaciumLightOrangeImageCacheAsData[] = {
#include "AudaciumLightOrangeThemeAsCeeCode.h"
};
static const unsigned char AudaciumLightPinkImageCacheAsData[] = {
#include "AudaciumLightPinkThemeAsCeeCode.h"
};
static const unsigned char AudaciumLightGreenImageCacheAsData[] = {
#include "AudaciumLightGreenThemeAsCeeCode.h"
};
static const unsigned char AudaciumLightPurpleImageCacheAsData[] = {
#include "AudaciumLightPurpleThemeAsCeeCode.h"
};

// Audacium dark themes
static const unsigned char AudaciumDarkBlueImageCacheAsData[] = {
#include "AudaciumDarkBlueThemeAsCeeCode.h"
};
static const unsigned char AudaciumDarkOrangeImageCacheAsData[] = {
#include "AudaciumDarkOrangeThemeAsCeeCode.h"
};
static const unsigned char AudaciumDarkPinkImageCacheAsData[] = {
#include "AudaciumDarkPinkThemeAsCeeCode.h"
};
static const unsigned char AudaciumDarkGreenImageCacheAsData[] = {
#include "AudaciumDarkGreenThemeAsCeeCode.h"
};
static const unsigned char AudaciumDarkPurpleImageCacheAsData[] = {
#include "AudaciumDarkPurpleThemeAsCeeCode.h"
};

///////////////////////////////////////////////////////////////////////////////

// theTheme is a global variable.
TENACITY_DLL_API Theme theTheme;

Theme::Theme(void)
{
   mbInitialised=false;
}

Theme::~Theme(void)
{
}


void Theme::EnsureInitialised()
{
   if (mbInitialised)
   {
      return;
   }
   RegisterImages();
   RegisterColours();
   LoadPreferredTheme();
}

bool ThemeBase::LoadPreferredTheme()
{
// DA: Default themes differ.
   auto theme = GUITheme.Read();

   theTheme.LoadTheme( theTheme.ThemeTypeOfTypeName( theme ) );
   return true;
}

void Theme::RegisterImages()
{
   if( mbInitialised )
      return;
   mbInitialised = true;

// This initialises the variables e.g
// RegisterImage( myFlags, bmpRecordButton, some image, wxT("RecordButton"));
   int myFlags = resFlagPaired;
#define THEME_INITS
#include "theme/AllThemeResources.h"


}


void Theme::RegisterColours()
{
}

ThemeBase::ThemeBase(void)
{
   bRecolourOnLoad = false;
   bIsUsingSystemTextColour = false;
}

ThemeBase::~ThemeBase(void)
{
}

/// This function is called to load the initial Theme images.
/// It does not though cause the GUI to refresh.
void ThemeBase::LoadTheme( teThemeType Theme )
{
   EnsureInitialised();
   const bool cbOkIfNotFound = true;

   if( !ReadImageCache( Theme, cbOkIfNotFound ) )
   {
      // THEN get the default set.
      ReadImageCache( GetFallbackThemeType(), !cbOkIfNotFound );

      // JKC: Now we could go on and load the individual images
      // on top of the default images using the commented out
      // code that follows...
      //
      // However, I think it is better to get the user to
      // build a NEW image cache, which they can do easily
      // from the Theme preferences tab.
#if 0
      // and now add any available component images.
      LoadComponents( cbOkIfNotFound );

      // JKC: I'm usure about doing this next step automatically.
      // Suppose the disk is write protected?
      // Is having the image cache created automatically
      // going to confuse users?  Do we need version specific names?
      // and now save the combined image as a cache for later use.
      // We should load the images a little faster in future as a result.
      CreateImageCache();
#endif
   }

   RotateImageInto( bmpRecordBeside, bmpRecordBelow, false );
   RotateImageInto( bmpRecordBesideDisabled, bmpRecordBelowDisabled, false );

   if( bRecolourOnLoad )
      RecolourTheme();

   wxColor Back        = theTheme.Colour( clrTrackInfo );
   wxColor CurrentText = theTheme.Colour( clrTrackPanelText );
   wxColor DesiredText = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

   int TextColourDifference =  ColourDistance( CurrentText, DesiredText );

   bIsUsingSystemTextColour = ( TextColourDifference == 0 );
   // Theming is very accepting of alternative text colours.  They just need to 
   // have decent contrast to the background colour, if we're blending themes. 
   if( !bIsUsingSystemTextColour ){
      int ContrastLevel        =  ColourDistance( Back, DesiredText );
      bIsUsingSystemTextColour = bRecolourOnLoad && (ContrastLevel > 250);
      if( bIsUsingSystemTextColour )
         Colour( clrTrackPanelText ) = DesiredText;
   }
   bRecolourOnLoad = false;

   // Next line is not required as we haven't yet built the GUI
   // when this function is (or should be) called.
   // ApplyUpdatedImages();
}

void ThemeBase::RecolourBitmap( int iIndex, wxColour From, wxColour To )
{
   wxImage Image( Bitmap( iIndex ).ConvertToImage() );

   std::unique_ptr<wxImage> pResult = ChangeImageColour(
      &Image, From, To );
   ReplaceImage( iIndex, pResult.get() );
}

int ThemeBase::ColourDistance( wxColour & From, wxColour & To ){
   return 
      abs( From.Red() - To.Red() )
      + abs( From.Green() - To.Green() )
      + abs( From.Blue() - To.Blue() );
}

// This function coerces a theme to be more like the system colours.
// Only used for built in themes.  For custom themes a user
// will choose a better theme for them and just not use a mismatching one.
void ThemeBase::RecolourTheme()
{
   wxColour From = Colour( clrMedium );
#if defined( __WXGTK__ )
   wxColour To = wxSystemSettings::GetColour( wxSYS_COLOUR_BACKGROUND );
#else
   wxColour To = wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE );
#endif

   // Don't recolour if difference is too big.
   int d = ColourDistance( From, To );
   if( d  > 120 )
      return;


   Colour( clrMedium ) = To;
   RecolourBitmap( bmpUpButtonLarge, From, To );
   RecolourBitmap( bmpDownButtonLarge, From, To );
   RecolourBitmap( bmpHiliteButtonLarge, From, To );
   RecolourBitmap( bmpUpButtonSmall, From, To );
   RecolourBitmap( bmpDownButtonSmall, From, To );
   RecolourBitmap( bmpHiliteButtonSmall, From, To );

   Colour( clrTrackInfo ) = To;
}

wxImage ThemeBase::MaskedImage( char const ** pXpm, char const ** pMask )
{
   wxBitmap Bmp1( pXpm );
   wxBitmap Bmp2( pMask );

//   wxLogDebug( wxT("Image 1: %i Image 2: %i"),
//      Bmp1.GetDepth(), Bmp2.GetDepth() );

   // We want a 24-bit-depth bitmap if all is working, but on some
   // platforms it might just return -1 (which means best available
   // or not relevant).
   // JKC: \todo check that we're not relying on 24 bit elsewhere.
   wxASSERT( Bmp1.GetDepth()==-1 || Bmp1.GetDepth()==24);
   wxASSERT( Bmp1.GetDepth()==-1 || Bmp2.GetDepth()==24);

   int i,nBytes;
   nBytes = Bmp1.GetWidth() * Bmp1.GetHeight();
   wxImage Img1( Bmp1.ConvertToImage());
   wxImage Img2( Bmp2.ConvertToImage());

//   unsigned char *src = Img1.GetData();
   unsigned char *mk = Img2.GetData();
   //wxImage::setAlpha requires memory allocated with malloc, not NEW
   MallocString<unsigned char> alpha{
      static_cast<unsigned char*>(malloc( nBytes )) };

   // Extract alpha channel from second XPM.
   for(i=0;i<nBytes;i++)
   {
      alpha[i] = mk[0];
      mk+=3;
   }

   Img1.SetAlpha( alpha.release() );

   //dmazzoni: the top line does not work on wxGTK
   //wxBitmap Result( Img1, 32 );
   //wxBitmap Result( Img1 );

   return Img1;
}

// Legacy function to allow use of an XPM where no theme image was defined.
// Bit depth and mask needs review.
// Note that XPMs don't offer translucency, so unsuitable for a round shape overlay, 
// for example.
void ThemeBase::RegisterImage( int &flags, int &iIndex, char const ** pXpm, const wxString & Name )
{
   wxASSERT( iIndex == -1 ); // Don't initialise same bitmap twice!
   wxBitmap Bmp( pXpm );
   wxImage Img( Bmp.ConvertToImage() );
   // The next line recommended by http://forum.audacityteam.org/viewtopic.php?f=50&t=96765
   Img.SetMaskColour(0xDE, 0xDE, 0xDE);
   Img.InitAlpha();

   //dmazzoni: the top line does not work on wxGTK
   //wxBitmap Bmp2( Img, 32 );
   //wxBitmap Bmp2( Img );

   RegisterImage( flags, iIndex, Img, Name );
}

void ThemeBase::RegisterImage( int &flags, int &iIndex, const wxImage &Image, const wxString & Name )
{
   wxASSERT( iIndex == -1 ); // Don't initialise same bitmap twice!
   mImages.push_back( Image );

#ifdef __APPLE__
   // On Mac, bitmaps with alpha don't work.
   // So we convert to a mask and use that.
   // It isn't quite as good, as alpha gives smoother edges.
   //[Does not affect the large control buttons, as for those we do
   // the blending ourselves anyway.]
   wxImage TempImage( Image );
   TempImage.ConvertAlphaToMask();
   mBitmaps.push_back( wxBitmap( TempImage ) );
#else
   mBitmaps.push_back( wxBitmap( Image ) );
#endif

   mBitmapNames.push_back( Name );
   mBitmapFlags.push_back( flags );
   flags &= ~resFlagSkip;
   iIndex = mBitmaps.size() - 1;
}

void ThemeBase::RegisterColour( int &iIndex, const wxColour &Clr, const wxString & Name )
{
   wxASSERT( iIndex == -1 ); // Don't initialise same colour twice!
   mColours.push_back( Clr );
   mColourNames.push_back( Name );
   iIndex = mColours.size() - 1;
}

// Must be wide enough for bmpAudacityLogo. Use double width + 10.
const int ImageCacheWidth = 440;

const int ImageCacheHeight = 836;

void ThemeBase::CreateImageCache( bool bBinarySave )
{
   EnsureInitialised();
   wxBusyCursor busy;

   wxImage ImageCache( ImageCacheWidth, ImageCacheHeight );
   ImageCache.SetRGB( wxRect( 0,0,ImageCacheWidth, ImageCacheHeight), 1,1,1);//Not-quite black.

   // Ensure we have an alpha channel...
   if( !ImageCache.HasAlpha() )
   {
      ImageCache.InitAlpha();
   }

   int i;

   FlowPacker context{ ImageCacheWidth };

//#define IMAGE_MAP
#ifdef IMAGE_MAP
   wxLogDebug( wxT("<img src=\"ImageCache.png\" usemap=\"#map1\">" ));
   wxLogDebug( wxT("<map name=\"map1\">") );
#endif

   // Save the bitmaps
   for(i = 0;i < (int)mImages.size();i++)
   {
      wxImage &SrcImage = mImages[i];
      context.mFlags = mBitmapFlags[i];
      if( (mBitmapFlags[i] & resFlagInternal)==0)
      {
         context.GetNextPosition( SrcImage.GetWidth(), SrcImage.GetHeight());
         ImageCache.SetRGB( context.Rect(), 0xf2, 0xb0, 0x27 );
         if( (context.mFlags & resFlagSkip) == 0 )
            PasteSubImage( &ImageCache, &SrcImage, 
               context.mxPos + context.mBorderWidth,
               context.myPos + context.mBorderWidth);
         else
            ImageCache.SetRGB( context.RectInner(), 1,1,1);
#ifdef IMAGE_MAP
         // No href in html.  Uses title not alt.
         wxRect R( context.Rect() );
         wxLogDebug( wxT("<area title=\"Bitmap:%s\" shape=rect coords=\"%i,%i,%i,%i\">"),
            mBitmapNames[i],
            R.GetLeft(), R.GetTop(), R.GetRight(), R.GetBottom() );
#endif
      }
   }

   // Now save the colours.
   int x,y;

   context.SetColourGroup();
   const int iColSize = 10;
   for(i = 0; i < (int)mColours.size(); i++)
   {
      context.GetNextPosition( iColSize, iColSize );
      wxColour c = mColours[i];
      ImageCache.SetRGB( context.Rect() , 0xf2, 0xb0, 0x27 );
      ImageCache.SetRGB( context.RectInner() , c.Red(), c.Green(), c.Blue() );

      // YUCK!  No function in wxWidgets to set a rectangle of alpha...
      for(x=0;x<iColSize;x++)
      {
         for(y=0;y<iColSize;y++)
         {
            ImageCache.SetAlpha( context.mxPos + x, context.myPos+y, 255);
         }
      }
#ifdef IMAGE_MAP
      // No href in html.  Uses title not alt.
      wxRect R( context.Rect() );
      wxLogDebug( wxT("<area title=\"Colour:%s\" shape=rect coords=\"%i,%i,%i,%i\">"),
         mColourNames[i],
         R.GetLeft(), R.GetTop(), R.GetRight(), R.GetBottom() );
#endif
   }
#if TEST_CARD
   int j;
   for(i=0;i<ImageCacheWidth;i++)
      for(j=0;j<ImageCacheHeight;j++){
         int r = j &0xff;
         int g = i &0xff;
         int b = (j >> 8) | ((i>>4)&0xf0);
         wxRect R( i,j, 1, 1);
         ImageCache.SetRGB( R, r, g, b );
         ImageCache.SetAlpha( i,j, 255);
   }
#endif

#ifdef IMAGE_MAP
   wxLogDebug( "</map>" );
#endif

   // IF bBinarySave, THEN saving to a normal PNG file.
   if( bBinarySave )
   {
      const auto &FileName = FileNames::ThemeCachePng();

      // Perhaps we should prompt the user if they are overwriting
      // an existing theme cache?
#if 0
      if( wxFileExists( FileName ))
      {
         auto message =
//          XO(
//"Theme cache file:\n  %s\nalready exists.\nAre you sure you want to replace it?")
//             .Format( FileName );
            TranslatableString{ FileName };
         AudacityMessageBox( message );
         return;
      }
#endif
#if 0
      // Deliberate policy to use the fast/cheap blocky pixel-multiplication
      // algorithm, as this introduces no artifacts on repeated scale up/down.
      ImageCache.Rescale( 
         ImageCache.GetWidth()*4,
         ImageCache.GetHeight()*4,
         wxIMAGE_QUALITY_NEAREST );
#endif
      if( !ImageCache.SaveFile( FileName, wxBITMAP_TYPE_PNG ))
      {
         AudacityMessageBox(
            XO("Tenacity could not write file:\n  %s.")
               .Format( FileName ));
         return;
      }
      AudacityMessageBox(
/* i18n-hint: A theme is a consistent visual style across an application's
 graphical user interface, including choices of colors, and similarity of images
 such as those on button controls.  Audacity can load and save alternative
 themes. */
         XO("Theme written to:\n  %s.")
            .Format( FileName ));
   }
   // ELSE saving to a C code textual version.
   else
   {
      SourceOutputStream OutStream;
      const auto &FileName = FileNames::ThemeCacheAsCee( );
      if( !OutStream.OpenFile( FileName ))
      {
         AudacityMessageBox(
            XO("Tenacity could not open file:\n  %s\nfor writing.")
               .Format( FileName ));
         return;
      }
      if( !ImageCache.SaveFile(OutStream, wxBITMAP_TYPE_PNG ) )
      {
         AudacityMessageBox(
            XO("Tenacity could not write images to file:\n  %s.")
               .Format( FileName ));
         return;
      }
      AudacityMessageBox(
         /* i18n-hint "Cee" means the C computer programming language */
         XO("Theme as Cee code written to:\n  %s.")
            .Format( FileName ));
   }
}

/// Writes an html file with an image map of the ImageCache
/// Very handy for seeing what each part is for.
void ThemeBase::WriteImageMap( )
{
   EnsureInitialised();
   wxBusyCursor busy;

   int i;
   FlowPacker context{ ImageCacheWidth };

   wxFFile File( FileNames::ThemeCacheHtm(), wxT("wb") );// I'll put in NEW lines explicitly.
   if( !File.IsOpened() )
      return;

   File.Write( wxT("<html>\n"));
   File.Write( wxT("<body bgcolor=\"303030\">\n"));
   wxString Temp = wxString::Format( wxT("<img src=\"ImageCache.png\" width=\"%i\" usemap=\"#map1\">\n" ), ImageCacheWidth );
   File.Write( Temp );
   File.Write( wxT("<map name=\"map1\">\n") );

   for(i = 0; i < (int)mImages.size(); i++)
   {
      wxImage &SrcImage = mImages[i];
      context.mFlags = mBitmapFlags[i];
      if( (mBitmapFlags[i] & resFlagInternal)==0)
      {
         context.GetNextPosition( SrcImage.GetWidth(), SrcImage.GetHeight());
         // No href in html.  Uses title not alt.
         wxRect R( context.RectInner() );
         File.Write( wxString::Format(
            wxT("<area title=\"Bitmap:%s\" shape=rect coords=\"%i,%i,%i,%i\">\n"),
            mBitmapNames[i],
            R.GetLeft(), R.GetTop(), R.GetRight(), R.GetBottom()) );
      }
   }
   // Now save the colours.
   context.SetColourGroup();
   const int iColSize = 10;
   for(i = 0; i < (int)mColours.size(); i++)
   {
      context.GetNextPosition( iColSize, iColSize );
      // No href in html.  Uses title not alt.
      wxRect R( context.RectInner() );
      File.Write( wxString::Format( wxT("<area title=\"Colour:%s\" shape=rect coords=\"%i,%i,%i,%i\">\n"),
         mColourNames[i],
         R.GetLeft(), R.GetTop(), R.GetRight(), R.GetBottom()) );
   }
   File.Write( wxT("</map>\n") );
   File.Write( wxT("</body>\n"));
   File.Write( wxT("</html>\n"));
   // File will be closed automatically.
}

/// Writes a series of Macro definitions that can be used in the include file.
void ThemeBase::WriteImageDefs( )
{
   EnsureInitialised();
   wxBusyCursor busy;

   int i;
   wxFFile File( FileNames::ThemeImageDefsAsCee(), wxT("wb") );
   if( !File.IsOpened() )
      return;
   teResourceFlags PrevFlags = (teResourceFlags)-1;
   for(i = 0; i < (int)mImages.size(); i++)
   {
      wxImage &SrcImage = mImages[i];
      // No href in html.  Uses title not alt.
      if( PrevFlags != mBitmapFlags[i] )
      {
         PrevFlags = (teResourceFlags)mBitmapFlags[i];
         int t = (int)PrevFlags;
         wxString Temp;
         if( t==0 ) Temp = wxT(" resFlagNone ");
         if( t & resFlagPaired )   Temp += wxT(" resFlagPaired ");
         if( t & resFlagCursor )   Temp += wxT(" resFlagCursor ");
         if( t & resFlagNewLine )  Temp += wxT(" resFlagNewLine ");
         if( t & resFlagInternal ) Temp += wxT(" resFlagInternal ");
         Temp.Replace( wxT("  "), wxT(" | ") );

         File.Write( wxString::Format( wxT("\n   SET_THEME_FLAGS( %s );\n"),
            Temp ));
      }
      File.Write( wxString::Format(
         wxT("   DEFINE_IMAGE( bmp%s, wxImage( %i, %i ), wxT(\"%s\"));\n"),
         mBitmapNames[i],
         SrcImage.GetWidth(),
         SrcImage.GetHeight(),
         mBitmapNames[i]
         ));
   }
}


teThemeType ThemeBase::GetFallbackThemeType()
{
   // Fallback must be an internally supported type,
   // to guarantee it is found.
   return themeDark;
}

teThemeType ThemeBase::ThemeTypeOfTypeName( const wxString & Name )
{
   static const wxArrayStringEx aThemes{
      "light",
      "dark",
      "saucedacity",
      "audacity",
      "audacity-classic",
      "pro-tools",
      "audacium-blue",
      "audacium-orange",
      "audacium-pink",
      "audacium-green",
      "audacium-purple",
      "audacium-dark-blue",
      "audacium-dark-orange",
      "audacium-dark-pink",
      "audacium-dark-green",
      "audacium-dark-purple",
      "high-contrast",
      "custom",
   };
   int themeIx = make_iterator_range( aThemes ).index( Name );
   if( themeIx < 0 )
      return GetFallbackThemeType();
   return (teThemeType)themeIx;
}



/// Reads an image cache including images, cursors and colours.
/// @param bBinaryRead if true means read from an external binary file.
///   otherwise the data is taken from a compiled in block of memory.
/// @param bOkIfNotFound if true means do not report absent file.
/// @return true iff we loaded the images.
bool ThemeBase::ReadImageCache( teThemeType type, bool bOkIfNotFound)
{
   EnsureInitialised();
   wxImage ImageCache;
   wxBusyCursor busy;

   // Ensure we have an alpha channel...
//   if( !ImageCache.HasAlpha() )
//   {
//      ImageCache.InitAlpha();
//   }

   bRecolourOnLoad = GUIBlendThemes.Read();

   if(  type == themeFromFile )
   {
      const auto &FileName = FileNames::ThemeCachePng();
      if( !wxFileExists( FileName ))
      {
         if( bOkIfNotFound )
            return false; // did not load the images, so return false.
         AudacityMessageBox(
            XO("Tenacity could not find file:\n  %s.\nTheme not loaded.")
               .Format( FileName ));
         return false;
      }
      if( !ImageCache.LoadFile( FileName, wxBITMAP_TYPE_PNG ))
      {
         AudacityMessageBox(
            /* i18n-hint: Do not translate png.  It is the name of a file format.*/
            XO("Tenacity could not load file:\n  %s.\nBad png format perhaps?")
               .Format( FileName ));
         return false;
      }
   }
   // ELSE we are reading from internal storage.
   else
   {
      size_t ImageSize = 0;
      const unsigned char * pImage = nullptr;
      switch( type )
      {
         default: 
         case themeDark:
            ImageSize = sizeof(DarkImageCacheAsData);
            pImage = DarkImageCacheAsData;
            break;

         case themeLight:
            ImageSize = sizeof(LightImageCacheAsData);
            pImage = LightImageCacheAsData;
            break;

         case themeSaucedacity:
            ImageSize = sizeof(SaucedacityImageCacheAsData);
            pImage = SaucedacityImageCacheAsData;
            break;

         case themeAudacity:
            ImageSize = sizeof(AudacityImageCacheAsData);
            pImage = AudacityImageCacheAsData;
            break;

         case themeAudacityClassic:
            ImageSize = sizeof(AudacityClassicImageCacheAsData);
            pImage = AudacityClassicImageCacheAsData;
            break;

         case themeProTools:
            ImageSize = sizeof(ProToolsImageCacheAsData);
            pImage = ProToolsImageCacheAsData;
            break;

         case themeAudaciumLightBlue:
             ImageSize = sizeof(AudaciumLightBlueImageCacheAsData);
             pImage = AudaciumLightBlueImageCacheAsData;
             break;

         case themeAudaciumLightOrange:
             ImageSize = sizeof(AudaciumLightOrangeImageCacheAsData);
             pImage = AudaciumLightOrangeImageCacheAsData;
             break;

         case themeAudaciumLightPink:
             ImageSize = sizeof(AudaciumLightPinkImageCacheAsData);
             pImage = AudaciumLightPinkImageCacheAsData;
             break;

         case themeAudaciumLightGreen:
             ImageSize = sizeof(AudaciumLightGreenImageCacheAsData);
             pImage = AudaciumLightGreenImageCacheAsData;
             break;

         case themeAudaciumLightPurple:
             ImageSize = sizeof(AudaciumLightPurpleImageCacheAsData);
             pImage = AudaciumLightPurpleImageCacheAsData;
             break;

         case themeAudaciumDarkBlue:
             ImageSize = sizeof(AudaciumDarkBlueImageCacheAsData);
             pImage = AudaciumDarkBlueImageCacheAsData;
             break;

         case themeAudaciumDarkOrange:
             ImageSize = sizeof(AudaciumDarkOrangeImageCacheAsData);
             pImage = AudaciumDarkOrangeImageCacheAsData;
             break;

         case themeAudaciumDarkPink:
             ImageSize = sizeof(AudaciumDarkPinkImageCacheAsData);
             pImage = AudaciumDarkPinkImageCacheAsData;
             break;

         case themeAudaciumDarkGreen:
             ImageSize = sizeof(AudaciumDarkGreenImageCacheAsData);
             pImage = AudaciumDarkGreenImageCacheAsData;
             break;

         case themeAudaciumDarkPurple:
             ImageSize = sizeof(AudaciumDarkGreenImageCacheAsData);
             pImage = AudaciumDarkPurpleImageCacheAsData;
             break;

         case themeHiContrast:
            ImageSize = sizeof(HiContrastImageCacheAsData);
            pImage = HiContrastImageCacheAsData;
            break;
      }
      //wxLogDebug("Reading ImageCache %p size %i", pImage, ImageSize );
      wxMemoryInputStream InternalStream( pImage, ImageSize );

      if( !ImageCache.LoadFile( InternalStream, wxBITMAP_TYPE_PNG ))
      {
         // If we get this message, it means that the data in file
         // was not a valid png image.
         // Most likely someone edited it by mistake,
         // Or some experiment is being tried with NEW formats for it.
         AudacityMessageBox(
            XO(
"Tenacity could not read its default theme.\nPlease report this problem."));
         return false;
      }
      //wxLogDebug("Read %i by %i", ImageCache.GetWidth(), ImageCache.GetHeight() );
   }

   // Resize a large image down.
   if( ImageCache.GetWidth() > ImageCacheWidth ){
      int h = ImageCache.GetHeight() * ((1.0*ImageCacheWidth)/ImageCache.GetWidth());
      ImageCache.Rescale(  ImageCacheWidth, h );
   }
   int i;
   FlowPacker context{ ImageCacheWidth };
   // Load the bitmaps
   for(i = 0; i < (int)mImages.size(); i++)
   {
      wxImage &Image = mImages[i];
      context.mFlags = mBitmapFlags[i];
      if( (mBitmapFlags[i] & resFlagInternal)==0)
      {
         context.GetNextPosition( Image.GetWidth(),Image.GetHeight() );
         wxRect R = context.RectInner();
         //wxLogDebug( "[%i, %i, %i, %i, \"%s\"], ", R.x, R.y, R.width, R.height, mBitmapNames[i].c_str() );
         Image = GetSubImageWithAlpha( ImageCache, context.RectInner() );
         mBitmaps[i] = wxBitmap(Image);
      }
   }
   if( !ImageCache.HasAlpha() )
      ImageCache.InitAlpha();

//   return true; //To not load colours..
   // Now load the colours.
   int x,y;
   context.SetColourGroup();
   wxColour TempColour;
   const int iColSize=10;
   for(i = 0; i < (int)mColours.size(); i++)
   {
      context.GetNextPosition( iColSize, iColSize );
      context.RectMid( x, y );
      wxRect R = context.RectInner();
      //wxLogDebug( "[%i, %i, %i, %i, \"%s\"], ", R.x, R.y, R.width, R.height, mColourNames[i].c_str() );
      // Only change the colour if the alpha is opaque.
      // This allows us to add NEW colours more easily.
      if( ImageCache.GetAlpha(x,y ) > 128 )
      {
         TempColour = wxColour(
            ImageCache.GetRed( x,y),
            ImageCache.GetGreen( x,y),
            ImageCache.GetBlue(x,y));
         /// \todo revisit this hack which makes adding NEW colours easier
         /// but which prevents a colour of (1,1,1) from being added.
         /// find an alternative way to make adding NEW colours easier.
         /// e.g. initialise the background to translucent, perhaps.
         if( TempColour != wxColour(1,1,1) )
            mColours[i] = TempColour;
      }
   }
   return true;
}

void ThemeBase::LoadComponents( bool bOkIfNotFound )
{
   // IF directory doesn't exist THEN return early.
   if( !wxDirExists( FileNames::ThemeComponentsDir() ))
      return;

   wxBusyCursor busy;
   int i;
   int n=0;
   FilePath FileName;
   for(i = 0; i < (int)mImages.size(); i++)
   {

      if( (mBitmapFlags[i] & resFlagInternal)==0)
      {
         FileName = FileNames::ThemeComponent( mBitmapNames[i] );
         if( wxFileExists( FileName ))
         {
            if( !mImages[i].LoadFile( FileName, wxBITMAP_TYPE_PNG ))
            {
               AudacityMessageBox(
                  XO(
               /* i18n-hint: Do not translate png.  It is the name of a file format.*/
"Tenacity could not load file:\n  %s.\nBad png format perhaps?")
                     .Format( FileName ));
               return;
            }
            /// JKC: \bug (wxWidgets) A png may have been saved with alpha, but when you
            /// load it, it comes back with a mask instead!  (well I guess it is more
            /// efficient).  Anyway, we want alpha and not a mask, so we call InitAlpha,
            /// and that transfers the mask into the alpha channel, and we're done.
            if( ! mImages[i].HasAlpha() )
            {
               // wxLogDebug( wxT("File %s lacked alpha"), mBitmapNames[i] );
               mImages[i].InitAlpha();
            }
            mBitmaps[i] = wxBitmap( mImages[i] );
            n++;
         }
      }
   }
   if( n==0 )
   {
      if( bOkIfNotFound )
         return;
      AudacityMessageBox(
         XO(
"None of the expected theme component files\n were found in:\n  %s.")
            .Format( FileNames::ThemeComponentsDir() ));
   }
}

void ThemeBase::SaveComponents()
{
   // IF directory doesn't exist THEN create it
   if( !wxDirExists( FileNames::ThemeComponentsDir() ))
   {
      /// \bug 1 in wxWidgets documentation; wxMkDir returns false if
      /// directory didn't exist, even if it successfully creates it.
      /// so we create and then test if it exists instead.
      /// \bug 2 in wxWidgets documentation; wxMkDir has only one argument
      /// under MSW
#ifdef __WXMSW__
      wxMkDir( FileNames::ThemeComponentsDir().fn_str() );
#else
      wxMkDir( FileNames::ThemeComponentsDir().fn_str(), 0700 );
#endif
      if( !wxDirExists( FileNames::ThemeComponentsDir() ))
      {
         AudacityMessageBox(
            XO("Could not create directory:\n  %s")
               .Format( FileNames::ThemeComponentsDir() ) );
         return;
      }
   }

   wxBusyCursor busy;
   int i;
   int n=0;
   FilePath FileName;
   for(i = 0; i < (int)mImages.size(); i++)
   {
      if( (mBitmapFlags[i] & resFlagInternal)==0)
      {
         FileName = FileNames::ThemeComponent( mBitmapNames[i] );
         if( wxFileExists( FileName ))
         {
            ++n;
            break;
         }
      }
   }

   using namespace GenericUI;

   if (n > 0)
   {
      auto result =
         ShowMessageBox(
            XO(
"Some required files in:\n  %s\nwere already present. Overwrite?")
               .Format( FileNames::ThemeComponentsDir() ),
            MessageBoxOptions{}
               .ButtonStyle(Button::YesNo)
               .DefaultIsNo());
      if (result == MessageBoxResult::No)
         return;
   }

   for(i = 0; i < (int)mImages.size(); i++)
   {
      if( (mBitmapFlags[i] & resFlagInternal)==0)
      {
         FileName = FileNames::ThemeComponent( mBitmapNames[i] );
         if( !mImages[i].SaveFile( FileName, wxBITMAP_TYPE_PNG ))
         {
            AudacityMessageBox(
               XO("Tenacity could not save file:\n  %s")
                  .Format( FileName ));
            return;
         }
      }
   }
   AudacityMessageBox(
      XO("Theme written to:\n  %s.")
         .Format( FileNames::ThemeComponentsDir() ) );
}


void ThemeBase::SaveThemeAsCode()
{
   // false indicates not using standard binary method.
   CreateImageCache( false );
}

wxImage ThemeBase::MakeImageWithAlpha( wxBitmap & Bmp )
{
   // BUG in wxWidgets.  Conversion from BMP to image does not preserve alpha.
   wxImage image( Bmp.ConvertToImage() );
   return image;
}

wxColour & ThemeBase::Colour( int iIndex )
{
   wxASSERT( iIndex >= 0 );
   EnsureInitialised();
   return mColours[iIndex];
}

void ThemeBase::SetBrushColour( wxBrush & Brush, int iIndex )
{
   wxASSERT( iIndex >= 0 );
   Brush.SetColour( Colour( iIndex ));
}

void ThemeBase::SetPenColour(   wxPen & Pen, int iIndex )
{
   wxASSERT( iIndex >= 0 );
   Pen.SetColour( Colour( iIndex ));
}

wxBitmap & ThemeBase::Bitmap( int iIndex )
{
   wxASSERT( iIndex >= 0 );
   EnsureInitialised();
   return mBitmaps[iIndex];
}

wxImage  & ThemeBase::Image( int iIndex )
{
   wxASSERT( iIndex >= 0 );
   EnsureInitialised();
   return mImages[iIndex];
}
wxSize  ThemeBase::ImageSize( int iIndex )
{
   wxASSERT( iIndex >= 0 );
   EnsureInitialised();
   wxImage & Image = mImages[iIndex];
   return wxSize( Image.GetWidth(), Image.GetHeight());
}

/// Replaces both the image and the bitmap.
void ThemeBase::ReplaceImage( int iIndex, wxImage * pImage )
{
   Image( iIndex ) = *pImage;
   Bitmap( iIndex ) = wxBitmap( *pImage );
}

void ThemeBase::RotateImageInto( int iTo, int iFrom, bool bClockwise )
{
   wxImage img(theTheme.Bitmap( iFrom ).ConvertToImage() );
   wxImage img2 = img.Rotate90( bClockwise );
   ReplaceImage( iTo, &img2 );
}

/** @brief Default Theme.
 * 
 * Options:
 * 
 * 1 - Default
 * 2 - Dark
 * 
 **/
constexpr int defaultTheme = 1;

ChoiceSetting GUITheme{
   wxT("/GUI/Theme"),
   {
      ByColumns,
      {
         XO("Light"),

         XO("Dark"),

         /* i18n-hint: describing the appearance of Tenacity 1.2 */
         XO("Saucedacity"),

         /* i18n-hint: describing the appearance of Audacity 3.0.4's default theme */
         XO("Audacity"),

         /* i18n-hint: describing the "classic" or traditional
            appearance of older versions of Audacity (before Tenacity) */
         XO("Audacity Classic"),

         // i18n-hint: A Pro Tools lookalike theme
         XO("Pro Tools"),

         // i18n-hint: A light theme with blue audio waveforms, taken from Audacium.
         XO("Audacium Light Blue"),

         //i18n-hint: A light theme with orange audio waveforms, taken from Audacium.
         XO("Audacium Light Orange"),

         // i18n-hint: A light theme with pink audio waveforms, taken from Audacium.
         XO("Audacium Light Pink"),

         // i18n-hint: A light theme with green audio waveforms, taken from Audacium.
         XO("Audacium Light Green"),

         // i18n-hint: A light theme with purple audio waveforms, taken from Audacium.
         XO("Audacium Light Purple"),

         // i18n-hint: A light theme with blue audio waveforms, taken from Audacium.
         XO("Audacium Dark Blue"),

         // i18n-hint: A dark theme with orange audio waveforms, taken from Audacium.
         XO("Audacium Dark Orange"),

         // i18n-hint: A dark theme with pink audio waveforms, taken from Audacium.
         XO("Audacium Dark Pink"),

         // i18n-hint: A dark theme with green audio waveforms, taken from Audacium.
         XO("Audacium Dark Green"),

         // i18n-hint: A dark theme with purple audio waveforms, taken from Audacium.
         XO("Audacium Dark Purple"),

         /* i18n-hint: greater difference between foreground and
            background colors */
         XO("High Contrast"),

         /* i18n-hint: user defined */
         XO("Custom"),
      },
      {
         wxT("light"),
         wxT("dark"),
         wxT("saucedacity"),
         wxT("audacity"),
         wxT("audacity-classic"),
         wxT("pro-tools"),
         wxT("audacium-blue"),
         wxT("audacium-orange"),
         wxT("audacium-pink"),
         wxT("audacium-green"),
         wxT("audacium-purple"),
         wxT("audacium-dark-blue"),
         wxT("audacium-dark-orange"),
         wxT("audacium-dark-pink"),
         wxT("audacium-dark-green"),
         wxT("audacium-dark-purple"),
         wxT("high-contrast"),
         wxT("custom"),
      }
   },
   defaultTheme
};

BoolSetting GUIBlendThemes{ wxT("/GUI/BlendThemes"), true };

