/*!********************************************************************

Tenacity: A Digital Audio Editor

@file GenericUIAssert.cpp
@brief Implementation for GenericUI::Assert()

Avery King

**********************************************************************/

#include "GenericUIAssert.h"

namespace
{

GenericUI::AssertFn __AssertFn;

}

namespace GenericUI
{

void SetAssertFn(AssertFn fn)
{
    __AssertFn = fn;
}

void Assert(bool condition)
{
    __AssertFn(condition);
}

}
