/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#pragma once

#include "DRMUtils.h"
#include "GLContextEGL.h"

class CDRMAtomic : public CDRMUtils
{
public:
  static void FlipPage(CGLContextEGL *pGLContext);
  static bool SetVideoMode(RESOLUTION_INFO res);
  static bool InitDrmAtomic(drm *drm, gbm *gbm);
  static void DestroyDrmAtomic();

private:
  static bool AddConnectorProperty(drmModeAtomicReq *req, int obj_id, const char *name, int value);
  static bool AddCrtcProperty(drmModeAtomicReq *req, int obj_id, const char *name, int value);
  static bool AddPlaneProperty(drmModeAtomicReq *req, int obj_id, const char *name, int value);
  static bool DrmAtomicCommit(int fb_id, int flags);
  static int GetPlaneId();
};
