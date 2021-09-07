/*!********************************************************************

Saucedacity: A Digital Audio Editor

@file GenericObject.h
@brief GenericObject definition

Avery King

**********************************************************************/
#ifndef GENERIC_OBJECT
#define GENERIC_OBJECT

#include <vector>
#include <memory>

namespace GenericUI {

/** \brief The base object of any GenericUI object.
 *
 * The GenericObject class is a base class for all the
 *
 */
class GenericObject
{
  public:
    /// Default constructor
    GenericObject() = default;

    /// Constructor to add child objects
    GenericObject(GenericObject* g_obj, bool is_allocated = true);

    /// Default virtual destructor
    virtual ~GenericObject();

    /** \brief Adds a child object, making the current object a parent.
     *
     * This function adds an object to the current object, making that object
     * a current object a child object and the current object a parent object.
     *
     * When an object becomes a child object, its parent object is given
     * responsibility for destroying any resources owned by the child object.
     * Interally, this is done through calling the object's internal member
     * function `DestroyObject()`, of which the child object must destroy all
     * its resources.
     *
     * The child object should not get destroyed before the parent object does.
     * If it does, undefined behavior occurs at that point.
     *
     */
    virtual GenericObject* AddParent(GenericObject* g_obj, bool is_allocated = true);

    /** \brief Removes the object's children.
     *
     * This function removes every single children
     *
     */
    virtual GenericObject* RemoveChildren();

  protected:
    /// Internal C-style struct for keeping track of manually allocated objects.
    struct LinkedObject
    {
      /// GenericObject Pointer
      GenericObject* object = nullptr;

      /// If the object should be deallocated
      bool shouldDeallocate = false;
    };

    //// Internal flags for the class // unique_ptr for easier mamagement
    //enum class InternalFlags
    //{
    //  IsParentObject = 1 << 0
    //};

    /// Internal object flags
    unsigned short mFlags;

    /** \brief Destructs the object.
     *
     * This internal function destructs the object. In derived objects, they
     * should have the exact same behavior as the object's destructor. In fact,
     * they might even call DestructObject() directly.
     *
     * By default (i.e. in GenericObject), this function does nothing. The only
     * reason why this is defined is to make GenericObject instantiable for any
     * plausible reason (more or less).
     *
     */
    virtual void DestroyObject()
    {
    }

    /** \brief Destructs any linked objects.
     *
     * This internal function destructs any linked objects associated with this
     * object. This should typically call obj->DestructObject(), which should be
     * enough for destructing any linked objects.
     *
     */
    virtual void DestroyLinkedObjects();

    /// Internal list of pointers to GenericObjects
    std::vector<LinkedObject> mChildObjects;

    /// Internal pointer
    std::unique_ptr<GenericObject> mParentObject;
};

} // namespace GenericUI

#endif // end GENERIC_OBJECT
