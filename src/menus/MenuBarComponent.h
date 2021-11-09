/**********************************************************************

  Saucedacity: A Digital Audio Editor

  MenuBarComponent.h

  Avery King

**********************************************************************/
#ifndef __SAUCEDACITY_MENUBARCOMPONENT__
#define __SAUCEDACITY_MENUBARCOMPONENT__

#include "../MainWindow.h"

#include <functional>
#include <vector>

#include <wx/string.h>
#include <wx/menu.h>

// class abbreviation prefix: Mbc
enum class MenuItemType
{
  RegularItem,
  RadioItem,
  CheckedItem
};

class MenuBarComponent final : public MainWindowComponent
{
  private:
    wxMenu*    mCurrentMenu = nullptr;
    wxMenuBar* mMenuBar     = nullptr;
    wxString mMenuEntryTitle, mSubMenuEntryTitle;


  public:
    ~MenuBarComponent();
    /** \brief Displays the main menu bar on the main window.
      *
      * This function shows the main menu bar in Saucedacity's main window.
      *
      * Note that this function automatically gets called by MainWindow when
      * MainWindow's Show() method gets called.
      *
      * @param window The window to be shown on. This is usually a global
      * instance of MainWindow representing Saucedacity's main window.
      *
      **/
    void ShowComponent(MainWindow* window) override;

    /** Initializes the component.
     * 
     * Note that a call to this function is required before being able to do
     * anything with a MenuBarComponent object. If this is not called prior to
     * using this object (e.g. calling StartMenuEntry()), 
     * 
     **/
    void InitComponent() override;
    void Destroy() override {}

    /** \brief Starts a menu entry.
     * 
     * This function starts a menu entry in the menubar. To end a menu
     * entry, call EndMenuEntry().
     * 
     * @param title The title for the menu entry.
     * @see EndMenuEntry()
     * 
     **/
    void StartMenuEntry(wxString title);

    /** \brief Ends a menu entry.
     * 
     * This function ends a menu entry previously started with
     * StartMenuEntry().
     * 
     * @see StartMenuEntry().
     * 
     */
    void EndMenuEntry();

    /** \brief Creates a menu entry in the menubar.
      *
      * This function creates a new menu in Saucedacity's menu bar. To create a
      * new submenu, call one of the AddSubMenuItem() member functions,
      * respectively.
      *
      * @param id The ID for the created menu.
      * @param label The label for the menu item
      * @param type The type of menu item to be created (e.g. checked menu
      * item, radio menu item)
      *
      * @see StartSubMenuItem()
      **/
    void AddMenuItem(int id, wxString label, wxString helpString = wxEmptyString, MenuItemType type = MenuItemType::RegularItem);

    /** \brief Creates a new submenu entry.
      *
      * This function creates a new submenu entry from the created menu item. To
      * end a submenu entry, call EndSubMenuEntry().
      * 
      * @see EndSubMenuEntry()
      *
      */
    void StartSubMenuEntry(wxString submenuTitle);

    /** \brief Ends a submenu entry.
     * 
     * This functino ends a submenu entry from the created menu item previously
     * started with StartSubMenuEntry().
     * 
     * @see StartSubMenuEntry()
     **/
    void EndSubMenuEntry();

    /** \brief Creates a submenu entry.
     * 
     * Before adding a submenu, you must start a submenu entry with
     * StartSubMenuEntry() and end it with EndSubMenuEntry() for correct use of
     * this function.
     * 
     */
    void AddSubMenuItem(int id, wxString label, wxString helpText = wxEmptyString);

    void AddSeparator();
};

#endif // end __SAUCEDACITY_MENUBARCOMPONENT__
