/*
 *      Copyright (C) 2014-2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIControllerList.h"
#include "GUIControllerDefines.h"
#include "GUIControllerWindow.h"
#include "GUIFeatureList.h"
#include "addons/AddonManager.h"
#include "games/controllers/Controller.h"
#include "games/controllers/DefaultController.h"
#include "games/controllers/guicontrols/GUIGameController.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIWindow.h"

#include <assert.h>

using namespace ADDON;
using namespace GAME;

CGUIControllerList::CGUIControllerList(CGUIControllerWindow* window, IFeatureList* featureList) :
  m_guiWindow(window),
  m_featureList(featureList),
  m_controllerList(nullptr),
  m_controllerButton(nullptr),
  m_window(window),
  m_focusedController(-1)
{
  assert(m_featureList != nullptr);
}

bool CGUIControllerList::Initialize(void)
{
  m_controllerList = dynamic_cast<CGUIControlGroupList*>(m_guiWindow->GetControl(CONTROL_CONTROLLER_LIST));
  m_controllerButton = dynamic_cast<CGUIButtonControl*>(m_guiWindow->GetControl(CONTROL_CONTROLLER_BUTTON_TEMPLATE));

  if (m_controllerButton)
    m_controllerButton->SetVisible(false);

  Refresh();

  CAddonMgr::GetInstance().RegisterObserver(this);

  return m_controllerList != nullptr &&
         m_controllerButton != nullptr;
}

void CGUIControllerList::Deinitialize(void)
{
  CAddonMgr::GetInstance().UnregisterObserver(this);

  CleanupButtons();

  m_controllerList = nullptr;
  m_controllerButton = nullptr;
}

void CGUIControllerList::Refresh(void)
{
  CleanupButtons();

  if (!RefreshControllers())
    return;

  if (m_controllerList)
  {
    // Remember which controller is focused
    /* TODO
    std::string strFocusedControllerId;
    if (0 <= m_focusedController && m_focusedController < (int)m_controllers.size())
      strFocusedControllerId = m_controllers[m_focusedController]->ID();
    */

    unsigned int buttonId = 0;
    for (ControllerVector::const_iterator it = m_controllers.begin(); it != m_controllers.end(); ++it)
    {
      const ControllerPtr& controller = *it;

      CGUIButtonControl* pButton = new CGUIButtonControl(*m_controllerButton);
      pButton->SetLabel(controller->Label());
      pButton->SetID(CONTROL_CONTROLLER_BUTTONS_START + buttonId++);
      pButton->SetVisible(true);
      pButton->AllocResources();
      m_controllerList->AddControl(pButton);

      // Just in case
      if (buttonId >= MAX_CONTROLLER_COUNT)
        break;
    }

    // Reselect previous controller
    unsigned int previousController = 0;

    /* TODO
    if (!strFocusedControllerId.empty())
    {
      for (unsigned int i = 0; i < m_controllers.size(); i++)
      {
        if (strFocusedControllerId == m_controllers[i]->ID())
        {
          previousController = i;
          break;
        }
      }
    }
    */

    m_window->FocusController(previousController);
  }
}

void CGUIControllerList::OnFocus(unsigned int controllerIndex)
{
  if (controllerIndex < m_controllers.size())
  {
    const ControllerPtr& controller = m_controllers[controllerIndex];

    if (m_focusedController != (int)controllerIndex)
    {
      m_focusedController = controllerIndex;
      m_featureList->Load(controller);
    }

    // TODO: Activate controller for all game controller controls
    CGUIGameController* pController = dynamic_cast<CGUIGameController*>(m_guiWindow->GetControl(CONTROL_GAME_CONTROLLER));
    if (pController)
      pController->ActivateController(controller);
  }
}

void CGUIControllerList::OnSelect(unsigned int controllerIndex)
{
  if (controllerIndex < m_controllers.size())
  {
    // TODO
  }
}

void CGUIControllerList::Notify(const Observable& obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageAddons)
    Refresh();
}

bool CGUIControllerList::RefreshControllers(void)
{
  bool bChanged = false;

  // Get controller add-ons
  ADDON::VECADDONS addons;
  CAddonMgr::GetInstance().GetAddons(ADDON_GAME_CONTROLLER, addons, true);

  // Convert to controllers
  ControllerVector controllers;
  std::transform(addons.begin(), addons.end(), std::back_inserter(controllers),
    [](const AddonPtr& addon)
    {
      return std::static_pointer_cast<CController>(addon);
    });

  // Look for new controllers
  ControllerVector newControllers;
  for (ControllerVector::const_iterator it = controllers.begin(); it != controllers.end(); ++it)
  {
    const ControllerPtr& controller = *it;

    if (std::find_if(m_controllers.begin(), m_controllers.end(),
      [controller](const ControllerPtr& ctrl)
      {
        return ctrl->ID() == controller->ID();
      }) == m_controllers.end())
    {
      newControllers.push_back(controller);
    }
  }

  // Remove old controllers
  for (ControllerVector::iterator it = m_controllers.begin(); it != m_controllers.end(); /* ++it */)
  {
    ControllerPtr& controller = *it;

    if (std::find_if(controllers.begin(), controllers.end(),
      [controller](const ControllerPtr& ctrl)
      {
        return ctrl->ID() == controller->ID();
      }) == newControllers.end())
    {
      it = m_controllers.erase(it); // Not found, remove it
      bChanged = true;
    }
    else
    {
      ++it;
    }
  }

  // Add new controllers
  for (ControllerVector::iterator it = newControllers.begin(); it != newControllers.end(); ++it)
  {
    ControllerPtr& newController = *it;

    if (newController->LoadLayout())
    {
      m_controllers.push_back(newController);
      bChanged = true;
    }
  }

  // Sort add-ons, with default controller first
  if (bChanged)
  {
    std::sort(m_controllers.begin(), m_controllers.end(),
      [](const ControllerPtr& i, const ControllerPtr& j)
      {
        if (i->ID() == DEFAULT_CONTROLLER_ID && j->ID() != DEFAULT_CONTROLLER_ID) return true;
        if (i->ID() != DEFAULT_CONTROLLER_ID && j->ID() == DEFAULT_CONTROLLER_ID) return false;

        return i->ID() < j->ID();
      });
  }

  return bChanged;
}

void CGUIControllerList::CleanupButtons(void)
{
  if (m_controllerList)
    m_controllerList->ClearAll();
}
