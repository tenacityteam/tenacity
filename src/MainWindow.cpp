/**********************************************************************

  Saucedacity: A Digital Audio Editor

  MainWindow.cpp

  Avery King

**********************************************************************/

#include "MainWindow.h"
#include "ShuttleGui.h"
#include <wx/button.h>

////////////////////////////////////////////////////////////////////////
// MainWindowComponent Implementations
////////////////////////////////////////////////////////////////////////

bool MainWindowComponent::IsShown()
{
  return mShown;
}

////////////////////////////////////////////////////////////////////////
// MainWindow Implementations
////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow() : wxFrame(nullptr, wxID_ANY, "Saucedacity")
{
}

MainWindow::~MainWindow()
{
}

void MainWindow::AddComponent(MainWindowComponent* component)
{
  mComponents.push_back(component);
}

void MainWindow::DestroyAllComponents()
{
  for (auto& c : mComponents)
  {
    c->Destroy();
    delete c;
  }

  mComponents.clear();
}

void MainWindow::ShowWindow()
{
  for (auto& c : mComponents)
  {
    c->ShowComponent(this);
  }

  Show();
}
