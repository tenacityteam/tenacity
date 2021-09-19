/*!********************************************************************

Saucedacity: A Digital Audio Editor

@file GenericObject.h
@brief GenericObject definition

Avery King

**********************************************************************/
#ifndef GENERIC_OBJECT
#define GENERIC_OBJECT

#include <vector>

namespace GenericUI {

/** \brief The base object of any GenericUI object.
 *
 * The GenericObject class is a base class for every single object in
 * GenericUI. It mostly implements internal things for derived objects.
 *
 * Note that you will not be able to instantiate a GenericObject object itself.
 * The default constructor and destructor are deleted.
 *
 */
class GenericObject
{
  public:
    /// Default constructor.
    GenericObject();

    /// Default destructor.
    ~GenericObject();

    /** Adds a parent object to the current object.
     *
     * This function adds a child object to the current object. Any resources
     * associated with the child object are now bound by the lifetime of the
     * parent.
     *
     * Deriving this member function further should be irrelevant. However,
     * some object types might require more than what the default GenericObject
     * implementation provides. In that case, there might be an overloaded
     * version of this function that you should use instead of this function.
     * In other cases, this function might throw an exception, requiring the
     * use of another (overloaded) function.
     *
     * **Exceptions**
     *
     * This function throws the following exceptions:
     *
     * **std::invalid_argument** - `child` is null.
     *
     * @param child The child object to add as a child.
     * @return Returns `this`.
     *
     */
    virtual GenericObject* AddChild(GenericObject* child, bool is_allocated);

    /** Clears all children from the current object.
     *
     * This function removes all children of the current object. Any child
     * objects are still preserved and are not destroyed by removing them.
     *
     * When removing child objects, each child object is to be immediately
     * disassociated from its parent. From then on, the object is a
     * **standalone object** (an object that manages its own resources). The
     * parent is also no longer a parent itself since it doesn't have any
     * children.
     *
     * @return Returns `this`.
     *
     */
    virtual GenericObject* ClearChildren();

    /** Gets the parent of the current object.
     *
     * This function returns a pointer to the parent of the current object. The
     * returned pointer should not be assumed to be allocated with `new`.
     *
     * @return Returns the parent object.
     *
     */
    virtual GenericObject* GetParent();

    /** Get the object's current internal flags.
     *
     * This function returns the internal flags of the current object. This
     * can be useful if you are trying to set the internal flags of an object
     * from another one.
     *
     * Note that the returned flags should *not* be considered the same across
     * different object types. Not all flags are the same across all objects
     * and should not be assumed so unless explicitly stated.
     *
     * @return Returns the internal flags
     *
     */
    unsigned long GetInternalFlags();

    /** Destructs the object.
     *
     * This function destructs the object. This means any resources associated
     * with the current object are also destroyed. Children of the current
     * object are also destroyed (since DestroyObject() essentially marks the
     * current object end-of-life).
     *
     * Note that behavior of this function varies by class. Except for resource
     * destruction, the aforementioned details are specific to GenericObject.
     * Other objects might copy the functionality as provided by GenericObject
     * while adding their own (appropriate) functionality. For that, please
     * consult that class's documentation.
     *
     * This function acts as the destructor for the object. For the *actual*
     * destructor of the object, it might call this function directly.
     *
     */
    virtual void DestroyObject();

  protected:
    /** Shareed object flags.
     * 
     * These flags are shared flags across derivative classes. This is
     * intended for reuse for different classes, although not all
     * classes might use such objects
     */
    enum class GenericObjectFlags
    {
      IsChildObject = 1 << 0 /// If the object is a parent
    };

    /// Internal C-style struct for keeping track of manually allocated objects.
    struct LinkedObject
    {
      /// GenericObject Pointer
      GenericObject* object = nullptr;

      /// If the object should be deallocated
      bool shouldDeallocate = false;
    };

    /** Internal flags for the current object.
     *
     * This variable represents the object's current internal flags. Note
     * that these flags can mean anything to any object and are not
     * portable across different object types. For example, flags from a
     * GenericWindow object might be interpreted differently by a
     * GenericString object. Additionally, they might represent
     * something else in another derived class. Therefore, universal
     * representation across different classes is not guaranteed.
     *
     */
    unsigned long mFlags = 0;

    /// Internal list of pointers to GenericObjects
    std::vector<LinkedObject> mChildObjects;

    /// Internal pointer
    GenericObject* mParentObject;

    /** \brief Destructs any linked objects.
     *
     * This internal function destructs any linked objects associated with this
     * object. This should typically call obj->DestructObject(), which should be
     * enough for destructing any linked objects.
     *
     */
    virtual void DestroyChildObjects();
};

} // namespace GenericUI

#endif // end GENERIC_OBJECT
