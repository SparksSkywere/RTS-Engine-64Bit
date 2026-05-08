/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: OptionsMenu.cpp //////////////////////////////////////////////////////////////////////////
// Author: Colin Day, October 2001
// Description: options menu window callbacks
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "GameSpy/ghttp/ghttp.h"

#include "Common/AudioAffect.h"
#include "Common/AudioSettings.h"
#include "Common/GameAudio.h"
#include "Common/GameEngine.h"
#include "Common/UserPreferences.h"
#include "Common/GameLOD.h"
#include "Common/Registry.h"
#include "Common/Version.h"

#include "GameClient/GameClient.h"
#include "GameClient/InGameUI.h"
#include "GameClient/WindowLayout.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetCheckBox.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/GadgetTextEntry.h"
#include "GameClient/GadgetComboBox.h"
#include "GameClient/GadgetRadioButton.h"
#include "GameClient/GadgetSlider.h"
#include "GameClient/HeaderTemplate.h"
#include "GameClient/Shell.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/Mouse.h"
#include "GameClient/GameText.h"
#include "GameClient/Display.h"
#include "GameClient/GameFont.h"
#include "GameClient/IMEManager.h"
#include "GameClient/ShellHooks.h"
#include "GameClient/GUICallbacks.h"
#include "GameNetwork/FirewallHelper.h"
#include "GameNetwork/IPEnumeration.h"
#include "GameNetwork/GameSpyOverlay.h"
#include "GameNetwork/GameSpy/PeerDefs.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/ScriptEngine.h"
#include "WWDownload/Registry.h"
//added by saad
//used to access a messagebox that does "ok" and "cancel"
#include "GameClient/MessageBox.h"

// This is for non-RC builds only!!!
#define VERBOSE_VERSION L"Release"

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif


static NameKeyType		comboBoxOnlineIPID	= NAMEKEY_INVALID;
static GameWindow *		comboBoxOnlineIP		= NULL;

static NameKeyType		comboBoxLANIPID	= NAMEKEY_INVALID;
static GameWindow *		comboBoxLANIP		= NULL;

static NameKeyType    comboBoxAntiAliasingID   = NAMEKEY_INVALID;
static GameWindow *   comboBoxAntiAliasing     = NULL;

static NameKeyType    comboBoxResolutionID      = NAMEKEY_INVALID;
static GameWindow *   comboBoxResolution       = NULL; 

static NameKeyType    comboBoxDetailID      = NAMEKEY_INVALID;
static GameWindow *   comboBoxDetail        = NULL; 

static NameKeyType		checkAlternateMouseID	= NAMEKEY_INVALID;
static GameWindow *		checkAlternateMouse		= NULL;

static NameKeyType		checkRetaliationID	= NAMEKEY_INVALID;
static GameWindow *		checkRetaliation		= NULL;

static NameKeyType		checkDoubleClickAttackMoveID	= NAMEKEY_INVALID;
static GameWindow *		checkDoubleClickAttackMove		= NULL;

static NameKeyType		sliderScrollSpeedID	= NAMEKEY_INVALID;
static GameWindow *		sliderScrollSpeed		= NULL;

static NameKeyType    checkLanguageFilterID = NAMEKEY_INVALID;
static GameWindow *   checkLanguageFilter   = NULL;

static NameKeyType		checkUseCameraID		= NAMEKEY_INVALID;
static GameWindow *		checkUseCamera			= NULL;

static NameKeyType		checkSaveCameraID		= NAMEKEY_INVALID;
static GameWindow *		checkSaveCamera			= NULL;

static NameKeyType		checkSendDelayID		= NAMEKEY_INVALID;
static GameWindow *		checkSendDelay			= NULL;

static NameKeyType		checkDrawAnchorID		= NAMEKEY_INVALID;
static GameWindow *		checkDrawAnchor			= NULL;

static NameKeyType		checkMoveAnchorID		= NAMEKEY_INVALID;
static GameWindow *		checkMoveAnchor			= NULL;

static NameKeyType		buttonFirewallRefreshID	= NAMEKEY_INVALID;
static GameWindow *		buttonFirewallRefresh		= NULL;
//
//static NameKeyType    checkAudioHardwareID = NAMEKEY_INVALID;
//static GameWindow *   checkAudioHardware   = NULL;
//
//static NameKeyType    checkAudioSurroundID = NAMEKEY_INVALID;
//static GameWindow *   checkAudioSurround   = NULL;
////volume controls
//
static NameKeyType    sliderMusicVolumeID = NAMEKEY_INVALID;
static GameWindow *   sliderMusicVolume   = NULL;

static NameKeyType    sliderSFXVolumeID = NAMEKEY_INVALID;
static GameWindow *   sliderSFXVolume   = NULL;

static NameKeyType    sliderVoiceVolumeID = NAMEKEY_INVALID;
static GameWindow *   sliderVoiceVolume   = NULL;

static NameKeyType    sliderMasterVolumeID = NAMEKEY_INVALID;
static GameWindow *   sliderMasterVolume   = NULL;

static NameKeyType    sliderGammaID = NAMEKEY_INVALID;
static GameWindow *   sliderGamma = NULL;

// Tab panels
static NameKeyType    tabPanelAudioID           = NAMEKEY_INVALID;
static GameWindow *   tabPanelAudio             = NULL;
static NameKeyType    tabPanelVideoID           = NAMEKEY_INVALID;
static GameWindow *   tabPanelVideo             = NULL;
static NameKeyType    tabPanelDisplayID         = NAMEKEY_INVALID;
static GameWindow *   tabPanelDisplay           = NULL;
static NameKeyType    tabPanelAccessibilityID   = NAMEKEY_INVALID;
static GameWindow *   tabPanelAccessibility     = NULL;
static NameKeyType    tabPanelKeyMappingID      = NAMEKEY_INVALID;
static GameWindow *   tabPanelKeyMapping        = NULL;
static NameKeyType    tabPanelNetworkID         = NAMEKEY_INVALID;
static GameWindow *   tabPanelNetwork           = NULL;
static Int            activeTab                 = 0;

// Panel scroll bar
static NameKeyType    panelScrollBarID          = NAMEKEY_INVALID;
static GameWindow *   panelScrollBar            = NULL;
static Int            s_currentScrollPx         = 0;  ///< current scroll offset in screen pixels for the active tab
static Int            s_maxScrollPx             = 0;  ///< max scroll range in pixels for the active tab

static NameKeyType    sliderTextureResolutionID = NAMEKEY_INVALID;
static GameWindow *   sliderTextureResolution = NULL;

static NameKeyType    sliderParticleCapID = NAMEKEY_INVALID;
static GameWindow *   sliderParticleCap = NULL;

static NameKeyType    check3DShadowsID = NAMEKEY_INVALID;
static GameWindow *   check3DShadows   = NULL;

static NameKeyType    check2DShadowsID = NAMEKEY_INVALID;
static GameWindow *   check2DShadows   = NULL;

static NameKeyType    checkCloudShadowsID = NAMEKEY_INVALID;
static GameWindow *   checkCloudShadows   = NULL;

static NameKeyType    checkGroundLightingID = NAMEKEY_INVALID;
static GameWindow *   checkGroundLighting   = NULL;

static NameKeyType    checkSmoothWaterID = NAMEKEY_INVALID;
static GameWindow *   checkSmoothWater   = NULL;

static NameKeyType    checkBuildingOcclusionID = NAMEKEY_INVALID;
static GameWindow *   checkBuildingOcclusion   = NULL;

static NameKeyType    checkPropsID = NAMEKEY_INVALID;
static GameWindow *   checkProps   = NULL;

static NameKeyType    checkExtraAnimationsID = NAMEKEY_INVALID;
static GameWindow *   checkExtraAnimations   = NULL;

static NameKeyType    checkNoDynamicLodID = NAMEKEY_INVALID;
static GameWindow *   checkNoDynamicLod   = NULL;

static NameKeyType    checkVSyncID = NAMEKEY_INVALID;
static GameWindow *   checkVSync   = NULL;

static NameKeyType    checkFPSCounterID = NAMEKEY_INVALID;
static GameWindow *   checkFPSCounter   = NULL;

static NameKeyType    checkFPSLimitID = NAMEKEY_INVALID;
static GameWindow *   checkFPSLimit   = NULL;

static NameKeyType    comboBoxUIScaleID = NAMEKEY_INVALID;
static GameWindow *   comboBoxUIScale   = NULL;

static NameKeyType    comboBoxVoiceFrequencyID = NAMEKEY_INVALID;
static GameWindow *   comboBoxVoiceFrequency   = NULL;

static NameKeyType    comboBoxSpeakerTypeID = NAMEKEY_INVALID;
static GameWindow *   comboBoxSpeakerType   = NULL;

static NameKeyType    checkHeatEffectsID = NAMEKEY_INVALID;
static GameWindow *   checkHeatEffects   = NULL;

static NameKeyType    comboBoxTextureQualityID = NAMEKEY_INVALID;
static GameWindow *   comboBoxTextureQuality   = NULL;

static NameKeyType    comboBoxParticleQualityID = NAMEKEY_INVALID;
static GameWindow *   comboBoxParticleQuality   = NULL;

static NameKeyType    comboBoxUIThemeColorID = NAMEKEY_INVALID;
static GameWindow *   comboBoxUIThemeColor   = NULL;

/*

static NameKeyType    radioHighID = NAMEKEY_INVALID;
static GameWindow *   radioHigh   = NULL;
static NameKeyType    radioMediumID = NAMEKEY_INVALID;
static GameWindow *   radioMedium   = NULL;
static NameKeyType    radioLowID = NAMEKEY_INVALID;
static GameWindow *   radioLow   = NULL;

*/

//Added By Saad for the resolution confirmation dialog box
DisplaySettings oldDispSettings, newDispSettings;
Bool dispChanged = FALSE;
extern Int timer;
extern void DoResolutionDialog();
//

static Bool ignoreSelected = FALSE;
WindowLayout *OptionsLayout = NULL;

enum Detail
{
	HIGHDETAIL = 0,
	MEDIUMDETAIL,
	LOWDETAIL,
	CUSTOMDETAIL,

	DETAIL,
};


OptionPreferences::OptionPreferences( void )
{
	// note, the superclass will put this in the right dir automatically, this is just a leaf name
	load("Options.ini");
}

OptionPreferences::~OptionPreferences()
{
}


Int OptionPreferences::getCampaignDifficulty(void)
{
	OptionPreferences::const_iterator it = find("CampaignDifficulty");
	if (it == end())
		return TheScriptEngine->getGlobalDifficulty();

	Int factor = atoi(it->second.str());
	if (factor < DIFFICULTY_EASY)
		factor = DIFFICULTY_EASY;
	if (factor > DIFFICULTY_HARD)
		factor = DIFFICULTY_HARD;
	
	return factor;
}

void OptionPreferences::setCampaignDifficulty( Int diff )
{
	AsciiString prefString;
	prefString.format("%d", diff );
	(*this)["CampaignDifficulty"] = prefString;
}

Int OptionPreferences::getDisplayMode(void)
{
	OptionPreferences::const_iterator it = find("DisplayMode");
	if (it == end())
		return 0;  // default: Fullscreen
	Int mode = atoi(it->second.str());
	if (mode < 0 || mode > 2)
		mode = 0;
	return mode;
}

Bool OptionPreferences::getVSync(void)
{
	OptionPreferences::const_iterator it = find("VSync");
	if (it == end())
		return TRUE;  // default: VSync on
	return (atoi(it->second.str()) != 0) ? TRUE : FALSE;
}

void OptionPreferences::setVSync(Bool vsync)
{
	(*this)["VSync"] = vsync ? "1" : "0";
}

Bool OptionPreferences::getFPSCounter(void)
{
	OptionPreferences::const_iterator it = find("ShowFPSCounter");
	if (it == end())
		return FALSE;  // default: FPS counter off
	return (atoi(it->second.str()) != 0) ? TRUE : FALSE;
}

void OptionPreferences::setFPSCounter(Bool show)
{
	(*this)["ShowFPSCounter"] = show ? "1" : "0";
}

UnsignedInt OptionPreferences::getLANIPAddress(void)
{
	AsciiString selectedIP = (*this)["IPAddress"];
	IPEnumeration IPs;
	EnumeratedIP *IPlist = IPs.getAddresses();
	while (IPlist)
	{
		if (selectedIP.compareNoCase(IPlist->getIPstring()) == 0)
		{
			return IPlist->getIP();
		}
		IPlist = IPlist->getNext();
	}
	return TheGlobalData->m_defaultIP;
}

void OptionPreferences::setLANIPAddress( AsciiString IP )
{
	(*this)["IPAddress"] = IP;
}

void OptionPreferences::setLANIPAddress( UnsignedInt IP )
{
	AsciiString tmp;
	tmp.format("%d.%d.%d.%d", ((IP & 0xff000000) >> 24), ((IP & 0xff0000) >> 16), ((IP & 0xff00) >> 8), (IP & 0xff));
	(*this)["IPAddress"] = tmp;
}

UnsignedInt OptionPreferences::getOnlineIPAddress(void)
{
	AsciiString selectedIP = (*this)["GameSpyIPAddress"];
	IPEnumeration IPs;
	EnumeratedIP *IPlist = IPs.getAddresses();
	while (IPlist)
	{
		if (selectedIP.compareNoCase(IPlist->getIPstring()) == 0)
		{
			return IPlist->getIP();
		}
		IPlist = IPlist->getNext();
	}
	return TheGlobalData->m_defaultIP;
}

void OptionPreferences::setOnlineIPAddress( AsciiString IP )
{
	(*this)["GameSpyIPAddress"] = IP;
}

void OptionPreferences::setOnlineIPAddress( UnsignedInt IP )
{
	AsciiString tmp;
	tmp.format("%d.%d.%d.%d", ((IP & 0xff000000) >> 24), ((IP & 0xff0000) >> 16), ((IP & 0xff00) >> 8), (IP & 0xff));
	(*this)["GameSpyIPAddress"] = tmp;
}

Bool OptionPreferences::getAlternateMouseModeEnabled(void)
{
	OptionPreferences::const_iterator it = find("UseAlternateMouse");
	if (it == end())
		return TheGlobalData->m_useAlternateMouse;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getRetaliationModeEnabled(void)
{
	OptionPreferences::const_iterator it = find("Retaliation");
	if (it == end())
		return TheGlobalData->m_clientRetaliationModeEnabled;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getDoubleClickAttackMoveEnabled(void)
{
	OptionPreferences::const_iterator it = find("UseDoubleClickAttackMove");
	if( it == end() )
		return TheGlobalData->m_doubleClickAttackMove;

	if( stricmp( it->second.str(), "yes" ) == 0 )
		return TRUE;

	return FALSE;
}

Real OptionPreferences::getScrollFactor(void)
{
	OptionPreferences::const_iterator it = find("ScrollFactor");
	if (it == end())
		return TheGlobalData->m_keyboardDefaultScrollFactor;

	Int factor = atoi(it->second.str());
	if (factor < 0)
		factor = 0;
	if (factor > 100)
		factor = 100;
	
	return factor/100.0f;
}

Bool OptionPreferences::usesSystemMapDir(void)
{
	OptionPreferences::const_iterator it = find("UseSystemMapDir");
	if (it == end())
		return TRUE;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::saveCameraInReplays(void)
{
	OptionPreferences::const_iterator it = find("SaveCameraInReplays");
	if (it == end())
		return TRUE;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::useCameraInReplays(void)
{
	OptionPreferences::const_iterator it = find("UseCameraInReplays");
	if (it == end())
		return TRUE;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Int OptionPreferences::getIdealStaticGameDetail(void)
{
	OptionPreferences::const_iterator it = find("IdealStaticGameLOD");
	if (it == end())
		return STATIC_GAME_LOD_UNKNOWN;

	return TheGameLODManager->getStaticGameLODIndex(it->second);
}

Int OptionPreferences::getStaticGameDetail(void)
{
	OptionPreferences::const_iterator it = find("StaticGameLOD");
	if (it == end())
		return TheGameLODManager->getStaticLODLevel();

	return TheGameLODManager->getStaticGameLODIndex(it->second);
}

Bool OptionPreferences::getSendDelay(void)
{
	OptionPreferences::const_iterator it = find("SendDelay");
	if (it == end())
		return TheGlobalData->m_firewallSendDelay;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Int OptionPreferences::getFirewallBehavior()
{
	OptionPreferences::const_iterator it = find("FirewallBehavior");
	if (it == end())
		return TheGlobalData->m_firewallBehavior;

	Int behavior = atoi(it->second.str());
	if (behavior < 0)
	{
		behavior = 0;
	}
	return behavior;
}

Short OptionPreferences::getFirewallPortAllocationDelta()
{
	OptionPreferences::const_iterator it = find("FirewallPortAllocationDelta");
	if (it == end()) {
		return TheGlobalData->m_firewallPortAllocationDelta;
	}

	Short delta = atoi(it->second.str());
	return delta;
}

UnsignedShort OptionPreferences::getFirewallPortOverride()
{
	OptionPreferences::const_iterator it = find("FirewallPortOverride");
	if (it == end()) {
		return TheGlobalData->m_firewallPortOverride;
	}

	Int override = atoi(it->second.str());
	if (override < 0 || override > 65535)
		override = 0;
	return override;
}

Bool OptionPreferences::getFirewallNeedToRefresh()
{
	OptionPreferences::const_iterator it = find("FirewallNeedToRefresh");
	if (it == end()) {
		return FALSE;
	}

	Bool retval = FALSE;
	AsciiString str = it->second;
	if (str.compareNoCase("TRUE") == 0) {
		retval = TRUE;
	}
	return retval;
}

AsciiString OptionPreferences::getPreferred3DProvider(void)
{
	OptionPreferences::const_iterator it = find("3DAudioProvider");
	if (it == end())
		return TheAudio->getAudioSettings()->m_preferred3DProvider[MAX_HW_PROVIDERS];
	return it->second;
}

AsciiString OptionPreferences::getSpeakerType(void)
{
	OptionPreferences::const_iterator it = find("SpeakerType");
	if (it == end())
		return TheAudio->translateUnsignedIntToSpeakerType(TheAudio->getAudioSettings()->m_defaultSpeakerType2D);
	return it->second;
}

Real OptionPreferences::getSoundVolume(void)
{
	OptionPreferences::const_iterator it = find("SFXVolume");
	if (it == end())
	{
		Real relative = TheAudio->getAudioSettings()->m_relative2DVolume;
		if( relative < 0 )
		{
			Real scale = 1.0f + relative;
			return TheAudio->getAudioSettings()->m_defaultSoundVolume * 100.0f * scale;
		}
		return TheAudio->getAudioSettings()->m_defaultSoundVolume * 100.0f;
	}

	Real volume = (Real) atof(it->second.str());
	if (volume < 0.0f)
	{
		volume = 0.0f;
	}
	return volume;
}

Real OptionPreferences::get3DSoundVolume(void)
{
	OptionPreferences::const_iterator it = find("SFX3DVolume");
	if (it == end())
	{
		Real relative = TheAudio->getAudioSettings()->m_relative2DVolume;
		if( relative > 0 )
		{
			Real scale = 1.0f - relative;
			return TheAudio->getAudioSettings()->m_default3DSoundVolume * 100.0f * scale;
		}
		return TheAudio->getAudioSettings()->m_default3DSoundVolume * 100.0f;
	}

	Real volume = (Real) atof(it->second.str());
	if (volume < 0.0f)
	{
		volume = 0.0f;
	}
	return volume;
}

Real OptionPreferences::getSpeechVolume(void)
{
	OptionPreferences::const_iterator it = find("VoiceVolume");
	if (it == end())
		return TheAudio->getAudioSettings()->m_defaultSpeechVolume * 100.0f;

	Real volume = (Real) atof(it->second.str());
	if (volume < 0.0f)
	{
		volume = 0.0f;
	}
	return volume;
}

Bool OptionPreferences::getCloudShadowsEnabled(void)
{
	OptionPreferences::const_iterator it = find("UseCloudMap");
	if (it == end())
		return TheGlobalData->m_useCloudMap;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getLightmapEnabled(void)
{
	OptionPreferences::const_iterator it = find("UseLightMap");
	if (it == end())
		return TheGlobalData->m_useLightMap;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getSmoothWaterEnabled(void)
{
	OptionPreferences::const_iterator it = find("ShowSoftWaterEdge");
	if (it == end())
		return TheGlobalData->m_showSoftWaterEdge;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getTreesEnabled(void)
{
	OptionPreferences::const_iterator it = find("ShowTrees");
	if (it == end())
		return TheGlobalData->m_useTrees;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getExtraAnimationsDisabled(void)
{
	OptionPreferences::const_iterator it = find("ExtraAnimations");
	if (it == end())
		return TheGlobalData->m_useDrawModuleLOD;

	if (stricmp(it->second.str(), "yes") == 0) {
		return FALSE;	//we are enabling extra animations, so disabled LOD
	}
	return TRUE;
}

Bool OptionPreferences::getUseHeatEffects(void)
{
	OptionPreferences::const_iterator it = find("HeatEffects");
	if (it == end())
		return TheGlobalData->m_useHeatEffects;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getDynamicLODEnabled(void)
{
	OptionPreferences::const_iterator it = find("DynamicLOD");
	if (it == end())
		return TheGlobalData->m_enableDynamicLOD;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getFPSLimitEnabled(void)
{
	OptionPreferences::const_iterator it = find("FPSLimit");
	if (it == end())
		return TheGlobalData->m_useFpsLimit;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::get3DShadowsEnabled(void)
{
	OptionPreferences::const_iterator it = find("UseShadowVolumes");
	if (it == end())
		return TheGlobalData->m_useShadowVolumes;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::get2DShadowsEnabled(void)
{
	OptionPreferences::const_iterator it = find("UseShadowDecals");
	if (it == end())
		return TheGlobalData->m_useShadowDecals;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Bool OptionPreferences::getBuildingOcclusionEnabled(void)
{
	OptionPreferences::const_iterator it = find("BuildingOcclusion");
	if (it == end())
		return TheGlobalData->m_enableBehindBuildingMarkers;

	if (stricmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

Int OptionPreferences::getParticleCap(void)
{
	OptionPreferences::const_iterator it = find("MaxParticleCount");
	if (it == end())
		return TheGlobalData->m_maxParticleCount;

	Int factor = (Int) atoi(it->second.str());
	if (factor < 100)	//clamp to at least 100 particles.
		factor = 100;

	return factor;
}

Int OptionPreferences::getTextureReduction(void)
{
	OptionPreferences::const_iterator it = find("TextureReduction");
	if (it == end())
		return -1;	//unknown texture reduction

	Int factor = (Int) atoi(it->second.str());
	if (factor > 2)	//clamp it.
		factor=2;
	return factor;
}

Real OptionPreferences::getGammaValue(void)
{
	OptionPreferences::const_iterator it = find("Gamma");
 	if (it == end())
 		return 50.0f;
 
 	Real gamma = (Real) atoi(it->second.str());
 	return gamma;
}

void OptionPreferences::getResolution(Int *xres, Int *yres)
{
	*xres = TheGlobalData->m_xResolution;
	*yres = TheGlobalData->m_yResolution;

	OptionPreferences::const_iterator it = find("Resolution");
	if (it == end())
		return;

	Int selectedXRes,selectedYRes;
	if (sscanf(it->second.str(),"%d%d", &selectedXRes, &selectedYRes) != 2)
		return;

	*xres=selectedXRes;
	*yres=selectedYRes;
}

Real OptionPreferences::getMusicVolume(void)
{
	OptionPreferences::const_iterator it = find("MusicVolume");
	if (it == end())
		return TheAudio->getAudioSettings()->m_defaultMusicVolume * 100.0f;

	Real volume = (Real) atof(it->second.str());
	if (volume < 0.0f)
	{
		volume = 0.0f;
	}
	return volume;
}

static OptionPreferences *pref = NULL;

static void SetCheckBoxCheckedSafe( GameWindow *checkBox, Bool checked )
{
	if (checkBox != NULL)
		GadgetCheckBoxSetChecked( checkBox, checked );
}

static void SetSliderPositionSafe( GameWindow *slider, Int pos )
{
	if (slider != NULL)
		GadgetSliderSetPosition( slider, pos );
}

static void SetWindowTextSafe( GameWindow *window, const wchar_t *text )
{
	if (window != NULL)
	{
		UnicodeString label( text );
		window->winSetText( label );
		TheWindowManager->winSendSystemMsg( window, GGM_SET_LABEL, (WindowMsgData)&label, 0 );
	}
}

static void SetComboBoxSelectedPosSafe( GameWindow *comboBox, Int pos )
{
	if (comboBox != NULL)
		GadgetComboBoxSetSelectedPos( comboBox, pos );
}

static Real getOptionMenuUIScaleFactor( void )
{
	if (pref == NULL)
		return 1.0f;

	AsciiString uiScalePref = (*pref)["UIScale"];
	if (uiScalePref.compareNoCase("75") == 0 || uiScalePref.compareNoCase("90") == 0 || uiScalePref.compareNoCase("minimal") == 0 || uiScalePref.compareNoCase("compact") == 0)
		return 0.75f;
	if (uiScalePref.compareNoCase("125") == 0 || uiScalePref.compareNoCase("110") == 0 || uiScalePref.compareNoCase("large") == 0)
		return 1.25f;
	if (uiScalePref.compareNoCase("150") == 0 || uiScalePref.compareNoCase("largest") == 0)
		return 1.50f;
	return 1.0f;
}

static Int getMasterVolumePercent( void )
{
	if (pref == NULL)
		return 100;

	Int masterVolume = 100;
	AsciiString masterPref = (*pref)["MasterVolume"];
	if (!masterPref.isEmpty())
	{
		masterVolume = atoi( masterPref.str() );
		if (masterVolume < 0)
			masterVolume = 0;
		if (masterVolume > 100)
			masterVolume = 100;
	}

	return masterVolume;
}

static Int normalizeChannelVolumeForDisplay( Real effectiveVolumePercent, Int masterVolumePercent )
{
	if (masterVolumePercent <= 0)
		return REAL_TO_INT( effectiveVolumePercent );

	Real rawVolumePercent = effectiveVolumePercent * 100.0f / (Real)masterVolumePercent;
	if (rawVolumePercent < 0.0f)
		rawVolumePercent = 0.0f;
	if (rawVolumePercent > 100.0f)
		rawVolumePercent = 100.0f;
	return REAL_TO_INT( rawVolumePercent + 0.5f );
}

static Int getSpeakerTypeComboIndex( const AsciiString &speakerType )
{
	if (speakerType.compareNoCase("Headphones") == 0)
		return 1;
	if (speakerType.compareNoCase("Surround Sound") == 0)
		return 2;
	if (speakerType.compareNoCase("4 Speaker") == 0 || speakerType.compareNoCase("4 Speakers") == 0)
		return 3;
	if (speakerType.compareNoCase("5.1 Surround") == 0)
		return 4;
	if (speakerType.compareNoCase("7.1 Surround") == 0)
		return 5;
	return 0;
}

static AsciiString getSpeakerTypeForComboIndex( Int index )
{
	switch (index)
	{
		case 1:
			return AsciiString( "Headphones" );
		case 2:
			return AsciiString( "Surround Sound" );
		case 3:
			return AsciiString( "4 Speaker" );
		case 4:
			return AsciiString( "5.1 Surround" );
		case 5:
			return AsciiString( "7.1 Surround" );
		default:
			return AsciiString( "2 Speakers" );
	}
}

static Int getNormalizedOptionMenuBasePointSize( UnsignedInt style, GameFont *font )
{
	if (font == NULL)
		return 0;

	if (BitTest(style, GWS_COMBO_BOX) || BitTest(style, GWS_CHECK_BOX))
		return 12; // H3 controls

	if (BitTest(style, GWS_STATIC_TEXT))
	{
		if (font->pointSize >= 13)
			return 14; // H2 labels
		return 12; // H3 helper text
	}

	return font->pointSize;
}

static void scaleOptionMenuFontsRecursive( GameWindow *window, Real scale )
{
	if (window == NULL || TheFontLibrary == NULL)
		return;

	UnsignedInt style = window->winGetStyle();
	if (!BitTest(style, GWS_PUSH_BUTTON))
	{
		GameFont *font = window->winGetFont();
		if (font != NULL)
		{
			Real effectiveScale = scale;
			if (!BitTest(style, GWS_STATIC_TEXT) &&
				!BitTest(style, GWS_COMBO_BOX) &&
				!BitTest(style, GWS_CHECK_BOX))
			{
				effectiveScale = 1.0f + ((scale - 1.0f) * 0.35f);
			}

			Int basePointSize = getNormalizedOptionMenuBasePointSize( style, font );
			if (basePointSize <= 0)
				basePointSize = font->pointSize;

			Int newPointSize = REAL_TO_INT((Real)basePointSize * effectiveScale + 0.25f);
			if (newPointSize < 8)
				newPointSize = 8;
			if (newPointSize > 16)
				newPointSize = 16;

			if (newPointSize != font->pointSize)
			{
				GameFont *scaledFont = TheFontLibrary->getFont( font->nameString, newPointSize, font->bold );
				if (scaledFont != NULL)
				{
					if (BitTest(style, GWS_STATIC_TEXT))
						GadgetStaticTextSetFont( window, scaledFont );
					else
						window->winSetFont( scaledFont );
				}
			}
		}
	}

	GameWindow *child = window->winGetChild();
	while (child != NULL)
	{
		scaleOptionMenuFontsRecursive( child, scale );
		child = child->winGetNext();
	}
}

static void applyResolutionAwareOptionsScale( WindowLayout *layout )
{
	GameWindow *root = layout ? layout->getFirstWindow() : NULL;
	if (root == NULL)
		return;

	Real uiScale = getOptionMenuUIScaleFactor();
	Real resolutionScale = 1.0f;
	if (TheDisplay != NULL)
	{
		if (TheDisplay->getWidth() >= 2560 || TheDisplay->getHeight() >= 1440)
			resolutionScale = 1.05f;
		if (TheDisplay->getWidth() >= 3840 || TheDisplay->getHeight() >= 2160)
			resolutionScale = 1.10f;
	}

	Real finalScale = uiScale * resolutionScale;
	if (finalScale < 0.90f)
		finalScale = 0.90f;
	if (finalScale > 1.35f)
		finalScale = 1.35f;

	scaleOptionMenuFontsRecursive( root, finalScale );
}

static void setDefaults( void )
{
	//-------------------------------------------------------------------------------------------------
	// provider type
//	GadgetCheckBoxSetChecked(checkAudioHardware, FALSE);

	//-------------------------------------------------------------------------------------------------
	// speaker type
//	GadgetCheckBoxSetChecked(checkAudioSurround, FALSE);

	//-------------------------------------------------------------------------------------------------
	// language filter
	SetCheckBoxCheckedSafe( checkLanguageFilter, TRUE );

	//-------------------------------------------------------------------------------------------------
	// send Delay
	SetCheckBoxCheckedSafe( checkSendDelay, FALSE );
	
	//-------------------------------------------------------------------------------------------------
	// LOD
	if ((TheGameLogic->isInGame() == FALSE) || (TheGameLogic->isInShellGame() == TRUE)) {
		StaticGameLODLevel level=TheGameLODManager->findStaticLODLevel();
		switch (level)
		{
		case STATIC_GAME_LOD_LOW:
			GadgetComboBoxSetSelectedPos(comboBoxDetail, LOWDETAIL);
			break;
		case STATIC_GAME_LOD_MEDIUM:
			GadgetComboBoxSetSelectedPos(comboBoxDetail, MEDIUMDETAIL);
			break;
		case STATIC_GAME_LOD_HIGH:
			GadgetComboBoxSetSelectedPos(comboBoxDetail, HIGHDETAIL);
			break;
		case STATIC_GAME_LOD_CUSTOM:
			GadgetComboBoxSetSelectedPos(comboBoxDetail, CUSTOMDETAIL);
			break;
		default:
			DEBUG_ASSERTCRASH(FALSE,("Tried to set comboBoxDetail to a value of %d ", TheGameLODManager->getStaticLODLevel()) );
		};
	}
	
	//-------------------------------------------------------------------------------------------------
	// Resolution
	//Find index of 800x600 mode.
	if ((TheGameLogic->isInGame() == FALSE) || (TheGameLogic->isInShellGame() == TRUE)  && !TheGameSpyInfo) {
		Int numResolutions = TheDisplay->getDisplayModeCount();
		Int defaultResIndex=0;
		for( Int i = 0; i < numResolutions; ++i )
		{	Int xres,yres,bitDepth;
			TheDisplay->getDisplayModeDescription(i,&xres,&yres,&bitDepth);
			if (xres == 800 && yres == 600)	//keep track of default mode in case we need it.
			{	defaultResIndex=i;
				break;
			}
		}
		GadgetComboBoxSetSelectedPos( comboBoxResolution, defaultResIndex );	//should be 800x600 (our lowest supported mode)
	}


	//-------------------------------------------------------------------------------------------------
	// Mouse Mode
	SetCheckBoxCheckedSafe( checkAlternateMouse, FALSE );
	SetCheckBoxCheckedSafe( checkRetaliation, TRUE );
	SetCheckBoxCheckedSafe( checkDoubleClickAttackMove, FALSE );

	//-------------------------------------------------------------------------------------------------
//	// scroll speed val
	Int valMin, valMax;
//	GadgetSliderGetMinMax(sliderScrollSpeed,&valMin, &valMax);
//	GadgetSliderSetPosition(sliderScrollSpeed, ((valMax - valMin) / 2 + valMin));
	Int scrollPos = (Int)(TheGlobalData->m_keyboardDefaultScrollFactor*100.0f);
	SetSliderPositionSafe( sliderScrollSpeed, scrollPos );


	//-------------------------------------------------------------------------------------------------
	// slider music volume
	GadgetSliderGetMinMax(sliderMusicVolume,&valMin, &valMax);
	SetSliderPositionSafe( sliderMusicVolume, REAL_TO_INT(TheAudio->getAudioSettings()->m_defaultMusicVolume * 100.0f) );

	//-------------------------------------------------------------------------------------------------
	// slider SFX volume
	GadgetSliderGetMinMax(sliderSFXVolume,&valMin, &valMax);
	Real maxVolume = MAX( TheAudio->getAudioSettings()->m_defaultSoundVolume, TheAudio->getAudioSettings()->m_default3DSoundVolume );
	SetSliderPositionSafe( sliderSFXVolume, REAL_TO_INT( maxVolume * 100.0f ) );

	//-------------------------------------------------------------------------------------------------
	// slider Voice volume
	GadgetSliderGetMinMax(sliderVoiceVolume,&valMin, &valMax);
	SetSliderPositionSafe( sliderVoiceVolume, REAL_TO_INT(TheAudio->getAudioSettings()->m_defaultSpeechVolume * 100.0f) );

	//-------------------------------------------------------------------------------------------------
	// master volume and speaker type
	if (sliderMasterVolume)
	{
		GadgetSliderGetMinMax(sliderMasterVolume,&valMin, &valMax);
		SetSliderPositionSafe( sliderMasterVolume, 100 );
	}
	if (comboBoxSpeakerType)
	{
		AsciiString defaultSpeaker = TheAudio->translateUnsignedIntToSpeakerType( TheAudio->getAudioSettings()->m_defaultSpeakerType2D );
		SetComboBoxSelectedPosSafe( comboBoxSpeakerType, getSpeakerTypeComboIndex( defaultSpeaker ) );
	}

	//-------------------------------------------------------------------------------------------------
 	// slider Gamma
 	GadgetSliderGetMinMax(sliderGamma,&valMin, &valMax);
 	SetSliderPositionSafe( sliderGamma, ((valMax - valMin) / 2 + valMin) );

	//-------------------------------------------------------------------------------------------------
 	// Texture resolution slider
	//

	if ((TheGameLogic->isInGame() == FALSE) || (TheGameLogic->isInShellGame() == TRUE))
	{	
		Int	txtFact=TheGameLODManager->getRecommendedTextureReduction();

		SetSliderPositionSafe( sliderTextureResolution, 2-txtFact );

		//-------------------------------------------------------------------------------------------------
 		// 3D Shadows checkbox
		//
		SetCheckBoxCheckedSafe( check3DShadows, TheGlobalData->m_useShadowVolumes );

		//-------------------------------------------------------------------------------------------------
 		// 2D Shadows checkbox
		//
		SetCheckBoxCheckedSafe( check2DShadows, TheGlobalData->m_useShadowDecals );

		//-------------------------------------------------------------------------------------------------
 		// Cloud Shadows checkbox
		//
		SetCheckBoxCheckedSafe( checkCloudShadows, TheGlobalData->m_useCloudMap );

		//-------------------------------------------------------------------------------------------------
 		// Ground Lighting (lightmap) checkbox
		//
		SetCheckBoxCheckedSafe( checkGroundLighting, TheGlobalData->m_useLightMap );

		//-------------------------------------------------------------------------------------------------
 		// Smooth Water Border checkbox
		//
		SetCheckBoxCheckedSafe( checkSmoothWater, TheGlobalData->m_showSoftWaterEdge );

		//-------------------------------------------------------------------------------------------------
 		// Extra Animations (tree sway and buildups) checkbox
		//
		SetCheckBoxCheckedSafe( checkExtraAnimations, !TheGlobalData->m_useDrawModuleLOD );

		//-------------------------------------------------------------------------------------------------
 		// DisableDynamicLOD
		//
		SetCheckBoxCheckedSafe( checkNoDynamicLod, !TheGlobalData->m_enableDynamicLOD );

		//-------------------------------------------------------------------------------------------------
 		// VSync
		//
		if (checkVSync) GadgetCheckBoxSetChecked( checkVSync, TheGlobalData->m_vsync);

		//-------------------------------------------------------------------------------------------------
 		// FPS Counter
		//
		if (checkFPSCounter) GadgetCheckBoxSetChecked( checkFPSCounter, TheGlobalData->m_showFPSCounter);

		//-------------------------------------------------------------------------------------------------
 		// FPS Limit
		//
		if (checkFPSLimit) GadgetCheckBoxSetChecked( checkFPSLimit, TheGlobalData->m_useFpsLimit );

		if (comboBoxUIScale) GadgetComboBoxSetSelectedPos( comboBoxUIScale, 1 );
		if (comboBoxVoiceFrequency) GadgetComboBoxSetSelectedPos( comboBoxVoiceFrequency, 2 );
		if (comboBoxTextureQuality) GadgetComboBoxSetSelectedPos( comboBoxTextureQuality, 0 );  // default High
		if (comboBoxParticleQuality) GadgetComboBoxSetSelectedPos( comboBoxParticleQuality, 2 );  // default High
		if (comboBoxUIThemeColor) GadgetComboBoxSetSelectedPos( comboBoxUIThemeColor, 0 );  // default Blue

		//-------------------------------------------------------------------------------------------------
 		// Heat Effects
		//
		SetCheckBoxCheckedSafe( checkHeatEffects, TheGlobalData->m_useHeatEffects );

		//-------------------------------------------------------------------------------------------------
 		// Building Occlusion checkbox
		//
		SetCheckBoxCheckedSafe( checkBuildingOcclusion, TheGlobalData->m_enableBehindBuildingMarkers );

		//-------------------------------------------------------------------------------------------------
 		// Particle Cap slider
		//
		SetSliderPositionSafe( sliderParticleCap, TheGlobalData->m_maxParticleCount );

		//-------------------------------------------------------------------------------------------------
 		// Trees and Shrubs
		//
		SetCheckBoxCheckedSafe( checkProps, TheGlobalData->m_useTrees );
	}
}

static void saveOptions( void )
{
	Int index;
	Int val;
	//-------------------------------------------------------------------------------------------------
//	// provider type
//	Bool isChecked = GadgetCheckBoxIsChecked(checkAudioHardware);
//	TheAudio->setHardwareAccelerated(isChecked);
//	(*pref)["3DAudioProvider"] = TheAudio->getProviderName(TheAudio->getSelectedProvider());
//
	//-------------------------------------------------------------------------------------------------
//	// speaker type
	//	isChecked = GadgetCheckBoxIsChecked(checkAudioSurround);
	//	TheAudio->setSpeakerSurround(isChecked);
	//	(*pref)["SpeakerType"] = TheAudio->translateUnsignedIntToSpeakerType(TheAudio->getSpeakerType());
	//
	//-------------------------------------------------------------------------------------------------
	// language filter
	if( GadgetCheckBoxIsChecked( checkLanguageFilter ) )
	{
			//GadgetCheckBoxSetChecked( checkLanguageFilter, true);
			TheWritableGlobalData->m_languageFilterPref = true;
			(*pref)["LanguageFilter"] = "true";
	}
	else
	{
			//GadgetCheckBoxSetChecked( checkLanguageFilter, false);
			TheWritableGlobalData->m_languageFilterPref = false;
			(*pref)["LanguageFilter"] = "false";
	}
	
	//-------------------------------------------------------------------------------------------------
	// send Delay
	TheWritableGlobalData->m_firewallSendDelay = GadgetCheckBoxIsChecked(checkSendDelay);
	if (TheGlobalData->m_firewallSendDelay) {
		(*pref)["SendDelay"] = AsciiString("yes");
	} else {
		(*pref)["SendDelay"] = AsciiString("no");
	}

	//-------------------------------------------------------------------------------------------------
	// Custom game detail settings.
	if (comboBoxDetail)
		GadgetComboBoxGetSelectedPos( comboBoxDetail, &index );
	else
		index = HIGHDETAIL;  // safe fallback if widget not found
	if (index == CUSTOMDETAIL)
	{
 		//-------------------------------------------------------------------------------------------------
 		// Texture resolution slider
		{
				AsciiString prefString;

		 		val = GadgetSliderGetPosition(sliderTextureResolution);
				val = 2-val;

				prefString.format("%d",val);
				(*pref)["TextureReduction"] = prefString;

				if (TheGlobalData->m_textureReductionFactor != val)
				{
					TheGameClient->adjustLOD(val-TheGlobalData->m_textureReductionFactor);	//apply the new setting
				}
		}

		TheWritableGlobalData->m_useShadowVolumes = GadgetCheckBoxIsChecked( check3DShadows );
		(*pref)["UseShadowVolumes"] = TheWritableGlobalData->m_useShadowVolumes ? AsciiString("yes") : AsciiString("no");

		TheWritableGlobalData->m_useShadowDecals = GadgetCheckBoxIsChecked( check2DShadows );
		(*pref)["UseShadowDecals"] = TheWritableGlobalData->m_useShadowDecals ? AsciiString("yes") : AsciiString("no");

		TheWritableGlobalData->m_useCloudMap = GadgetCheckBoxIsChecked( checkCloudShadows );
		(*pref)["UseCloudMap"] = TheGlobalData->m_useCloudMap ? AsciiString("yes") : AsciiString("no");

		TheWritableGlobalData->m_useLightMap = GadgetCheckBoxIsChecked( checkGroundLighting );
		(*pref)["UseLightMap"] = TheGlobalData->m_useLightMap ? AsciiString("yes") : AsciiString("no");

		TheWritableGlobalData->m_showSoftWaterEdge = GadgetCheckBoxIsChecked( checkSmoothWater );
		(*pref)["ShowSoftWaterEdge"] = TheGlobalData->m_showSoftWaterEdge ? AsciiString("yes") : AsciiString("no");

		TheWritableGlobalData->m_useDrawModuleLOD = !GadgetCheckBoxIsChecked( checkExtraAnimations );
		TheWritableGlobalData->m_useTreeSway = !TheWritableGlobalData->m_useDrawModuleLOD;	//borrow same setting.
		(*pref)["ExtraAnimations"] = TheGlobalData->m_useDrawModuleLOD ? AsciiString("no") : AsciiString("yes");

		TheWritableGlobalData->m_enableDynamicLOD = !GadgetCheckBoxIsChecked( checkNoDynamicLod );
		(*pref)["DynamicLOD"] = TheGlobalData->m_enableDynamicLOD ? AsciiString("yes") : AsciiString("no");

		TheWritableGlobalData->m_useHeatEffects = GadgetCheckBoxIsChecked( checkHeatEffects );
		(*pref)["HeatEffects"] = TheGlobalData->m_useHeatEffects ? AsciiString("yes") : AsciiString("no");

		TheWritableGlobalData->m_enableBehindBuildingMarkers = GadgetCheckBoxIsChecked( checkBuildingOcclusion );
		(*pref)["BuildingOcclusion"] = TheWritableGlobalData->m_enableBehindBuildingMarkers ? AsciiString("yes") : AsciiString("no");

		TheWritableGlobalData->m_useTrees = GadgetCheckBoxIsChecked( checkProps);
		(*pref)["ShowTrees"] = TheWritableGlobalData->m_useTrees ? AsciiString("yes") : AsciiString("no");

 		//-------------------------------------------------------------------------------------------------
		// Particle Cap slider
		{
				AsciiString prefString;

		 		val = GadgetSliderGetPosition(sliderParticleCap);

				prefString.format("%d",val);
				(*pref)["MaxParticleCount"] = prefString;

				TheWritableGlobalData->m_maxParticleCount = val;
		}
	}

	// VSync toggle (always save, regardless of detail level)
	if (checkVSync) {
		TheWritableGlobalData->m_vsync = GadgetCheckBoxIsChecked( checkVSync );
		pref->setVSync(TheWritableGlobalData->m_vsync);
	}

	// FPS Counter toggle (always save, regardless of detail level)
	if (checkFPSCounter) {
		TheWritableGlobalData->m_showFPSCounter = GadgetCheckBoxIsChecked( checkFPSCounter );
		pref->setFPSCounter(TheWritableGlobalData->m_showFPSCounter);
	}

	// FPS limiter toggle (always save, regardless of detail level)
	if (checkFPSLimit) {
		TheWritableGlobalData->m_useFpsLimit = GadgetCheckBoxIsChecked( checkFPSLimit );
		(*pref)["FPSLimit"] = TheWritableGlobalData->m_useFpsLimit ? AsciiString("yes") : AsciiString("no");
	}

	// Store the new text-first options added to the tabs.
	if (comboBoxUIScale)
	{
		Int uiScaleIndex = 1;
		static const char * const uiScaleValues[] = { "75", "100", "125", "150" };
		GadgetComboBoxGetSelectedPos( comboBoxUIScale, &uiScaleIndex );
		if (uiScaleIndex < 0 || uiScaleIndex > 3)
			uiScaleIndex = 1;
		(*pref)["UIScale"] = AsciiString( uiScaleValues[uiScaleIndex] );
	}
	if (comboBoxVoiceFrequency)
	{
		Int voiceFreqIndex = 2;
		static const char * const voiceFreqValues[] = { "Off", "Occasional", "Normal", "Always" };
		GadgetComboBoxGetSelectedPos( comboBoxVoiceFrequency, &voiceFreqIndex );
		if (voiceFreqIndex < 0 || voiceFreqIndex > 3)
			voiceFreqIndex = 2;
		(*pref)["VoiceResponseFrequency"] = AsciiString( voiceFreqValues[voiceFreqIndex] );
	}
	// Texture Quality combo (always applied, independent of detail level)
	if (comboBoxTextureQuality)
	{
		Int texQualityIdx = 0;
		GadgetComboBoxGetSelectedPos( comboBoxTextureQuality, &texQualityIdx );
		if (texQualityIdx < 0 || texQualityIdx > 2) texQualityIdx = 0;
		AsciiString texStr;
		texStr.format("%d", texQualityIdx);
		(*pref)["TextureReduction"] = texStr;
		if (TheGlobalData->m_textureReductionFactor != texQualityIdx)
		{
			TheGameClient->adjustLOD(texQualityIdx - TheGlobalData->m_textureReductionFactor);
			TheWritableGlobalData->m_textureReductionFactor = texQualityIdx;
		}
	}
	// Particle Quality combo (always applied, independent of detail level)
	if (comboBoxParticleQuality)
	{
		Int particleQualityIdx = 2;
		GadgetComboBoxGetSelectedPos( comboBoxParticleQuality, &particleQualityIdx );
		static const Int particleCapValues[] = { 600, 2000, 5000 };
		if (particleQualityIdx < 0 || particleQualityIdx > 2) particleQualityIdx = 2;
		Int newCap = particleCapValues[particleQualityIdx];
		AsciiString capStr;
		capStr.format("%d", newCap);
		(*pref)["MaxParticleCount"] = capStr;
		TheWritableGlobalData->m_maxParticleCount = newCap;
	}
	// UI Theme Color combo (save to prefs; takes effect after restart)
	if (comboBoxUIThemeColor)
	{
		Int themeIdx = 0;
		GadgetComboBoxGetSelectedPos( comboBoxUIThemeColor, &themeIdx );
		if (themeIdx < 0 || themeIdx > 3) themeIdx = 0;
		AsciiString themeStr;
		themeStr.format("%d", themeIdx);
		(*pref)["UITheme"] = themeStr;
	}

	//-------------------------------------------------------------------------------------------------
	// LOD
	Bool levelChanged=FALSE;
	GadgetComboBoxGetSelectedPos( comboBoxDetail, &index );
	//The levels stored by the LOD Manager are inverted compared to GUI so find correct one:
	switch (index) {
	case HIGHDETAIL:
		levelChanged=TheGameLODManager->setStaticLODLevel(STATIC_GAME_LOD_HIGH);
		break;
	case MEDIUMDETAIL:
		levelChanged=TheGameLODManager->setStaticLODLevel(STATIC_GAME_LOD_MEDIUM);
		break;
	case LOWDETAIL:
		levelChanged=TheGameLODManager->setStaticLODLevel(STATIC_GAME_LOD_LOW);
		break;
	case CUSTOMDETAIL:
		levelChanged=TheGameLODManager->setStaticLODLevel(STATIC_GAME_LOD_CUSTOM);
		break;
	default:
		DEBUG_ASSERTCRASH(FALSE,("LOD passed in was %d, %d is not a supported LOD",index,index));
		break;
	}

	if (levelChanged)
	        (*pref)["StaticGameLOD"] = TheGameLODManager->getStaticGameLODLevelName(TheGameLODManager->getStaticLODLevel());

	//-------------------------------------------------------------------------------------------------
	// Display mode (read first; used when applying resolution too)
	Bool newWindowed  = TheGlobalData->m_windowed;
	Bool newBorderless = TheGlobalData->m_borderless;
	Bool displayModeChanged = FALSE;
	{
		Int dmIndex;
		GadgetComboBoxGetSelectedPos(comboBoxAntiAliasing, &dmIndex);
		if (dmIndex >= 0)
		{
			Bool dm_windowed  = (dmIndex >= 1) ? TRUE : FALSE;
			Bool dm_borderless = (dmIndex == 1) ? TRUE : FALSE;
			AsciiString dmPref;
			dmPref.format("%d", dmIndex);
			(*pref)["DisplayMode"] = dmPref;
			if (TheGlobalData->m_windowed != dm_windowed || TheGlobalData->m_borderless != dm_borderless)
			{
				newWindowed   = dm_windowed;
				newBorderless = dm_borderless;
				displayModeChanged = TRUE;
				TheWritableGlobalData->m_windowed   = dm_windowed;
				TheWritableGlobalData->m_borderless = dm_borderless;
			}
		}
	}

	//-------------------------------------------------------------------------------------------------
	// Resolution
	GadgetComboBoxGetSelectedPos( comboBoxResolution, &index );
	Int xres, yres, bitDepth;
	
	oldDispSettings.xRes = TheDisplay->getWidth();
	oldDispSettings.yRes = TheDisplay->getHeight();
	oldDispSettings.bitDepth = TheDisplay->getBitDepth();
	oldDispSettings.windowed = TheDisplay->getWindowed();
	
	if (index < TheDisplay->getDisplayModeCount() && index >= 0)
	{
		TheDisplay->getDisplayModeDescription(index,&xres,&yres,&bitDepth);
		if (TheGlobalData->m_xResolution != xres || TheGlobalData->m_yResolution != yres || displayModeChanged)
		{
			
			if (TheDisplay->setDisplayMode(xres,yres,bitDepth,newWindowed))
			{
				dispChanged = TRUE;
				TheWritableGlobalData->m_xResolution = xres;
				TheWritableGlobalData->m_yResolution = yres;

				TheHeaderTemplateManager->headerNotifyResolutionChange();
				TheMouse->mouseNotifyResolutionChange();
				
				//Save new settings for a dialog box confirmation after options are accepted
				newDispSettings.xRes = xres;
				newDispSettings.yRes = yres;
				newDispSettings.bitDepth = bitDepth;
				newDispSettings.windowed = TheDisplay->getWindowed();

				AsciiString prefString;
				prefString.format("%d %d", xres, yres );
				(*pref)["Resolution"] = prefString;

				// delete the shell
				delete TheShell;
				TheShell = NULL;

				// create the shell
				TheShell = MSGNEW("GameClientSubsystem") Shell;
				if( TheShell )
					TheShell->init();
				
				TheInGameUI->recreateControlBar();

				TheShell->push( AsciiString("Menus/MainMenu.wnd") );
			}
		}
	}

	//-------------------------------------------------------------------------------------------------
	// IP address
	UnsignedInt ip;
	GadgetComboBoxGetSelectedPos(comboBoxLANIP, &index);
	if (index>=0 && TheGlobalData)
	{
		ip = static_cast<UnsignedInt>(reinterpret_cast<uintptr_t>(GadgetComboBoxGetItemData(comboBoxLANIP, index)));
		TheWritableGlobalData->m_defaultIP = ip;
		pref->setLANIPAddress(ip);
	}
	GadgetComboBoxGetSelectedPos(comboBoxOnlineIP, &index);
	if (index>=0)
	{
		ip = static_cast<UnsignedInt>(reinterpret_cast<uintptr_t>(GadgetComboBoxGetItemData(comboBoxOnlineIP, index)));
		pref->setOnlineIPAddress(ip);
	}

	//-------------------------------------------------------------------------------------------------
	// HTTP Proxy
	GameWindow *textEntryHTTPProxy = TheWindowManager->winGetWindowFromId(NULL, NAMEKEY("OptionsMenu.wnd:TextEntryHTTPProxy"));
	if (textEntryHTTPProxy)
	{
		UnicodeString uStr = GadgetTextEntryGetText(textEntryHTTPProxy);
		AsciiString aStr;
		aStr.translate(uStr);
		SetStringInRegistry("", "Proxy", aStr.str());
		ghttpSetProxy(aStr.str());
	}

	//-------------------------------------------------------------------------------------------------
	// Firewall Port Override
	GameWindow *textEntryFirewallPortOverride = TheWindowManager->winGetWindowFromId(NULL, NAMEKEY("OptionsMenu.wnd:TextEntryFirewallPortOverride"));
	if (textEntryFirewallPortOverride)
	{
		UnicodeString uStr = GadgetTextEntryGetText(textEntryFirewallPortOverride);
		AsciiString aStr;
		aStr.translate(uStr);
		Int override = atoi(aStr.str());
		if (override < 0 || override > 65535)
			override = 0;
		if (TheGlobalData->m_firewallPortOverride != override)
		{	TheWritableGlobalData->m_firewallPortOverride = override;
		    aStr.format("%d", override);
			(*pref)["FirewallPortOverride"] = aStr;
		}
	}

	//-------------------------------------------------------------------------------------------------
	// mouse mode
	TheWritableGlobalData->m_useAlternateMouse = GadgetCheckBoxIsChecked(checkAlternateMouse);
	(*pref)["UseAlternateMouse"] = TheWritableGlobalData->m_useAlternateMouse ? AsciiString("yes") : AsciiString("no");

	TheWritableGlobalData->m_clientRetaliationModeEnabled = GadgetCheckBoxIsChecked(checkRetaliation);
	(*pref)["Retaliation"] = TheWritableGlobalData->m_clientRetaliationModeEnabled? AsciiString("yes") : AsciiString("no");

	TheWritableGlobalData->m_doubleClickAttackMove = GadgetCheckBoxIsChecked( checkDoubleClickAttackMove );
	(*pref)["UseDoubleClickAttackMove"] = TheWritableGlobalData->m_doubleClickAttackMove ? AsciiString("yes") : AsciiString("no");

	//-------------------------------------------------------------------------------------------------
	// scroll speed val
	val = GadgetSliderGetPosition(sliderScrollSpeed);
	if(val != -1)
	{
		TheWritableGlobalData->m_keyboardScrollFactor = val/100.0f;
		AsciiString prefString;
		prefString.format("%d", val);
		(*pref)["ScrollFactor"] = prefString;
	}
	
	//-------------------------------------------------------------------------------------------------
	// Master volume and audio output mode
	Int masterVolumePercent = 100;
	Real masterVolumeScale = 1.0f;
	if (sliderMasterVolume)
	{
		masterVolumePercent = GadgetSliderGetPosition( sliderMasterVolume );
		if (masterVolumePercent < 0)
			masterVolumePercent = 100;
		masterVolumePercent = MIN( 100, MAX( 0, masterVolumePercent ) );
		masterVolumeScale = masterVolumePercent / 100.0f;

		AsciiString prefString;
		prefString.format("%d", masterVolumePercent);
		(*pref)["MasterVolume"] = prefString;
	}
	if (comboBoxSpeakerType)
	{
		Int speakerTypeIndex = 0;
		GadgetComboBoxGetSelectedPos( comboBoxSpeakerType, &speakerTypeIndex );
		AsciiString speakerType = getSpeakerTypeForComboIndex( speakerTypeIndex );
		(*pref)["SpeakerType"] = speakerType;
		if (TheAudio)
		{
			TheAudio->setPreferredSpeaker( speakerType );
			TheAudio->setSpeakerType( TheAudio->translateSpeakerTypeToUnsignedInt( speakerType ) );
		}
	}

	//-------------------------------------------------------------------------------------------------
	// slider music volume
	val = GadgetSliderGetPosition(sliderMusicVolume);
	if(val != -1)
	{
		Real musicVolume = (val / 100.0f) * masterVolumeScale;
	  TheWritableGlobalData->m_musicVolumeFactor = REAL_TO_INT( musicVolume * 100.0f );
    AsciiString prefString;
    prefString.format("%d", REAL_TO_INT( musicVolume * 100.0f ) );
    (*pref)["MusicVolume"] = prefString;
    TheAudio->setVolume(musicVolume, (AudioAffect) (AudioAffect_Music | AudioAffect_SystemSetting));
	}
	
	//-------------------------------------------------------------------------------------------------
	// slider SFX volume
	val = GadgetSliderGetPosition(sliderSFXVolume);
	if(val != -1)
	{
		//Both 2D and 3D sound effects are sharing the same slider. However, there is a 
		//relative slider that gets applied to one of these values to lower that sound volume.
		Real baseSFXVolume = (val / 100.0f) * masterVolumeScale;
		Real sound2DVolume = baseSFXVolume;
		Real sound3DVolume = baseSFXVolume;
		Real relative2DVolume = TheAudio->getAudioSettings()->m_relative2DVolume;
		relative2DVolume = MIN( 1.0f, MAX( -1.0, relative2DVolume ) );
		if( relative2DVolume < 0.0f )
		{
			//Lower the 2D volume
			sound2DVolume *= 1.0f + relative2DVolume;
		}
		else
		{
			//Lower the 3D volume
			sound3DVolume *= 1.0f - relative2DVolume;
		}

		//Apply the sound volumes in the audio system now.
    TheAudio->setVolume( sound2DVolume, (AudioAffect) (AudioAffect_Sound | AudioAffect_SystemSetting) );
		TheAudio->setVolume( sound3DVolume, (AudioAffect) (AudioAffect_Sound3D | AudioAffect_SystemSetting) );

		//Save the settings in the options.ini.
    TheWritableGlobalData->m_SFXVolumeFactor = REAL_TO_INT( MAX( sound2DVolume, sound3DVolume ) * 100.0f );
    AsciiString prefString;
    prefString.format("%d", REAL_TO_INT( sound2DVolume * 100.0f ) );
    (*pref)["SFXVolume"] = prefString;
    prefString.format("%d", REAL_TO_INT( sound3DVolume * 100.0f ) );
		(*pref)["SFX3DVolume"] = prefString;
	}

	//-------------------------------------------------------------------------------------------------
	// slider Voice volume
	val = GadgetSliderGetPosition(sliderVoiceVolume);
	if(val != -1)
	{
		Real voiceVolume = (val / 100.0f) * masterVolumeScale;
    TheWritableGlobalData->m_voiceVolumeFactor = REAL_TO_INT( voiceVolume * 100.0f );
    AsciiString prefString;
    prefString.format("%d", REAL_TO_INT( voiceVolume * 100.0f ) );
    (*pref)["VoiceVolume"] = prefString;
    TheAudio->setVolume(voiceVolume, (AudioAffect) (AudioAffect_Speech | AudioAffect_SystemSetting));
	}

 	//-------------------------------------------------------------------------------------------------
 	// slider Gamma
 	val = GadgetSliderGetPosition(sliderGamma);
 	if(val != -1)
 	{
		Real gammaval=1.0f;
		//generate a value between 0.6 and 2.0.
		if (val < 50)
		{	//darker gamma
			if (val <= 0)
				gammaval = 0.6f;
			else
				gammaval=1.0f-(0.4f) * (Real)(50-val)/50.0f;
		}
		else
		if (val > 50)
			gammaval=1.0f+(1.0f) * (Real)(val-50)/50.0f;

 		AsciiString prefString;
 		prefString.format("%d", val);
 		(*pref)["Gamma"] = prefString;

		if (TheGlobalData->m_displayGamma != gammaval)
		{	TheWritableGlobalData->m_displayGamma = gammaval;
			TheDisplay->setGamma(TheGlobalData->m_displayGamma,0.0f, 1.0f, FALSE);
		}
 	}

}

static void DestroyOptionsLayout() {

	SignalUIInteraction(SHELL_SCRIPT_HOOK_OPTIONS_CLOSED);

	TheShell->destroyOptionsLayout();
	OptionsLayout = NULL;
}

/// Move a window to the head of its parent's sibling list.
/// Children are rendered in REVERSE order (last→first), so the HEAD child
/// is drawn last = topmost visual layer, and is also first in hit-testing.
static void bringWindowToFront( GameWindow *win )
{
	if (win == NULL) return;
	GameWindow *parent = win->winGetParent();
	if (parent == NULL) return;
	if (parent->winGetChild() == win) return;  // already at head = already topmost

	// Unlink win from its current position
	GameWindow *prev = win->winGetPrev();
	GameWindow *next = win->winGetNext();
	if (prev) prev->winSetNext( next );
	if (next) next->winSetPrev( prev );

	// Splice win onto the head of the parent's child list
	GameWindow *head = parent->winGetChild();
	win->winSetPrev( NULL );
	win->winSetNext( head );
	if (head) head->winSetPrev( win );
	parent->winSetChild( win );
}

static Int getTabScrollRange( GameWindow *panel )
{
	if (panel == NULL)
		return 0;

	Int pw = 0, ph = 0;
	panel->winGetSize(&pw, &ph);

	Int panelBottom = ph;
	Int maxBottom = 0;
	GameWindow *child = panel->winGetChild();
	while (child != NULL)
	{
		Int cx = 0, cy = 0;
		Int cw = 0, ch = 0;
		child->winGetPosition(&cx, &cy);
		child->winGetSize(&cw, &ch);
		if (cy + ch > maxBottom)
			maxBottom = cy + ch;
		child = child->winGetNext();
	}

	Int scrollRange = maxBottom - panelBottom + 12;
	if (scrollRange < 0)
		scrollRange = 0;
	return scrollRange;
}

/// Return the tab panel window for a given tab index.
static GameWindow* getTabPanel( Int idx )
{
	switch (idx)
	{
		case 0: return tabPanelAudio;
		case 1: return tabPanelVideo;
		case 2: return tabPanelDisplay;
		case 3: return tabPanelAccessibility;
		case 4: return tabPanelNetwork;
		default: return NULL;
	}
}

static void showTab( Int tabIdx )
{
	// Un-scroll the departing tab so controls return to their WND-defined positions.
	if (s_currentScrollPx != 0)
	{
		GameWindow *prevPanel = getTabPanel(activeTab);
		if (prevPanel != NULL)
		{
			GameWindow *child = prevPanel->winGetChild();
			while (child != NULL)
			{
				Int cx, cy;
				child->winGetPosition(&cx, &cy);
				child->winSetPosition(cx, cy + s_currentScrollPx);
				child = child->winGetNext();
			}
		}
		s_currentScrollPx = 0;
		if (panelScrollBar) GadgetSliderSetPosition(panelScrollBar, 100);
	}

	if (tabPanelAudio)         tabPanelAudio->winHide(TRUE);
	if (tabPanelVideo)         tabPanelVideo->winHide(TRUE);
	if (tabPanelDisplay)       tabPanelDisplay->winHide(TRUE);
	if (tabPanelAccessibility) tabPanelAccessibility->winHide(TRUE);
	if (tabPanelKeyMapping)    tabPanelKeyMapping->winHide(TRUE);
	if (tabPanelNetwork)       tabPanelNetwork->winHide(TRUE);
	activeTab = tabIdx;
	switch (tabIdx)
	{
		case 0: if (tabPanelAudio)         tabPanelAudio->winHide(FALSE);         break;
		case 1: if (tabPanelVideo)         tabPanelVideo->winHide(FALSE);         break;
		case 2: if (tabPanelDisplay)       tabPanelDisplay->winHide(FALSE);       break;
		case 3: if (tabPanelAccessibility) tabPanelAccessibility->winHide(FALSE); break;
		case 4: if (tabPanelNetwork)       tabPanelNetwork->winHide(FALSE);       break;
	}

	s_maxScrollPx = getTabScrollRange( getTabPanel(activeTab) );
	if (panelScrollBar)
	{
		Bool hasOverflow = (s_maxScrollPx > 0) ? TRUE : FALSE;
		panelScrollBar->winHide( !hasOverflow );
		panelScrollBar->winEnable( hasOverflow );
		GadgetSliderSetPosition( panelScrollBar, 100 );
		if (hasOverflow)
			bringWindowToFront( panelScrollBar );
	}
}
//-------------------------------------------------------------------------------------------------
/** Initialize the options menu */
//-------------------------------------------------------------------------------------------------
void OptionsMenuInit( WindowLayout *layout, void *userData )
{
	ignoreSelected = TRUE;
	if (TheGameEngine->getQuitting())
		return;
	OptionsLayout = layout;
	if (!pref)
	{
		pref = NEW OptionPreferences;
	}

	SignalUIInteraction(SHELL_SCRIPT_HOOK_OPTIONS_OPENED);

	comboBoxLANIPID				 = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxIP" ) );
	comboBoxLANIP					 = TheWindowManager->winGetWindowFromId( NULL,  comboBoxLANIPID);
	comboBoxOnlineIPID		 = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxOnlineIP" ) );
	comboBoxOnlineIP			 = TheWindowManager->winGetWindowFromId( NULL,  comboBoxOnlineIPID);
	checkAlternateMouseID  = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckAlternateMouse" ) );
	checkAlternateMouse	   = TheWindowManager->winGetWindowFromId( NULL, checkAlternateMouseID);
	checkRetaliationID		 = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:Retaliation" ) );
	checkRetaliation	     = TheWindowManager->winGetWindowFromId( NULL, checkRetaliationID);
	checkDoubleClickAttackMoveID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckDoubleClickAttackMove" ) );
	checkDoubleClickAttackMove   = TheWindowManager->winGetWindowFromId( NULL, checkDoubleClickAttackMoveID );
	sliderScrollSpeedID	   = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:SliderScrollSpeed" ) );
	sliderScrollSpeed		   = TheWindowManager->winGetWindowFromId( NULL,  sliderScrollSpeedID);
	comboBoxAntiAliasingID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxAntiAliasing" ) );
	comboBoxAntiAliasing   = TheWindowManager->winGetWindowFromId( NULL, comboBoxAntiAliasingID );
	comboBoxResolutionID   = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxResolution" ) );
	comboBoxResolution     = TheWindowManager->winGetWindowFromId( NULL, comboBoxResolutionID );
	comboBoxDetailID			 = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxDetail" ) );
	comboBoxDetail		   = TheWindowManager->winGetWindowFromId( NULL, comboBoxDetailID );

	checkLanguageFilterID  = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckLanguageFilter" ) );
	checkLanguageFilter    = TheWindowManager->winGetWindowFromId( NULL, checkLanguageFilterID );
	checkSendDelayID       = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckSendDelay" ) );
	checkSendDelay				 = TheWindowManager->winGetWindowFromId( NULL, checkSendDelayID);
	buttonFirewallRefreshID	= TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonFirewallRefresh" ) );
	buttonFirewallRefresh		= TheWindowManager->winGetWindowFromId( NULL, buttonFirewallRefreshID);
	checkDrawAnchorID       = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckBoxDrawAnchor" ) );
	checkDrawAnchor				 = TheWindowManager->winGetWindowFromId( NULL, checkDrawAnchorID);
	checkMoveAnchorID       = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckBoxMoveAnchor" ) );
	checkMoveAnchor				 = TheWindowManager->winGetWindowFromId( NULL, checkMoveAnchorID);

	// Replay camera
	checkSaveCameraID      = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckBoxSaveCamera" ) );
	checkSaveCamera        = TheWindowManager->winGetWindowFromId( NULL, checkSaveCameraID );
	checkUseCameraID       = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckBoxUseCamera" ) );
	checkUseCamera         = TheWindowManager->winGetWindowFromId( NULL, checkUseCameraID );

//	// Speakers and 3-D Audio
//	checkAudioSurroundID   = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckAudioSurround" ) );
//	checkAudioSurround     = TheWindowManager->winGetWindowFromId( NULL, checkAudioSurroundID );
//	checkAudioHardwareID   = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckAudioHardware" ) );
//	checkAudioHardware     = TheWindowManager->winGetWindowFromId( NULL, checkAudioHardwareID );
//
	// Volume Controls
	sliderMusicVolumeID    = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:SliderMusicVolume" ) );
	sliderMusicVolume      = TheWindowManager->winGetWindowFromId( NULL, sliderMusicVolumeID );
	sliderSFXVolumeID      = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:SliderSFXVolume" ) );
	sliderSFXVolume        = TheWindowManager->winGetWindowFromId( NULL, sliderSFXVolumeID );
	sliderVoiceVolumeID    = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:SliderVoiceVolume" ) );
	sliderVoiceVolume      = TheWindowManager->winGetWindowFromId( NULL, sliderVoiceVolumeID );
 	sliderGammaID    = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:SliderGamma" ) );
 	sliderGamma      = TheWindowManager->winGetWindowFromId( NULL, sliderGammaID );

//	checkBoxLowTextureDetailID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckLowTextureDetail" ) );
//	checkBoxLowTextureDetail      = TheWindowManager->winGetWindowFromId( NULL, checkBoxLowTextureDetailID );
	
	tabPanelAudioID         = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:TabPanelAudio" ) );
	tabPanelAudio           = TheWindowManager->winGetWindowFromId( NULL, tabPanelAudioID );
	tabPanelVideoID         = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:TabPanelVideo" ) );
	tabPanelVideo           = TheWindowManager->winGetWindowFromId( NULL, tabPanelVideoID );
	tabPanelDisplayID       = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:TabPanelDisplay" ) );
	tabPanelDisplay         = TheWindowManager->winGetWindowFromId( NULL, tabPanelDisplayID );
	tabPanelAccessibilityID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:TabPanelAccessibility" ) );
	tabPanelAccessibility   = TheWindowManager->winGetWindowFromId( NULL, tabPanelAccessibilityID );
	tabPanelKeyMappingID    = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:TabPanelKeyMapping" ) );
	tabPanelKeyMapping      = TheWindowManager->winGetWindowFromId( NULL, tabPanelKeyMappingID );
	tabPanelNetworkID       = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:TabPanelNetwork" ) );
	tabPanelNetwork         = TheWindowManager->winGetWindowFromId( NULL, tabPanelNetworkID );

	panelScrollBarID        = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:PanelScrollBar" ) );
	panelScrollBar          = TheWindowManager->winGetWindowFromId( NULL, panelScrollBarID );
	s_currentScrollPx       = 0;
	if (panelScrollBar) GadgetSliderSetPosition( panelScrollBar, 100 );
	if (panelScrollBar) bringWindowToFront( panelScrollBar );

	sliderTextureResolutionID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:LowResSlider" ) );
	sliderTextureResolution = TheWindowManager->winGetWindowFromId( NULL, sliderTextureResolutionID );

	check3DShadowsID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:Check3DShadows" ) );
	check3DShadows   = TheWindowManager->winGetWindowFromId( NULL, check3DShadowsID);

	check2DShadowsID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:Check2DShadows" ) );
	check2DShadows   = TheWindowManager->winGetWindowFromId( NULL, check2DShadowsID);

	checkCloudShadowsID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckCloudShadows" ) );
	checkCloudShadows   = TheWindowManager->winGetWindowFromId( NULL, checkCloudShadowsID);

	checkGroundLightingID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckGroundLighting" ) );
	checkGroundLighting   = TheWindowManager->winGetWindowFromId( NULL, checkGroundLightingID);

	checkSmoothWaterID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckSmoothWater" ) );
	checkSmoothWater   = TheWindowManager->winGetWindowFromId( NULL, checkSmoothWaterID);

	checkExtraAnimationsID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckExtraAnimations" ) );
	checkExtraAnimations   = TheWindowManager->winGetWindowFromId( NULL, checkExtraAnimationsID);

	checkNoDynamicLodID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckNoDynamicLOD" ) );
	checkNoDynamicLod   = TheWindowManager->winGetWindowFromId( NULL, checkNoDynamicLodID);

	checkHeatEffectsID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckHeatEffects" ) );
	checkHeatEffects   = TheWindowManager->winGetWindowFromId( NULL, checkHeatEffectsID);

	checkVSyncID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckVSync" ) );
	checkVSync   = TheWindowManager->winGetWindowFromId( NULL, checkVSyncID);

	checkFPSCounterID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckFPSCounter" ) );
	checkFPSCounter   = TheWindowManager->winGetWindowFromId( NULL, checkFPSCounterID);
	if (checkFPSCounter)
		checkFPSCounter->winSetText( UnicodeString( L"Show FPS Counter" ) );

	GameWindow *buttonTabAudio = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonTabAudio") );
	GameWindow *buttonTabVideo = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonTabVideo") );
	GameWindow *buttonTabDisplay = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonTabDisplay") );
	GameWindow *buttonTabAccessibility = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonTabAccessibility") );
	GameWindow *buttonTabKeyMapping = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonTabKeyMapping") );
	GameWindow *buttonTabNetwork = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonTabNetwork") );
	GameWindow *buttonAccept = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonAccept") );
	GameWindow *buttonBack = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonBack") );
	GameWindow *buttonDefaults = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonDefaults") );
	SetWindowTextSafe( buttonTabAudio, L"Audio" );
	SetWindowTextSafe( buttonTabVideo, L"Video" );
	SetWindowTextSafe( buttonTabDisplay, L"Display" );
	SetWindowTextSafe( buttonTabAccessibility, L"Accessibilities" );
	SetWindowTextSafe( buttonTabNetwork, L"Network" );
	if (buttonTabAudio)         buttonTabAudio->winSetTooltip( UnicodeString(L"Adjust music, effects, voice and output settings") );
	if (buttonTabVideo)         buttonTabVideo->winSetTooltip( UnicodeString(L"Choose graphics quality and visual settings") );
	if (buttonTabDisplay)       buttonTabDisplay->winSetTooltip( UnicodeString(L"Configure interface, HUD and display presentation") );
	if (buttonTabAccessibility) buttonTabAccessibility->winSetTooltip( UnicodeString(L"Adjust readability, comfort and accessibility options") );
	if (buttonTabKeyMapping)
	{
		buttonTabKeyMapping->winHide( TRUE );
		buttonTabKeyMapping->winEnable( FALSE );
	}
	if (buttonTabNetwork)       buttonTabNetwork->winSetTooltip( UnicodeString(L"Set LAN, firewall and online connection options") );
	SetWindowTextSafe( buttonAccept, L"Save" );
	SetWindowTextSafe( buttonBack, L"Cancel" );
	SetWindowTextSafe( buttonDefaults, L"Defaults" );
	if (buttonAccept)   buttonAccept->winSetTooltip( UnicodeString(L"Save your changes and apply them now") );
	if (buttonBack)     buttonBack->winSetTooltip( UnicodeString(L"Close the menu without saving") );
	if (buttonDefaults) buttonDefaults->winSetTooltip( UnicodeString(L"Restore the recommended default settings") );
	GameWindow *buttonKeyboardOptions = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ButtonKeyboardOptions") );
	SetWindowTextSafe( buttonKeyboardOptions, L"Keyboard Shortcuts" );
	if (buttonKeyboardOptions) buttonKeyboardOptions->winSetTooltip( UnicodeString(L"Open the full shortcut browser and remap any command") );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:KeymapTitle") ), L"Keyboard Layout Guide" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:KeymapHelpText") ), L"Read the rows below like a QWERTY keyboard, then open Keyboard Shortcuts to remap any binding." );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:LabelVersion") ), L"Version" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:UIScaleLabel") ), L"UI Scale" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:VoiceResponseLabel") ), L"Voice Response Frequency" );
	SetWindowTextSafe( checkDrawAnchor, L"Draw RMB Scroll Anchor" );
	if (checkDrawAnchor) checkDrawAnchor->winSetTooltip( UnicodeString(L"Show a visible anchor marker when using right-mouse-button scrolling") );
	SetWindowTextSafe( checkMoveAnchor, L"Move RMB Scroll Anchor" );
	if (checkMoveAnchor) checkMoveAnchor->winSetTooltip( UnicodeString(L"Move the camera toward the RMB anchor marker while scrolling") );
	SetWindowTextSafe( checkSaveCamera, L"Replay" );
	if (checkSaveCamera) checkSaveCamera->winSetTooltip( UnicodeString(L"Turn replay camera saving and playback on or off") );
	if (checkUseCamera)
	{
		checkUseCamera->winHide( TRUE );
		checkUseCamera->winEnable( FALSE );
	}

	checkFPSLimitID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckFPSLimit" ) );
	checkFPSLimit   = TheWindowManager->winGetWindowFromId( NULL, checkFPSLimitID );
	if (checkFPSLimit) checkFPSLimit->winSetTooltip( UnicodeString(L"Limit the game frame rate to 30 FPS to reduce CPU usage") );
	if (checkVSync) checkVSync->winSetTooltip( UnicodeString(L"Synchronise the game to your monitor refresh rate to reduce tearing") );
	if (checkFPSCounter) checkFPSCounter->winSetTooltip( UnicodeString(L"Show a real-time FPS counter while playing") );
	if (sliderGamma) sliderGamma->winSetTooltip( UnicodeString(L"Adjust the overall screen brightness and gamma") );
	if (comboBoxResolution) comboBoxResolution->winSetTooltip( UnicodeString(L"Choose the game rendering resolution") );
	if (comboBoxAntiAliasing) comboBoxAntiAliasing->winSetTooltip( UnicodeString(L"Choose display mode: Fullscreen, Borderless Windowed, or Windowed") );
	if (comboBoxDetail) comboBoxDetail->winSetTooltip( UnicodeString(L"Select the overall graphics detail preset") );
	comboBoxUIScaleID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxUIScale" ) );
	comboBoxUIScale   = TheWindowManager->winGetWindowFromId( NULL, comboBoxUIScaleID );
	if (comboBoxUIScale) comboBoxUIScale->winSetTooltip( UnicodeString(L"Scale the menu and HUD readability: 75%, 100%, 125% or 150%") );
	comboBoxVoiceFrequencyID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxVoiceFrequency" ) );
	comboBoxVoiceFrequency   = TheWindowManager->winGetWindowFromId( NULL, comboBoxVoiceFrequencyID );
	if (comboBoxVoiceFrequency) comboBoxVoiceFrequency->winSetTooltip( UnicodeString(L"How often units answer with voice lines: Off, Occasional, Normal or Always") );
	sliderMasterVolumeID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:SliderMasterVolume" ) );
	sliderMasterVolume   = TheWindowManager->winGetWindowFromId( NULL, sliderMasterVolumeID );
	if (sliderMasterVolume) sliderMasterVolume->winSetTooltip( UnicodeString(L"Overall mix level applied to music, effects, and voices") );
	comboBoxSpeakerTypeID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxSpeakerType" ) );
	comboBoxSpeakerType   = TheWindowManager->winGetWindowFromId( NULL, comboBoxSpeakerTypeID );
	if (comboBoxSpeakerType) comboBoxSpeakerType->winSetTooltip( UnicodeString(L"Choose stereo, headphones, surround, 4-speaker, 5.1, or 7.1 output") );
	GameWindow *detailLabel = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:DetailLabel") );
	if (detailLabel) detailLabel->winSetTooltip( UnicodeString(L"Select the overall graphics detail preset") );
	GameWindow *speakerTypeLabel = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:SpeakerTypeLabel") );
	if (speakerTypeLabel) speakerTypeLabel->winSetTooltip( UnicodeString(L"Choose stereo, headphones, surround, 4-speaker, 5.1, or 7.1 output") );

	comboBoxTextureQualityID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxTextureQuality" ) );
	comboBoxTextureQuality   = TheWindowManager->winGetWindowFromId( NULL, comboBoxTextureQualityID );
	comboBoxParticleQualityID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxParticleQuality" ) );
	comboBoxParticleQuality   = TheWindowManager->winGetWindowFromId( NULL, comboBoxParticleQualityID );
	comboBoxUIThemeColorID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ComboBoxUIThemeColor" ) );
	comboBoxUIThemeColor   = TheWindowManager->winGetWindowFromId( NULL, comboBoxUIThemeColorID );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:MusicVolumeLabel") ),    L"Music Volume" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:SFXVolumeLabel") ),      L"Effects Volume" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:VoiceVolumeLabel") ),    L"Voice Volume" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:MasterVolumeLabel") ),   L"Master Volume" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:SpeakerTypeLabel") ),    L"Audio Output" );
	GameWindow *audioOutputNote = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:AudioOutputNote") );
	SetWindowTextSafe( audioOutputNote, L"" );
	if (audioOutputNote) audioOutputNote->winHide( TRUE );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:DetailLabel") ),         L"Detail Level" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ResolutionLabel") ),     L"Resolution" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:GammaLabel") ),          L"Gamma" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:TextureQualityLabel") ), L"Texture Quality" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:ParticleQualityLabel") ), L"Particle Quality" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:UIThemeColorLabel") ), L"UI Theme Color" );
	if (checkVSync)
	{
		checkVSync->winSetText( UnicodeString( L"VSync" ) );
		checkVSync->winSetTooltip( UnicodeString(L"Synchronise the game to your monitor refresh rate to reduce tearing") );
	}
	if (checkFPSLimit)       checkFPSLimit->winSetText( UnicodeString( L"Enable FPS Limit" ) );
	if (checkFPSCounter)
	{
		checkFPSCounter->winSetTooltip( UnicodeString(L"Show a real-time FPS counter while playing") );
	}
	if (checkLanguageFilter) checkLanguageFilter->winSetText( UnicodeString( L"Language Filter" ) );
	if (checkLanguageFilter) checkLanguageFilter->winSetTooltip( UnicodeString( L"Filter profanity and offensive language from chat" ) );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:LabelTopComment") ),  L"Esc = Back" );
	SetWindowTextSafe( TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("OptionsMenu.wnd:DisplayModeLabel") ), L"Display Mode" );
	if (comboBoxTextureQuality) comboBoxTextureQuality->winSetTooltip( UnicodeString(L"Override texture resolution: High (best), Medium, or Low (saves memory)") );
	if (comboBoxParticleQuality) comboBoxParticleQuality->winSetTooltip( UnicodeString(L"Maximum particles on screen: Low (600), Medium (2000), High (5000)") );
	if (comboBoxUIThemeColor) comboBoxUIThemeColor->winSetTooltip( UnicodeString(L"UI accent colour theme - takes effect after restarting the game") );

	checkBuildingOcclusionID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckBehindBuilding" ) );
	checkBuildingOcclusion   = TheWindowManager->winGetWindowFromId( NULL, checkBuildingOcclusionID);

	checkPropsID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:CheckShowProps" ) );
	checkProps   = TheWindowManager->winGetWindowFromId( NULL, checkPropsID);

	sliderParticleCapID = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ParticleCapSlider" ) );
  sliderParticleCap = TheWindowManager->winGetWindowFromId( NULL, sliderParticleCapID );

	// Bring Display tab comboboxes to the end of their sibling list so their
	// dropdowns render on top of all sibling controls (labels, checkboxes, sliders).
	// Order: topmost-y combobox last = rendered on top when its dropdown is open.
	bringWindowToFront( comboBoxDetail );
	bringWindowToFront( comboBoxUIThemeColor );
	bringWindowToFront( comboBoxParticleQuality );
	bringWindowToFront( comboBoxTextureQuality );
	bringWindowToFront( comboBoxUIScale );
	bringWindowToFront( comboBoxSpeakerType );
	bringWindowToFront( comboBoxAntiAliasing );
	bringWindowToFront( comboBoxResolution );

	showTab(0);

	Color color =  GameMakeColor(255,255,255,255);

  enum AliasingMode
  {
    OFF = 0,
    LOW,
    HIGH,
    NUM_ALIASING_MODES
  };

	NameKeyType versionID = TheNameKeyGenerator->nameToKey( AsciiString("OptionsMenu.wnd:LabelVersion") );
	GameWindow *labelVersion = TheWindowManager->winGetWindowFromId( NULL, versionID );
	UnicodeString versionString;
	versionString.format(TheGameText->fetch("Version:Format2").str(), (GetRegistryVersion() >> 16), (GetRegistryVersion() & 0xffff));
	
	if (labelVersion != NULL && TheVersion && TheVersion->showFullVersion())
	{
		UnicodeString version;
		version.format(L"(%s) %s -- %s", versionString.str(), TheVersion->getFullUnicodeVersion().str(), TheVersion->getUnicodeBuildTime().str());
		GadgetStaticTextSetText( labelVersion, version );
	}
	else if (labelVersion != NULL && TheVersion)
	{
		GadgetStaticTextSetText( labelVersion, versionString );
	}
	else
	{
		if (labelVersion) labelVersion->winHide( TRUE );
	}


	// Choose an IP address, then initialize the IP combo box
	UnsignedInt selectedIP = pref->getLANIPAddress();

	UnicodeString str;
	IPEnumeration IPs;
	EnumeratedIP *IPlist = IPs.getAddresses();
	Int index;
	Int selectedIndex = -1;
	Int count = 0;
	if (comboBoxLANIP)
	{
		GadgetComboBoxReset(comboBoxLANIP);
		while (IPlist)
		{
			count++;
			str.translate(IPlist->getIPstring());
			index = GadgetComboBoxAddEntry(comboBoxLANIP, str, color);
			if (index >= 0)
			{
				GadgetComboBoxSetItemData(comboBoxLANIP, index, reinterpret_cast<void*>(static_cast<uintptr_t>(IPlist->getIP())));
				if (selectedIP == IPlist->getIP())
				{
					selectedIndex = index;
				}
			}
			IPlist = IPlist->getNext();
		}
		if (selectedIndex >= 0)
		{
			GadgetComboBoxSetSelectedPos(comboBoxLANIP, selectedIndex);
		}
		else
		{
			GadgetComboBoxSetSelectedPos(comboBoxLANIP, 0);
			if (IPs.getAddresses())
			{
				pref->setLANIPAddress(IPs.getAddresses()->getIPstring());
			}
		}
	}

	// And now the GameSpy one
	if (comboBoxOnlineIP)
	{
		UnsignedInt selectedIP = pref->getOnlineIPAddress();
		UnicodeString str;
		IPEnumeration IPs;
		EnumeratedIP *IPlist = IPs.getAddresses();
		Int index;
		Int selectedIndex = -1;
		Int count = 0;
		GadgetComboBoxReset(comboBoxOnlineIP);
		while (IPlist)
		{
			count++;
			str.translate(IPlist->getIPstring());
			index = GadgetComboBoxAddEntry(comboBoxOnlineIP, str, color);
			GadgetComboBoxSetItemData(comboBoxOnlineIP, index, reinterpret_cast<void*>(static_cast<uintptr_t>(IPlist->getIP())));
			if (selectedIP == IPlist->getIP())
			{
				selectedIndex = index;
			}
			IPlist = IPlist->getNext();
		}
		if (selectedIndex >= 0)
		{
			GadgetComboBoxSetSelectedPos(comboBoxOnlineIP, selectedIndex);
		}
		else
		{
			GadgetComboBoxSetSelectedPos(comboBoxOnlineIP, 0);
			if (IPs.getAddresses())
			{
				pref->setOnlineIPAddress(IPs.getAddresses()->getIPstring());
			}
		}
	}

	// HTTP Proxy
	GameWindow *textEntryHTTPProxy = TheWindowManager->winGetWindowFromId(NULL, NAMEKEY("OptionsMenu.wnd:TextEntryHTTPProxy"));
	if (textEntryHTTPProxy)
	{
		UnicodeString uStr;
		std::string proxy;
		GetStringFromRegistry("", "Proxy", proxy);
		uStr.translate(proxy.c_str());
		GadgetTextEntrySetText(textEntryHTTPProxy, uStr);
	}

	// Firewall Port Override
	GameWindow *textEntryFirewallPortOverride = TheWindowManager->winGetWindowFromId(NULL, NAMEKEY("OptionsMenu.wnd:TextEntryFirewallPortOverride"));
	if (textEntryFirewallPortOverride)
	{
			UnicodeString uStr;
			if (TheGlobalData->m_firewallPortOverride != 0)
			{	AsciiString aStr;
				aStr.format("%d",TheGlobalData->m_firewallPortOverride);
				uStr.translate(aStr);
			}
			GadgetTextEntrySetText(textEntryFirewallPortOverride,uStr);
	}

	// populate display mode selector (Fullscreen=0, Borderless Windowed=1, Windowed=2)
	Int i;
	if (comboBoxAntiAliasing)
	{
		GadgetComboBoxReset(comboBoxAntiAliasing);
		static const wchar_t * const displayModeNames[] = { L"Fullscreen", L"Borderless Windowed", L"Windowed" };
		for (i = 0; i < 3; ++i)
		{
			str = displayModeNames[i];
			GadgetComboBoxAddEntry(comboBoxAntiAliasing, str, color);
		}
		SetComboBoxSelectedPosSafe(comboBoxAntiAliasing, pref->getDisplayMode());
	}

	// get resolution from saved preferences file
	AsciiString selectedResolution = (*pref) ["Resolution"];
	Int selectedXRes=800,selectedYRes=600;
	Int selectedResIndex=-1;
	Int defaultResIndex=0;	//index of default video mode that should always exist
	if (!selectedResolution.isEmpty())
	{	//try to parse 2 integers out of string
		if (sscanf(selectedResolution.str(),"%d%d", &selectedXRes, &selectedYRes) != 2)
		{	selectedXRes=800; selectedYRes=600;
		}
	}

	// populate resolution modes
	if (comboBoxResolution)
		GadgetComboBoxReset(comboBoxResolution);
	Int numResolutions = TheDisplay->getDisplayModeCount();
	for( i = 0; i < numResolutions; ++i )
	{	Int xres,yres,bitDepth;
		TheDisplay->getDisplayModeDescription(i,&xres,&yres,&bitDepth);
		str.format(L"%d x %d",xres,yres);
		if (comboBoxResolution)
			GadgetComboBoxAddEntry( comboBoxResolution, str, color);
		if (xres == 800 && yres == 600)	//keep track of default mode in case we need it.
			defaultResIndex=i;
		if (xres == selectedXRes && yres == selectedYRes)
			selectedResIndex=i;
	}

	if (selectedResIndex == -1)	//check if saved mode no longer available
	{	//pick default resolution
		selectedXRes = 800;
		selectedXRes = 600;
		selectedResIndex = defaultResIndex;
	}

	TheWritableGlobalData->m_xResolution = selectedXRes;
	TheWritableGlobalData->m_yResolution = selectedYRes;

	SetComboBoxSelectedPosSafe( comboBoxResolution, selectedResIndex );

	// set the display detail
	if (comboBoxDetail)
	{
		GadgetComboBoxReset(comboBoxDetail);
		GadgetComboBoxAddEntry(comboBoxDetail, TheGameText->fetch("GUI:High"), color);
		GadgetComboBoxAddEntry(comboBoxDetail, TheGameText->fetch("GUI:Medium"), color);
		GadgetComboBoxAddEntry(comboBoxDetail, TheGameText->fetch("GUI:Low"), color);
		GadgetComboBoxAddEntry(comboBoxDetail, TheGameText->fetch("GUI:Custom"), color);
	}

	//Check if level was never set and default to setting most suitable for system.
	if (TheGameLODManager->getStaticLODLevel() == STATIC_GAME_LOD_UNKNOWN)
		TheGameLODManager->setStaticLODLevel(TheGameLODManager->findStaticLODLevel());

	switch (TheGameLODManager->getStaticLODLevel())
	{
	case STATIC_GAME_LOD_LOW:
		SetComboBoxSelectedPosSafe(comboBoxDetail, LOWDETAIL);
		break;
	case STATIC_GAME_LOD_MEDIUM:
		SetComboBoxSelectedPosSafe(comboBoxDetail, MEDIUMDETAIL);
		break;
	case STATIC_GAME_LOD_HIGH:
		SetComboBoxSelectedPosSafe(comboBoxDetail, HIGHDETAIL);
		break;
	case STATIC_GAME_LOD_CUSTOM:
		SetComboBoxSelectedPosSafe(comboBoxDetail, CUSTOMDETAIL);
		break;
	default:
		DEBUG_ASSERTCRASH(FALSE,("Tried to set comboBoxDetail to a value of %d ", TheGameLODManager->getStaticLODLevel()) );
	};

	Int txtFact=TheGameLODManager->getCurrentTextureReduction();

	SetSliderPositionSafe( sliderTextureResolution, 2-txtFact);

	SetCheckBoxCheckedSafe( check3DShadows, TheGlobalData->m_useShadowVolumes);

	SetCheckBoxCheckedSafe( check2DShadows, TheGlobalData->m_useShadowDecals);

	SetCheckBoxCheckedSafe( checkCloudShadows, TheGlobalData->m_useCloudMap);

	SetCheckBoxCheckedSafe( checkGroundLighting, TheGlobalData->m_useLightMap);

	SetCheckBoxCheckedSafe( checkSmoothWater, TheGlobalData->m_showSoftWaterEdge);

	SetCheckBoxCheckedSafe( checkExtraAnimations, !TheGlobalData->m_useDrawModuleLOD);

	SetCheckBoxCheckedSafe( checkNoDynamicLod, !TheGlobalData->m_enableDynamicLOD);

	SetCheckBoxCheckedSafe( checkHeatEffects, TheGlobalData->m_useHeatEffects);

	if (checkVSync) {
		GadgetCheckBoxSetChecked( checkVSync, TheGlobalData->m_vsync);
		GadgetCheckBoxSetText( checkVSync, UnicodeString(L"Enable VSync") );
	}
	if (checkFPSLimit) {
		GadgetCheckBoxSetChecked( checkFPSLimit, pref->getFPSLimitEnabled() );
		GadgetCheckBoxSetText( checkFPSLimit, UnicodeString(L"Enable FPS Limit") );
	}
	if (checkSaveCamera) {
		GadgetCheckBoxSetText( checkSaveCamera, UnicodeString(L"Replay") );
	}
	if (checkUseCamera) {
		GadgetCheckBoxSetChecked( checkUseCamera, FALSE );
		checkUseCamera->winHide( TRUE );
		checkUseCamera->winEnable( FALSE );
	}
	if (comboBoxUIScale)
	{
		Int uiScaleIndex = 1;
		AsciiString uiScalePref = (*pref)["UIScale"];
		GadgetComboBoxReset( comboBoxUIScale );
		GadgetComboBoxAddEntry( comboBoxUIScale, UnicodeString( L"Minimal (75%)" ), color );
		GadgetComboBoxAddEntry( comboBoxUIScale, UnicodeString( L"Classic (100%)" ), color );
		GadgetComboBoxAddEntry( comboBoxUIScale, UnicodeString( L"Large (125%)" ), color );
		GadgetComboBoxAddEntry( comboBoxUIScale, UnicodeString( L"Largest (150%)" ), color );
		if (uiScalePref.compareNoCase("75") == 0 || uiScalePref.compareNoCase("90") == 0 || uiScalePref.compareNoCase("minimal") == 0 || uiScalePref.compareNoCase("compact") == 0)
			uiScaleIndex = 0;
		else if (uiScalePref.compareNoCase("125") == 0 || uiScalePref.compareNoCase("110") == 0 || uiScalePref.compareNoCase("large") == 0)
			uiScaleIndex = 2;
		else if (uiScalePref.compareNoCase("150") == 0 || uiScalePref.compareNoCase("largest") == 0)
			uiScaleIndex = 3;
		GadgetComboBoxSetSelectedPos( comboBoxUIScale, uiScaleIndex );
	}
	if (comboBoxVoiceFrequency)
	{
		Int voiceFreqIndex = 2;
		AsciiString voiceFreqPref = (*pref)["VoiceResponseFrequency"];
		GadgetComboBoxReset( comboBoxVoiceFrequency );
		GadgetComboBoxAddEntry( comboBoxVoiceFrequency, UnicodeString( L"Off" ), color );
		GadgetComboBoxAddEntry( comboBoxVoiceFrequency, UnicodeString( L"Occasional" ), color );
		GadgetComboBoxAddEntry( comboBoxVoiceFrequency, UnicodeString( L"Normal" ), color );
		GadgetComboBoxAddEntry( comboBoxVoiceFrequency, UnicodeString( L"Always" ), color );
		if (voiceFreqPref.compareNoCase("off") == 0 || voiceFreqPref.compareNoCase("none") == 0)
			voiceFreqIndex = 0;
		else if (voiceFreqPref.compareNoCase("occasional") == 0 || voiceFreqPref.compareNoCase("low") == 0)
			voiceFreqIndex = 1;
		else if (voiceFreqPref.compareNoCase("always") == 0 || voiceFreqPref.compareNoCase("high") == 0)
			voiceFreqIndex = 3;
		GadgetComboBoxSetSelectedPos( comboBoxVoiceFrequency, voiceFreqIndex );
	}
	if (comboBoxSpeakerType)
	{
		Int speakerTypeIndex = getSpeakerTypeComboIndex( pref->getSpeakerType() );
		GadgetComboBoxReset( comboBoxSpeakerType );
		GadgetComboBoxAddEntry( comboBoxSpeakerType, UnicodeString( L"Stereo / 2.1" ), color );
		GadgetComboBoxAddEntry( comboBoxSpeakerType, UnicodeString( L"Headphones" ), color );
		GadgetComboBoxAddEntry( comboBoxSpeakerType, UnicodeString( L"Surround" ), color );
		GadgetComboBoxAddEntry( comboBoxSpeakerType, UnicodeString( L"4 Speakers" ), color );
		GadgetComboBoxAddEntry( comboBoxSpeakerType, UnicodeString( L"5.1 Surround" ), color );
		GadgetComboBoxAddEntry( comboBoxSpeakerType, UnicodeString( L"7.1 Surround" ), color );
		GadgetComboBoxSetSelectedPos( comboBoxSpeakerType, speakerTypeIndex );
	}
	if (comboBoxTextureQuality)
	{
		Int texReduction = pref->getTextureReduction();
		if (texReduction < 0) texReduction = 0;
		if (texReduction > 2) texReduction = 2;
		GadgetComboBoxReset( comboBoxTextureQuality );
		GadgetComboBoxAddEntry( comboBoxTextureQuality, UnicodeString( L"High" ), color );
		GadgetComboBoxAddEntry( comboBoxTextureQuality, UnicodeString( L"Medium" ), color );
		GadgetComboBoxAddEntry( comboBoxTextureQuality, UnicodeString( L"Low" ), color );
		GadgetComboBoxSetSelectedPos( comboBoxTextureQuality, texReduction );
	}
	if (comboBoxParticleQuality)
	{
		Int particleQualityIdx = 2;
		Int particleCap = pref->getParticleCap();
		if (particleCap < 1000) particleQualityIdx = 0;
		else if (particleCap < 3000) particleQualityIdx = 1;
		GadgetComboBoxReset( comboBoxParticleQuality );
		GadgetComboBoxAddEntry( comboBoxParticleQuality, UnicodeString( L"Low" ), color );
		GadgetComboBoxAddEntry( comboBoxParticleQuality, UnicodeString( L"Medium" ), color );
		GadgetComboBoxAddEntry( comboBoxParticleQuality, UnicodeString( L"High" ), color );
		GadgetComboBoxSetSelectedPos( comboBoxParticleQuality, particleQualityIdx );
	}
	if (comboBoxUIThemeColor)
	{
		Int themeIdx = 0;
		OptionPreferences::const_iterator themeIt = pref->find("UITheme");
		if (themeIt != pref->end())
			themeIdx = atoi(themeIt->second.str());
		if (themeIdx < 0 || themeIdx > 3) themeIdx = 0;
		GadgetComboBoxReset( comboBoxUIThemeColor );
		GadgetComboBoxAddEntry( comboBoxUIThemeColor, UnicodeString( L"Blue" ), color );
		GadgetComboBoxAddEntry( comboBoxUIThemeColor, UnicodeString( L"Gold" ), color );
		GadgetComboBoxAddEntry( comboBoxUIThemeColor, UnicodeString( L"Green" ), color );
		GadgetComboBoxAddEntry( comboBoxUIThemeColor, UnicodeString( L"Red" ), color );
		GadgetComboBoxSetSelectedPos( comboBoxUIThemeColor, themeIdx );
	}

	SetCheckBoxCheckedSafe( checkBuildingOcclusion, TheGlobalData->m_enableBehindBuildingMarkers);

	SetCheckBoxCheckedSafe( checkProps, TheGlobalData->m_useTrees);
	//checkProps->winEnable(false);	//gray out the option for now.

	SetSliderPositionSafe( sliderParticleCap, TheGlobalData->m_maxParticleCount );

	//set language filter
	AsciiString languageFilter = (*pref)["LanguageFilter"];
	if (languageFilter == "true" )
	{
		SetCheckBoxCheckedSafe( checkLanguageFilter, true);
		TheWritableGlobalData->m_languageFilterPref = true;
	}
	else
	{
		SetCheckBoxCheckedSafe( checkLanguageFilter, false);
		TheWritableGlobalData->m_languageFilterPref = false;
	}
	
	{
		Bool replayEnabled = (pref->saveCameraInReplays() || pref->useCameraInReplays()) ? TRUE : FALSE;
		SetCheckBoxCheckedSafe( checkSaveCamera, replayEnabled );
		SetCheckBoxCheckedSafe( checkUseCamera, replayEnabled );
		TheWritableGlobalData->m_saveCameraInReplay = replayEnabled;
		TheWritableGlobalData->m_useCameraInReplay = replayEnabled;
	}

	//set scroll options
	AsciiString test = (*pref)["DrawScrollAnchor"];
	if (test == "Yes" || (test.isEmpty() && TheInGameUI->getDrawRMBScrollAnchor()))
	{
		SetCheckBoxCheckedSafe( checkDrawAnchor, true );
		TheInGameUI->setDrawRMBScrollAnchor(true);
	}
	else
	{
		SetCheckBoxCheckedSafe( checkDrawAnchor, false );
		TheInGameUI->setDrawRMBScrollAnchor(false);
	}
	test = (*pref)["MoveScrollAnchor"];
	if (test == "Yes" || (test.isEmpty() && TheInGameUI->getMoveRMBScrollAnchor()))
	{
		SetCheckBoxCheckedSafe( checkMoveAnchor, true );
		TheInGameUI->setMoveRMBScrollAnchor(true);
	}
	else
	{
		SetCheckBoxCheckedSafe( checkMoveAnchor, false );
		TheInGameUI->setMoveRMBScrollAnchor(false);
	}

//	// Audio Init shiznat
//	GadgetCheckBoxSetChecked(checkAudioHardware, TheAudio->getHardwareAccelerated());
//	GadgetCheckBoxSetChecked(checkAudioSurround, TheAudio->getSpeakerSurround());

	// set the mouse mode
	SetCheckBoxCheckedSafe( checkAlternateMouse, TheGlobalData->m_useAlternateMouse );
	SetCheckBoxCheckedSafe( checkRetaliation, TheGlobalData->m_clientRetaliationModeEnabled );
	SetCheckBoxCheckedSafe( checkDoubleClickAttackMove, TheGlobalData->m_doubleClickAttackMove );

	// set scroll speed slider
	Int scrollPos = (Int)(TheGlobalData->m_keyboardScrollFactor*100.0f);
	SetSliderPositionSafe( sliderScrollSpeed, scrollPos );

	// set the send delay check box
	SetCheckBoxCheckedSafe( checkSendDelay, TheGlobalData->m_firewallSendDelay );

 	// set volume sliders
	Int masterVolumePercent = getMasterVolumePercent();
	SetSliderPositionSafe( sliderMasterVolume, masterVolumePercent );

	// set music volume slider
	SetSliderPositionSafe( sliderMusicVolume, normalizeChannelVolumeForDisplay( pref->getMusicVolume(), masterVolumePercent ) );

	//set SFX volume slider
	Real maxVolume = MAX( pref->getSoundVolume(), pref->get3DSoundVolume() );
	SetSliderPositionSafe( sliderSFXVolume, normalizeChannelVolumeForDisplay( maxVolume, masterVolumePercent ) );

	//set voice volume slider
	SetSliderPositionSafe( sliderVoiceVolume, normalizeChannelVolumeForDisplay( pref->getSpeechVolume(), masterVolumePercent ) );
	SetComboBoxSelectedPosSafe( comboBoxSpeakerType, getSpeakerTypeComboIndex( pref->getSpeakerType() ) );
	
	// set the gamma slider
 	SetSliderPositionSafe( sliderGamma, REAL_TO_INT(pref->getGammaValue()) );

	applyResolutionAwareOptionsScale( layout );

	// show menu
	layout->hide( FALSE );

	// set keyboard focus to main parent
	AsciiString parentName( "OptionsMenu.wnd:OptionsMenuParent" );
	NameKeyType parentID = TheNameKeyGenerator->nameToKey( parentName );
	GameWindow *parent = TheWindowManager->winGetWindowFromId( NULL, parentID );
	TheWindowManager->winSetFocus( parent );
	
	if( (TheGameLogic->isInGame() && TheGameLogic->getGameMode() != GAME_SHELL) || TheGameSpyInfo )
	{
		// disable controls that you can't change the options for in game
		if (comboBoxLANIP)
			comboBoxLANIP->winEnable(FALSE);
		if (comboBoxOnlineIP)
			comboBoxOnlineIP->winEnable(FALSE);
		if (checkSendDelay)
			checkSendDelay->winEnable(FALSE);
		if (buttonFirewallRefresh)
			buttonFirewallRefresh->winEnable(FALSE);

		if (comboBoxDetail)
			comboBoxDetail->winEnable(FALSE);


		if (comboBoxResolution)
			comboBoxResolution->winEnable(FALSE);

//		if (checkAudioSurround)
//			checkAudioSurround->winEnable(FALSE);
//
//		if (checkAudioHardware)
//			checkAudioHardware->winEnable(FALSE);
	}


	GameWindow *modalRoot = layout ? layout->getFirstWindow() : parent;
	if (modalRoot != NULL)
		TheWindowManager->winSetModal(modalRoot);
	ignoreSelected = FALSE;
}  // end OptionsMenuInit

//-------------------------------------------------------------------------------------------------
/** options menu shutdown method */
//-------------------------------------------------------------------------------------------------
void OptionsMenuShutdown( WindowLayout *layout, void *userData )
{
/* moved into the back button stuff
	if (pref)
	{
		pref->write();
		delete pref;
		pref = NULL;
	}

	comboBoxIP = NULL;

	// hide menu
	layout->hide( TRUE );

	// our shutdown is complete
	TheShell->shutdownComplete( layout );
*/

}  // end OptionsMenuShutdown

//-------------------------------------------------------------------------------------------------
/** options menu update method */
//-------------------------------------------------------------------------------------------------
void OptionsMenuUpdate( WindowLayout *layout, void *userData )
{

}  // end OptionsMenuUpdate

//-------------------------------------------------------------------------------------------------
/** Options menu input callback */
//-------------------------------------------------------------------------------------------------
WindowMsgHandledType OptionsMenuInput( GameWindow *window, UnsignedInt msg,
																			 WindowMsgData mData1, WindowMsgData mData2 )
{

	switch( msg ) 
	{

		// --------------------------------------------------------------------------------------------
		case GWM_CHAR:
		{
			UnsignedByte key = mData1;
			UnsignedByte state = mData2;

			switch( key )
			{

				// ----------------------------------------------------------------------------------------
				case KEY_ESC:
				{
					
					//
					// send a simulated selected event to the parent window of the
					// back/exit button
					//
					if( BitTest( state, KEY_STATE_UP ) )
					{
						AsciiString buttonName( "OptionsMenu.wnd:ButtonBack" );
						NameKeyType buttonID = TheNameKeyGenerator->nameToKey( buttonName );
						GameWindow *button = TheWindowManager->winGetWindowFromId( NULL, buttonID );
						GameWindow *target = window;
						if( button != NULL && button->winGetParent() != NULL )
							target = button->winGetParent();

						if( button != NULL && target != NULL )
						{
							TheWindowManager->winSendSystemMsg( target, GBM_SELECTED,
								(WindowMsgData)button, buttonID );
						}
						else if( OptionsLayout != NULL )
						{
							DestroyOptionsLayout();
						}
					}

					return MSG_HANDLED;

				}  // end escape

			}  // end switch( key )

		}  // end char

	// -----------------------------------------------------------------------
	case GWM_WHEEL_UP:
	case GWM_WHEEL_DOWN:
	{
		if (s_maxScrollPx <= 0 || panelScrollBar == NULL)
			return MSG_IGNORED;

		const Int step = 18;
		Int newOffset = s_currentScrollPx + (msg == GWM_WHEEL_DOWN ? step : -step);
		if (newOffset < 0) newOffset = 0;
		if (newOffset > s_maxScrollPx) newOffset = s_maxScrollPx;
		if (newOffset == s_currentScrollPx)
			return MSG_IGNORED;

		Int delta = newOffset - s_currentScrollPx;
		GameWindow *panel = getTabPanel(activeTab);
		if (panel != NULL)
		{
			GameWindow *child = panel->winGetChild();
			while (child != NULL)
			{
				Int cx, cy;
				child->winGetPosition(&cx, &cy);
				child->winSetPosition(cx, cy - delta);
				child = child->winGetNext();
			}
		}

		s_currentScrollPx = newOffset;
		Int sliderPos = 100 - (s_currentScrollPx * 100 / s_maxScrollPx);
		GadgetSliderSetPosition(panelScrollBar, sliderPos);
		bringWindowToFront(panelScrollBar);
		return MSG_HANDLED;
	}  // end wheel

	}  // end switch( msg )

	return MSG_IGNORED;

}  // end OptionsMenuInput

//-------------------------------------------------------------------------------------------------
/** options menu window system callback */
//-------------------------------------------------------------------------------------------------
WindowMsgHandledType OptionsMenuSystem( GameWindow *window, UnsignedInt msg, 
																				WindowMsgData mData1, WindowMsgData mData2 )
{
	static NameKeyType buttonBack = NAMEKEY_INVALID;
	static NameKeyType buttonDefaults = NAMEKEY_INVALID;
	static NameKeyType buttonAccept = NAMEKEY_INVALID;
	static NameKeyType buttonKeyboardOptionsMenu = NAMEKEY_INVALID;
	static NameKeyType buttonTabAudio         = NAMEKEY_INVALID;
	static NameKeyType buttonTabVideo         = NAMEKEY_INVALID;
	static NameKeyType buttonTabDisplay       = NAMEKEY_INVALID;
	static NameKeyType buttonTabAccessibility = NAMEKEY_INVALID;
	static NameKeyType buttonTabNetwork       = NAMEKEY_INVALID;

	switch( msg )
	{

		// -----------------------------------------------------------------------------------------
		case GWM_CREATE:
		{
			buttonBack                 = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonBack" ) );
			buttonDefaults             = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonDefaults" ) );
			buttonAccept               = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonAccept" ) );
			buttonKeyboardOptionsMenu  = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonKeyboardOptions" ) );
			buttonTabAudio         = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonTabAudio" ) );
			buttonTabVideo         = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonTabVideo" ) );
			buttonTabDisplay       = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonTabDisplay" ) );
			buttonTabAccessibility = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonTabAccessibility" ) );
			buttonTabNetwork       = TheNameKeyGenerator->nameToKey( AsciiString( "OptionsMenu.wnd:ButtonTabNetwork" ) );

			break;

		}  // end create

		//---------------------------------------------------------------------------------------------
		case GWM_DESTROY:
		{

			break;

		}  // end case

		// --------------------------------------------------------------------------------------------
		case GWM_INPUT_FOCUS:
		{

			// if we're givin the opportunity to take the keyboard focus we must say we want it
			if( mData1 == TRUE )
				*(Bool *)mData2 = TRUE;

			return MSG_HANDLED;

		}  // end input

		//---------------------------------------------------------------------------------------------
		case GCM_SELECTED:
			{
				if(ignoreSelected)
					break;
				GameWindow *control = (GameWindow *)mData1;
				Int controlID = control->winGetWindowId();
		
				if (controlID == comboBoxDetailID)
				{
					Int index;
					GadgetComboBoxGetSelectedPos( comboBoxDetail, &index );
					if(index != CUSTOMDETAIL)
						break;

					showTab(1);  // switch to Video tab to show quality options
				}
			break;
		}

		//---------------------------------------------------------------------------------------------
		case GBM_SELECTED:
		{
			if(ignoreSelected)
				break;
			GameWindow *control = (GameWindow *)mData1;
			Int controlID = control->winGetWindowId();

			if( controlID == buttonBack )
			{
				// go back one screen
				//TheShell->pop();
				if (pref)
				{
					delete pref;
					pref = NULL;
				}

				comboBoxLANIP = NULL;
				comboBoxOnlineIP = NULL;

				if(GameSpyIsOverlayOpen(GSOVERLAY_OPTIONS))
					GameSpyCloseOverlay(GSOVERLAY_OPTIONS);
				else
				{
					DestroyOptionsLayout();
				}

			}  // end if
			else if (controlID == buttonAccept )
					{
				if (pref)
				{
					saveOptions();
					pref->write();
					delete pref;
					pref = NULL;
				}

				comboBoxLANIP = NULL;
				comboBoxOnlineIP = NULL;
				
				if(!TheGameLogic->isInGame() || TheGameLogic->isInShellGame())
					destroyQuitMenu(); // if we're in a game, the change res then enter the same kind of game, we nee the quit menu to be gone.


				if(GameSpyIsOverlayOpen(GSOVERLAY_OPTIONS))
					GameSpyCloseOverlay(GSOVERLAY_OPTIONS);
				else
				{
					DestroyOptionsLayout();
					if (dispChanged)
					{
						DoResolutionDialog();
					}
				}

			}
			else if (controlID == buttonDefaults )
			{
				setDefaults();
			}
			else if (controlID == buttonTabAudio )
			{
				showTab(0);
			}
			else if (controlID == buttonTabVideo )
			{
				showTab(1);
			}
			else if (controlID == buttonTabDisplay )
			{
				showTab(2);
			}
			else if (controlID == buttonTabAccessibility )
			{
				showTab(3);
			}
			else if (controlID == buttonTabNetwork )
			{
				showTab(4);
			}
			else if ( controlID == buttonKeyboardOptionsMenu )
			{
				TheShell->push( AsciiString( "Menus/KeyboardOptionsMenu.wnd" ) );
			}
			else if(controlID == checkDrawAnchorID )
      {
        if( GadgetCheckBoxIsChecked( control ) )
        {
          	TheInGameUI->setDrawRMBScrollAnchor(true);
          	(*pref)["DrawScrollAnchor"] = "Yes";
        }
				else
        {
          	TheInGameUI->setDrawRMBScrollAnchor(false);
          	(*pref)["DrawScrollAnchor"] = "No";
        }
      }
			else if(controlID == checkMoveAnchorID )
      {
        if( GadgetCheckBoxIsChecked( control ) )
        {
          	TheInGameUI->setMoveRMBScrollAnchor(true);
          	(*pref)["MoveScrollAnchor"] = "Yes";
        }
				else
        {
          	TheInGameUI->setMoveRMBScrollAnchor(false);
          	(*pref)["MoveScrollAnchor"] = "No";
        }
      }
			else if(controlID == checkSaveCameraID || controlID == checkUseCameraID )
      {
        Bool replayEnabled = GadgetCheckBoxIsChecked( control ) ? TRUE : FALSE;
        TheWritableGlobalData->m_saveCameraInReplay = replayEnabled;
        TheWritableGlobalData->m_useCameraInReplay = replayEnabled;
        (*pref)["SaveCameraInReplays"] = replayEnabled ? "yes" : "no";
        (*pref)["UseCameraInReplays"] = replayEnabled ? "yes" : "no";
        SetCheckBoxCheckedSafe( checkSaveCamera, replayEnabled );
        SetCheckBoxCheckedSafe( checkUseCamera, replayEnabled );
      }
			else if (controlID == buttonFirewallRefreshID)
			{
				// setting the behavior to unknown will force the firewall helper to detect the firewall behavior
				// the next time we log into gamespy/WOL/whatever.
				char num[16];
				num[0] = 0;
				TheWritableGlobalData->m_firewallBehavior = FirewallHelperClass::FIREWALL_TYPE_UNKNOWN;
				itoa(TheGlobalData->m_firewallBehavior, num, 10);
				AsciiString numstr;
				numstr = num;
				(*pref)["FirewallBehavior"] = numstr;
			}
			break;

		}  // end selected

		// -----------------------------------------------------------------------------------------
		case GSM_SLIDER_TRACK:
		{
			GameWindow *slider = (GameWindow *)mData1;
			if (slider && panelScrollBar && slider->winGetWindowId() == panelScrollBarID)
			{
				Int sliderPos = (Int)mData2;  // 100 = thumb at top (no scroll), 0 = thumb at bottom (max scroll)
				if (s_maxScrollPx <= 0)
					return MSG_HANDLED;
				Int newOffset = ((100 - sliderPos) * s_maxScrollPx) / 100;
				Int delta     = newOffset - s_currentScrollPx;
				if (delta != 0)
				{
					GameWindow *panel = getTabPanel(activeTab);
					if (panel != NULL)
					{
						GameWindow *child = panel->winGetChild();
						while (child != NULL)
						{
							Int cx, cy;
							child->winGetPosition(&cx, &cy);
							child->winSetPosition(cx, cy - delta);
							child = child->winGetNext();
						}
					}
					s_currentScrollPx = newOffset;
					bringWindowToFront(panelScrollBar);
				}
			}
			return MSG_HANDLED;
		}

		default:
			return MSG_IGNORED;

	}  // end switch

	return MSG_HANDLED;

}  // end OptionsMenuSystem
