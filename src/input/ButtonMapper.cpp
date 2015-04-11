/*
 *      Copyright (C) 2015 Team XBMC
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

#include "ButtonMapper.h"
#include "LibretroDefinitions.h"
#include "LibretroDLL.h"
#include "LibretroEnvironment.h"
#include "LibretroTranslator.h"
#include "libretro.h"

#include "kodi/libXBMC_addon.h"
#include "tinyxml.h"

#define BUTTONMAP_XML  "buttonmap.xml"

using namespace LIBRETRO;

CButtonMapper::CButtonMapper(void)
  : m_bLoadAttempted(false),
    m_buttonMapXml(NULL),
    m_addon(NULL)
{
}

CButtonMapper& CButtonMapper::Get(void)
{
  static CButtonMapper _instance;
  return _instance;
}

bool CButtonMapper::LoadButtonMap(void)
{
  if (!m_bLoadAttempted)
  {
    m_bLoadAttempted = true;
    m_addon = CLibretroEnvironment::Get().GetXBMC();

    CLibretroDLL* client = CLibretroEnvironment::Get().GetClient();

    if (client && m_addon)
    {
      bool bSuccess = false;

      std::string strFilename = client->GetLibraryDirectory() + "/" + BUTTONMAP_XML;

      m_addon->Log(ADDON::LOG_INFO, "Loading libretro buttonmap %s", strFilename.c_str());

      m_buttonMapXml = new TiXmlDocument;
      if (m_buttonMapXml->LoadFile(strFilename))
      {
        TiXmlElement* pRootElement = m_buttonMapXml->RootElement();
        if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueStr() != BUTTONMAP_XML_ROOT)
          m_addon->Log(ADDON::LOG_ERROR, "Can't find root <%s> tag", BUTTONMAP_XML_ROOT);
        else
          bSuccess = true;
      }

      if (!bSuccess)
      {
        delete m_buttonMapXml;
        m_buttonMapXml = NULL;
      }
    }
  }

  return m_buttonMapXml != NULL;
}

libretro_device_t CButtonMapper::GetLibretroType(const std::string& strDeviceId)
{
  // Handle default controller
  if (strDeviceId == "game.controller.default")
    return RETRO_DEVICE_ANALOG;

  // Check buttonmap for other controllers
  const TiXmlElement* pDeviceNode = GetDeviceNode(strDeviceId);
  if (pDeviceNode)
  {
    const char* type = pDeviceNode->Attribute(BUTTONMAP_XML_ATTR_DEVICE_TYPE);
    if (!type)
    {
      m_addon->Log(ADDON::LOG_ERROR, "<%s> tag has no \"%s\" attribute", BUTTONMAP_XML_ELM_DEVICE, BUTTONMAP_XML_ATTR_DEVICE_TYPE);
    }
    else
    {
      return LibretroTranslator::GetDeviceType(type);
    }
  }

  return RETRO_DEVICE_NONE;
}

int CButtonMapper::GetLibretroIndex(const std::string& strDeviceId, const std::string& strFeatureName)
{
  // Handle default controller
  if (strDeviceId == "game.controller.default")
  {
    if (strFeatureName == "a")            return RETRO_DEVICE_ID_JOYPAD_A;
    if (strFeatureName == "b")            return RETRO_DEVICE_ID_JOYPAD_B;
    if (strFeatureName == "x")            return RETRO_DEVICE_ID_JOYPAD_X;
    if (strFeatureName == "y")            return RETRO_DEVICE_ID_JOYPAD_Y;
    if (strFeatureName == "start")        return RETRO_DEVICE_ID_JOYPAD_START;
    if (strFeatureName == "back")         return RETRO_DEVICE_ID_JOYPAD_SELECT;
    if (strFeatureName == "leftbumber")   return RETRO_DEVICE_ID_JOYPAD_L2;
    if (strFeatureName == "rightbumper")  return RETRO_DEVICE_ID_JOYPAD_R2;
    if (strFeatureName == "leftthumb")    return RETRO_DEVICE_ID_JOYPAD_L;
    if (strFeatureName == "rightthumb")   return RETRO_DEVICE_ID_JOYPAD_R;
    if (strFeatureName == "up")           return RETRO_DEVICE_ID_JOYPAD_UP;
    if (strFeatureName == "down")         return RETRO_DEVICE_ID_JOYPAD_DOWN;
    if (strFeatureName == "right")        return RETRO_DEVICE_ID_JOYPAD_RIGHT;
    if (strFeatureName == "left")         return RETRO_DEVICE_ID_JOYPAD_LEFT;
    if (strFeatureName == "lefttrigger")  return RETRO_DEVICE_ID_JOYPAD_L3;
    if (strFeatureName == "righttrigger") return RETRO_DEVICE_ID_JOYPAD_R3;
    if (strFeatureName == "leftstick")    return RETRO_DEVICE_INDEX_ANALOG_LEFT;
    if (strFeatureName == "rightstick")   return RETRO_DEVICE_INDEX_ANALOG_RIGHT;
  }

  // Check buttonmap for other controllers
  const TiXmlElement* pFeatureNode = GetFeatureNode(strDeviceId, strFeatureName);
  if (pFeatureNode)
  {
    const char* mapto = pFeatureNode->Attribute(BUTTONMAP_XML_ATTR_FEATURE_MAPTO);
    if (!mapto)
    {
      m_addon->Log(ADDON::LOG_ERROR, "<%s> tag has no \"%s\" attribute", BUTTONMAP_XML_ELM_FEATURE, BUTTONMAP_XML_ATTR_FEATURE_MAPTO);
    }
    else
    {
      return LibretroTranslator::GetFeatureIndex(mapto);
    }
  }

  return -1;
}

const TiXmlElement* CButtonMapper::GetDeviceNode(const std::string& strDeviceId)
{
  const TiXmlElement* pDeviceNode = NULL;

  if (LoadButtonMap())
  {
    TiXmlElement* pRootElement = m_buttonMapXml->RootElement();

    const TiXmlElement* pChild = pRootElement->FirstChildElement(BUTTONMAP_XML_ELM_DEVICE);

    if (!pChild)
      m_addon->Log(ADDON::LOG_ERROR, "Can't find <%s> tag", BUTTONMAP_XML_ELM_DEVICE);

    while (pChild)
    {
      const char* deviceId = pChild->Attribute(BUTTONMAP_XML_ATTR_DEVICE_ID);
      if (!deviceId)
      {
        m_addon->Log(ADDON::LOG_ERROR, "<%s> tag has no \"%s\" attribute", BUTTONMAP_XML_ELM_DEVICE, BUTTONMAP_XML_ATTR_DEVICE_ID);
        break;
      }

      if (strDeviceId == deviceId)
      {
        pDeviceNode = pChild;
        break;
      }

      pChild = pChild->NextSiblingElement(BUTTONMAP_XML_ELM_DEVICE);
    }

    if (!pDeviceNode)
      m_addon->Log(ADDON::LOG_ERROR, "Can't find <%s> tag for device \"%s\"", BUTTONMAP_XML_ELM_DEVICE, strDeviceId.c_str());
  }

  return pDeviceNode;
}

const TiXmlElement* CButtonMapper::GetFeatureNode(const std::string& strDeviceId, const std::string& strFeatureName)
{
  const TiXmlElement* pFeatureNode = NULL;

  const TiXmlElement* pDeviceNode = GetDeviceNode(strDeviceId);

  if (pDeviceNode)
  {
    const TiXmlElement* pChild = pDeviceNode->FirstChildElement(BUTTONMAP_XML_ELM_FEATURE);

    if (!pChild)
      m_addon->Log(ADDON::LOG_ERROR, "Can't find <%s> tag for device \"%s\"", BUTTONMAP_XML_ELM_FEATURE, strDeviceId.c_str());

    while (pChild)
    {
      const char* name = pChild->Attribute(BUTTONMAP_XML_ATTR_FEATURE_NAME);
      if (!name)
      {
        m_addon->Log(ADDON::LOG_ERROR, "<%s> tag has no \"%s\" attribute", BUTTONMAP_XML_ELM_FEATURE, BUTTONMAP_XML_ATTR_FEATURE_NAME);
        break;
      }

      if (strFeatureName == name)
      {
        pFeatureNode = pChild;
        break;
      }

      pChild = pChild->NextSiblingElement(BUTTONMAP_XML_ELM_FEATURE);
    }

    if (!pFeatureNode)
      m_addon->Log(ADDON::LOG_ERROR, "Can't find feature \"%s\" for device \"%s\"", strFeatureName.c_str(), strDeviceId.c_str());
  }

  return pFeatureNode;
}
