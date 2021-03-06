//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2015 Marianne Gagnon
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "config/user_config.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/widgets/kart_stats_widget.hpp"
#include "karts/abstract_characteristic.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "utils/log.hpp"
#include "utils/string_utils.hpp"
#include "io/file_manager.hpp"

#include <IGUIEnvironment.h>
#include <IGUIElement.h>
#include <IGUIButton.h>
#include <string>

using namespace GUIEngine;
using namespace irr::core;
using namespace irr;

// -----------------------------------------------------------------------------
KartStatsWidget::KartStatsWidget(core::recti area, const int player_id,
                                 std::string kart_group, bool multiplayer,
                                 bool display_icons) : Widget(WTYPE_DIV)
{
    m_title_font = !multiplayer;
    m_player_id = player_id;

    const std::string default_kart = UserConfigParams::m_default_kart;
    const KartProperties* props =
        kart_properties_manager->getKart(default_kart);
    if(!props)
    {
        // If the default kart can't be found (e.g. previously a addon
        // kart was used, but the addon package was removed), use the
        // first kart as a default. This way we don't have to hardcode
        // any kart names.
        int id = kart_properties_manager->getKartByGroup(kart_group, 0);
        if (id == -1)
        {
            props = kart_properties_manager->getKartById(0);
        }
        else
        {
            props = kart_properties_manager->getKartById(id);
        }

        if(!props)
            Log::fatal("KartSelectionScreen", "Can't find default "
                       "kart '%s' nor any other kart.",
                       default_kart.c_str());
    }
                                                                                                                                    
    for (int i = 0; i < SKILL_COUNT; ++i)
    {
        irr::core::recti skillArea(0, 0, 1, 1);

        SkillLevelWidget* skill_bar = NULL;

        skill_bar = new SkillLevelWidget(skillArea, m_player_id, multiplayer, display_icons);       

        m_skills.push_back(skill_bar);
        m_children.push_back(skill_bar);
    }

	setValues(props, PLAYER_DIFFICULTY_NORMAL);

    move(area.UpperLeftCorner.X, area.UpperLeftCorner.Y,
         area.getWidth(), area.getHeight());
}   // KartStatsWidget

// -----------------------------------------------------------------------------
void KartStatsWidget::setValues(const KartProperties* props,
                                PerPlayerDifficulty d)
{
    // Use kart properties computed for best difficulty to show the user, so
    // that properties don't change according to the the last used difficulty
    RaceManager::Difficulty previous_difficulty = race_manager->getDifficulty();
    race_manager->setDifficulty(RaceManager::DIFFICULTY_BEST);
    KartProperties kp_computed;
    kp_computed.copyForPlayer(props, d);
    for (SkillLevelWidget* skills : m_skills)
        skills->setVisible(true);

    // Scale the values so they look better
    // A value of 100 takes the whole bar width, including borders.
    // So values should be in the 0-99 range

    // The base mass is of 350 ; 350/3.89 ~= 90
    m_skills[SKILL_MASS]->setValue(
    	kp_computed.getCombinedCharacteristic()->getMass()/3.89f);
    m_skills[SKILL_MASS]->setIcon(irr::core::stringc(
            file_manager->getAsset(FileManager::GUI_ICON, "mass.png").c_str()));    
    m_skills[SKILL_MASS]->m_properties[PROP_ID] = StringUtils::insertValues("@p%i_mass", m_player_id);
    m_skills[SKILL_MASS]->m_iconbutton->setTooltip( _("Mass") );
    
    // The base speed is of 25
    // Here we are not fully proportional, because small differences matter more
    m_skills[SKILL_SPEED]->setValue(
    	(kp_computed.getCombinedCharacteristic()->getEngineMaxSpeed() - 20.0f) * 15.0f);
    m_skills[SKILL_SPEED]->setIcon(irr::core::stringc(
            file_manager->getAsset(FileManager::GUI_ICON, "speed.png").c_str()));    
    m_skills[SKILL_SPEED]->m_properties[PROP_ID] = StringUtils::insertValues("@p%i_speed", m_player_id);
    m_skills[SKILL_SPEED]->m_iconbutton->setTooltip( _("Maximum speed") );
    
    // The acceleration depend on power and mass, and it changes depending on speed
    // We call a function which gives us a single number to represent it
    // power/mass gives numbers in the 1-10 range, so we multiply it by 10.

    m_skills[SKILL_ACCELERATION]->setValue(kp_computed.getAccelerationEfficiency()*10.0f);
    m_skills[SKILL_ACCELERATION]->setIcon(irr::core::stringc(
            file_manager->getAsset(FileManager::GUI_ICON, "power.png").c_str()));    
    m_skills[SKILL_ACCELERATION]->m_properties[PROP_ID] =
        StringUtils::insertValues("@p%i_acceleration", m_player_id);
    m_skills[SKILL_ACCELERATION]->m_iconbutton->setTooltip( _("Acceleration") );

    // The base nitro consumption is 1, higher for heavier karts.
    // Nitro efficiency is hence 90/nitro_consumption

    m_skills[SKILL_NITRO_EFFICIENCY]->setValue(
        90.0f/kp_computed.getCombinedCharacteristic()->getNitroConsumption());
    m_skills[SKILL_NITRO_EFFICIENCY]->setIcon(irr::core::stringc(
            file_manager->getAsset(FileManager::GUI_ICON, "nitro.png").c_str()));    
    m_skills[SKILL_NITRO_EFFICIENCY]->m_properties[PROP_ID] =
        StringUtils::insertValues("@p%i_nitro_efficiency", m_player_id);
    m_skills[SKILL_NITRO_EFFICIENCY]->m_iconbutton->setTooltip( _("Nitro efficiency") );
    
    race_manager->setDifficulty(previous_difficulty);
}   // setValues

// -----------------------------------------------------------------------------
void KartStatsWidget::hideAll()
{
    for (SkillLevelWidget* skills : m_skills)
        skills->setVisible(false);
}   // hideAll

// -----------------------------------------------------------------------------
void KartStatsWidget::add()
{
    for (int i = 0; i < SKILL_COUNT; ++i)
    {
        m_skills[i]->add();
    }
}   // add

// -----------------------------------------------------------------------------
void KartStatsWidget::move(int x, int y, int w, int h)
{
    Widget::move(x,y,w,h);
    setSize(m_x, m_y, m_w, m_h);
    int margin = m_h / SKILL_COUNT - m_skill_bar_h / 2;
    if (margin > m_skill_bar_h)
        margin = m_skill_bar_h;
    int offset = (m_h - (SKILL_COUNT * margin)) / 2;
    for (int i = 0; i < SKILL_COUNT; ++i)
    {
        m_skills[i]->move(m_skill_bar_x,
                          m_y + offset + margin * i,
                          m_skill_bar_w,
                          m_skill_bar_h);
    }
}   // move

// -----------------------------------------------------------------------------
/** Set value for given type
 */
void KartStatsWidget::setValue(Stats type, float value)
{
    m_skills[type]->setValue(value);
}   // setValue

// -----------------------------------------------------------------------------
/** Get value for given type
 */
float KartStatsWidget::getValue(Stats type)
{
    return m_skills[type]->getValue();
}   // getVAlue

// -----------------------------------------------------------------------------
/** Set size for widgets inside KartStatsWidget
 */
void KartStatsWidget::setSize(const int x, const int y, const int w, const int h)
{
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;

    // -- sizes
    m_skill_bar_w = w;
    m_skill_bar_h = (m_title_font ? GUIEngine::getTitleFontHeight() : GUIEngine::getFontHeight());

    // for shrinking effect
    if (h < 175)
    {
        const float factor = h / 175.0f;
        m_skill_bar_h   = (int)(m_skill_bar_h*factor);
    }

    m_skill_bar_x = x;
    m_skill_bar_y = y + h/2 - m_skill_bar_h/2;
}   // setSize

// -----------------------------------------------------------------------------
void KartStatsWidget::setDisplayIcons(bool display_icons)
{
    for (int i = 0; i < SKILL_COUNT; ++i)
    {
        m_skills[i]->setDisplayIcon(display_icons);
    }
}   // setDisplayText
