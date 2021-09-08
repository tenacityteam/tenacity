/*!********************************************************************

Saucedacity: A Digital Audio Editor

@file GenericObject.cpp
@brief GenericObject declarations

Avery King

**********************************************************************/

#include "GenericObject.h"

namespace GenericUI
{

//// GenericObject Implementations ////////////////////////////////////////////

// Constructors and Destructors
GenericObject::GenericObject(GenericObject* g_obj, bool is_allocated)
{
  AddParent(g_obj, is_allocated);
}

GenericObject::GenericObject(GenericObject& g_obj)
{
  mFlags        = g_obj.mFlags;
  mChildObjects = g_obj.mChildObjects;
  mParentObject = g_obj.mParentObject;
}

GenericObject::GenericObject(GenericObject&& g_obj)
{
  mFlags        = g_obj.mFlags;
  mChildObjects = g_obj.mChildObjects;
  mParentObject = g_obj.mParentObject;

  g_obj.mFlags = 0;
  g_obj.mChildObjects.clear();
  g_obj.mParent Object.reset();
}

GenericObject::~GenericObject()
{
  DestroyLinkedObjects();
}

// Public member functions

GenericObject* GenericObject::AddParent(GenericObject* g_obj, bool is_allocated)
{
  LinkedObject linked_obj;
  if (g_obj != nullptr)
  {
    linked_obj.object = g_obj;
    linked_obj.shouldDeallocate = is_allocated;
    g_obj->mParentObject = std::make_unique<GenericObject>(this);
    mChildObjects.push_back(linked_obj);
  }

  return this;
}

GenericObject* GenericObject::RemoveChildren()
{
  mChildObjects.clear();
  return this;
}

// Protected member functions
void GenericObject::DestroyLinkedObjects()
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

virtual void GenericObject::DestroyObject()
{
  DestroyChildObjects();
  mChildObjects.clear();
  mFlags = 0;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace GenericUI
