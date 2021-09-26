/**********************************************************************

  Audacity: A Digital Audio Editor

  Theme.h

  Avery King split from Theme.cpp

  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#pragma once

// JKC: will probably change name from 'teBmps' to 'tIndexBmp';
using teBmps = int; /// The index of a bitmap resource in Theme Resources.

enum teResourceType
{
   resTypeColour,
   resTypeBitmap,
   resTypeImage = resTypeBitmap
};

enum teResourceFlags
{
   resFlagNone   =0x00,
   resFlagPaired =0x01,
   resFlagCursor =0x02,
   resFlagNewLine = 0x04,
   resFlagInternal = 0x08,  // For image manipulation.  Don't save or load.
   resFlagSkip = 0x10
};
