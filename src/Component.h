/**********************************************************************

  Saucedacity: A Digital Audio Editor

  Component.h

  Avery King

**********************************************************************/
#ifndef __SAUCEDACITY_COMPONENT__
#define __SAUCEDACITY_COMPONENT__

#include <memory>

class Component
{
  protected:
    bool mInitizlied = false;
    
  public:

    /// Initializes the component. Purely virtual.
    virtual void InitComponent() = 0;

    /// Destroys the component. Purely virtual.
    virtual void Destroy() = 0;

    /// Returns true for an initialized component, false for an uninitialized
    /// component.
    virtual bool IsInitialized()
    {
      return mInitialized;
    }
};

// Typedefs. None are used right now
using ComponentSharedPtr = std::shared_ptr<Component>;
using ComponentUniquePtr = std::unique_ptr<Component>;

#endif // end __SAUCEDACITY_COMPONENT__
