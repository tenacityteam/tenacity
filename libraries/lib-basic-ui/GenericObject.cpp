/*!********************************************************************

Saucedacity: A Digital Audio Editor

@file GenericObject.cpp
@brief GenericObject declarations

Avery King

**********************************************************************/

#include "GenericObject.h"
#include <stdexcept>

namespace GenericUI
{

//// GenericObject Implementations ////////////////////////////////////////////

// Constructors and Destructors
GenericObject::GenericObject()
{
}

GenericObject::~GenericObject()
{
  DestroyObject();
}

// Public member functions

GenericObject& GenericObject::AddChild(GenericObject* child, bool is_allocated)
{
  LinkedObject linked_obj;

  if (child != nullptr)
  {
    linked_obj.object = child;
    linked_obj.shouldDeallocate = is_allocated;
    child->mParentObject = this;
    child->mFlags |= IsChildObject;
    mChildObjects.push_back(linked_obj);
  }

  return *this;
}

GenericObject& GenericObject::ClearChildren()
{
  mChildObjects.clear();
  return *this;
}

void GenericObject::DestroyObject()
{
  ClearChildren();
  mChildObjects.clear();
  mFlags = 0;
}

GenericObject* GenericObject::GetParent()
{
  return mParentObject;
}

// Protected member functions
void GenericObject::DestroyChildObjects()
{
  for (auto obj : mChildObjects)
  {
    obj.object->DestroyObject();
    if (obj.shouldDeallocate)
    {
      delete obj.object;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

} // namespace GenericUI
