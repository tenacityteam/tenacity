/**********************************************************************

  Tenacity: A Digital Audio Editor

  FlowPacker.h

  Avery King split from Theme.h

  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#pragma once

#include <wx/gdicmn.h>

/** @brief Packs rectangular boxes into a rectangle, using simple first fit.
 *
 * This class is currently used by Theme to pack its images into the image
 * cache.  Perhaps someday we will improve FlowPacker and make it more flexible,
 * and use it for toolbar and window layouts too.
 *
 **/
class TENACITY_DLL_API FlowPacker
{
    public:
        FlowPacker() {}
        ~FlowPacker() {}

        void Init(int width);
        void GetNextPosition( int xSize, int ySize );
        void SetNewGroup( int iGroupSize );
        void SetColourGroup( );
        wxRect Rect();
        wxRect RectInner();
        void RectMid( int &x, int &y );

        // These 4 should become private again...
        int mFlags;
        int mxPos;
        int myPos;
        int myHeight;
        int mBorderWidth;

    private:
        int iImageGroupSize;
        int iImageGroupIndex;
        int mOldFlags;
        int myPosBase;
        int mxWidth;
        int mxCacheWidth;

        int mComponentWidth;
        int mComponentHeight;
};
