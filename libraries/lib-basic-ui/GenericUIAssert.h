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

void SetAssertFn(AssertFn fn);
void Assert(bool condition);

} // namespace GenericUI

#endif // end __SAUCEDACITY_GENERICUI_ASSERT_H__
