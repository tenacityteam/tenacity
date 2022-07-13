/*!********************************************************************

Saucedacity: A Digital Audio Editor

@file GenericUIAssert.h
@brief Definition for GenericUI::Assert()

Avery King

**********************************************************************/

#ifndef __SAUCEDACITY_GENERICUI_ASSERT_H__
#define __SAUCEDACITY_GENERICUI_ASSERT_H__

#include <functional>

namespace GenericUI
{

using AssertFn = std::function<void(bool)>;

BASIC_UI_API void SetAssertFn(AssertFn fn);
BASIC_UI_API void Assert(bool condition);

} // namespace GenericUI

#endif // end __SAUCEDACITY_GENERICUI_ASSERT_H__
