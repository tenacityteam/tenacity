/**********************************************************************

  Saucedacity: A Digital Audio Editor

  FlowPacker.cpp

  Avery King split from Theme.cpp

  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#include "FlowPacker.h"
#include "ThemeFlags.h"

void FlowPacker::Init(int width)
{
   mFlags = resFlagPaired;
   mOldFlags = mFlags;
   mxCacheWidth = width;

   myPos = 0;
   myPosBase =0;
   myHeight = 0;
   iImageGroupSize = 1;
   SetNewGroup(1);
   mBorderWidth = 0;
}

void FlowPacker::SetNewGroup( int iGroupSize )
{
   myPosBase +=myHeight * iImageGroupSize;
   mxPos =0;
   mOldFlags = mFlags;
   iImageGroupSize = iGroupSize;
   iImageGroupIndex = -1;
   mComponentWidth=0;
}

void FlowPacker::SetColourGroup( )
{
   myPosBase = 750;
   mxPos =0;
   mOldFlags = mFlags;
   iImageGroupSize = 1;
   iImageGroupIndex = -1;
   mComponentWidth=0;
   myHeight = 11;
}

void FlowPacker::GetNextPosition( int xSize, int ySize )
{
   xSize += 2*mBorderWidth;
   ySize += 2*mBorderWidth;
   // if the height has increased, then we are on a NEW group.
   if(( ySize > myHeight )||(((mFlags ^ mOldFlags )& ~resFlagSkip)!=0))
   {
      SetNewGroup( ((mFlags & resFlagPaired)!=0) ? 2 : 1 );
      myHeight = ySize;
//      mFlags &= ~resFlagNewLine;
//      mOldFlags = mFlags;
   }

   iImageGroupIndex++;
   if( iImageGroupIndex == iImageGroupSize )
   {
      iImageGroupIndex = 0;
      mxPos += mComponentWidth;
   }

   if(mxPos > (mxCacheWidth - xSize ))
   {
      SetNewGroup(iImageGroupSize);
      iImageGroupIndex++;
      myHeight = ySize;
   }
   myPos = myPosBase + iImageGroupIndex * myHeight;

   mComponentWidth = xSize;
   mComponentHeight = ySize;
}

wxRect FlowPacker::Rect()
{
   return wxRect( mxPos, myPos, mComponentWidth, mComponentHeight);
}

wxRect FlowPacker::RectInner()
{
   return Rect().Deflate( mBorderWidth, mBorderWidth );
}

void FlowPacker::RectMid( int &x, int &y )
{
   x = mxPos + mComponentWidth/2;
   y = myPos + mComponentHeight/2;
}
