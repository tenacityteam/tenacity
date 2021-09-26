/**********************************************************************

  Tenacity: A Digital Audio Editor

  FlowPacker.h

  Avery King split from Theme.h

  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#pragma once

#include <wx/gdicmn.h>

#include "ThemeFlags.h"

/** @brief Packs rectangular boxes into a rectangle, using simple first fit.
 *
 * This class is currently used by Theme to pack its images into the image
 * cache.  Perhaps someday we will improve FlowPacker and make it more flexible,
 * and use it for toolbar and window layouts too.
 *
 **/
//! A cursor for iterating the theme bitmap
class TENACITY_DLL_API FlowPacker
{
    public:
        explicit FlowPacker(int width);
        ~FlowPacker() {}
        void GetNextPosition( int xSize, int ySize );
        void SetNewGroup( int iGroupSize );
        void SetColourGroup( );
        wxRect Rect();
        wxRect RectInner();
        void RectMid( int &x, int &y );

        // These 4 should become private again...
        int mFlags = resFlagPaired;
        int mxPos = 0;
        int myPos = 0;
        int myHeight = 0;
        int mBorderWidth = 1;

    private:
        int iImageGroupSize = 1;
        int iImageGroupIndex = -1;
        int mOldFlags = resFlagPaired;
        int myPosBase = 0;
        int mxCacheWidth = 0;

        int mComponentWidth = 0;
        int mComponentHeight = 0;
};
