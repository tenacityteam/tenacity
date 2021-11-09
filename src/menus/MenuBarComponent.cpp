/**********************************************************************

  Saucedacity: A Digital Audio Editor

  MenuBarComponent.cpp

  Avery King

**********************************************************************/

#include "MenuBarComponent.h"

MenuBarComponent::~MenuBarComponent()
{
  Destroy();
}

void MenuBarComponent::ShowComponent(MainWindow* window)
{
  window->SetMenuBar(mMenuBar);
}

void MenuBarComponent::InitComponent()
{
  mMenuBar = new wxMenuBar;
  mInitialized = true;
}

void MenuBarComponent::StartMenuEntry(wxString title)
{
  wxASSERT(mInitialized);

  mCurrentMenu = new wxMenu;
  mMenuEntryTitle = title;
}

void MenuBarComponent::EndMenuEntry()
{
  mMenuBar->Append(mCurrentMenu, mMenuEntryTitle);
  mMenuEntryTitle.Empty();

  // GP: This looks like a memory leak, but the wxMenuBar instance that
  // mCurrentMenu was added to should manage the previous instance. It should be
  // safe to do this, overall.
  mCurrentMenu = nullptr;
}

void MenuBarComponent::AddMenuItem(int id, wxString label, wxString helpText, MenuItemType type)
{
  wxASSERT(!mMenuEntryTitle.IsEmpty());
  wxASSERT(mInitialized);


  if (type == MenuItemType::RegularItem)
  {
    mCurrentMenu->Append(id, label, helpText);
  }
  else if (type == MenuItemType::CheckedItem)
  {
    mCurrentMenu->AppendCheckItem(id, label, helpText);
  }
  else if (type == MenuItemType::RadioItem)
  {
    mCurrentMenu->AppendRadioItem(id, label, helpText);
  }
}

void MenuBarComponent::StartSubMenuEntry(wxString submenuTitle)
{
  wxASSERT(mInitialized);
  mSubMenuEntryTitle = submenuTitle;
}

void MenuBarComponent::EndSubMenuEntry()
{
  mSubMenuEntryTitle.Empty();
}

void MenuBarComponent::AddSubMenuItem(int id, wxString label, wxString helpText)
{
  wxASSERT(mInitialized);
  wxMenu* menu = new wxMenu;

  menu->Append(id, label, helpText);
  mCurrentMenu->AppendSubMenu(menu, mSubMenuEntryTitle, helpText);
}

void MenuBarComponent::AddSeparator()
{
  mCurrentMenu->AppendSeparator();
}
