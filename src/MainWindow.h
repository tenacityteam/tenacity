/**********************************************************************

  Saucedacity: A Digital Audio Editor

  MainWindow.h

  Avery King

  This is the main source file for Saucedacity's main window and its
  components (represented by MainWindowComponent).

**********************************************************************/
#ifndef __SAUCEDACITY_MAIN_WINDOW__
#define __SAUCEDACITY_MAIN_WINDOW__

#include <vector>
#include <memory>

#include <wx/frame.h>
#include <wx/sizer.h>

#include "Component.h"

class MainWindowComponent;

/// MainWindowComponent shared pointer (typedef)
using MwcSharedPtr = std::shared_ptr<MainWindowComponent>;

/** \brief Class representing Saucedacity's main window.
 *
 * This class represents the entire main window of Saucedacity, which contains
 * tracks, toolbars, a menubar, and the likes.
 *
 **/
class MainWindow final : public wxFrame
{
  private:
    std::vector<MainWindowComponent*> mComponents;

  public:
    /// Constructor. Initializes everything for the main window
    MainWindow();

    /// Default destructor
    ~MainWindow();

    /** Adds a main window component.
     * 
     * This function adds the main window component `component` to the main
     * window. `comopnent` should be allocated with `new` as it will be deleted
     * when the main window closes (around when Saucedacity exits).
     * 
     **/
    void AddComponent(MainWindowComponent* component);

    /** Destroys all added window components
     *
     * This function destroys all added window components added through
     * AddComponent(). This function does not update the main window in
     * return and should only be called if the main window is being
     * destroyed.
     *
     **/
    void DestroyAllComponents();

    /** Shows the main window.
      *
      * This should be the function to call when the main window is to be
      * shown. Calling this function also calls its components' ShowComponent()
      * method, ensuring that all added components also get shown and
      * initialized.
      *
      **/
    void ShowWindow();
};

/** \brief Class to describe a window component.
 *
 * This class is intended to describe a window component to be added to
 * Saucedacity's main window. Each component is treated differently and
 * consists of a separate type.
 *
 * This class is meant to be derived from. MainWindowComponent lacks the
 * appropriate functions to actually define something onscreen (e.g. a
 * wxHtmlWindow). Additionally, a derived class **must** overload the
 * Destroy() function in order to properly destroy its resources when a
 * component gets destroyed by the main window (MainWindow).
 *
 **/
class MainWindowComponent : public Component
{
  protected:
    bool mShown; /// indicates of the component is shown on the main window

  public:
    /// Displays the component on the main window.
    virtual void ShowComponent(MainWindow* window) = 0;

    /// Returns true if the component is displayed on the main window (from
    /// calling ShowComponent()), false if the component is not.
    bool IsShown();
};

#endif // end __SAUCEDACITY_MAIN_WINDOW__
