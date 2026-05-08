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

// FILE: PersistentStorageThread.cpp //////////////////////////////////////////////////////
// GameSpy Persistent Storage thread
// This thread communicates with GameSpy's persistent storage server
// and talks through a message queue with the rest of
// the game.
// Author: Matthew D. Campbell, July 2002

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/GameSpyMiscPreferences.h"
#include "Common/Registry.h"
#include "Common/UserPreferences.h"
#include "Common/PlayerTemplate.h"
#include "GameNetwork/GameSpy/PersistentStorageThread.h"
#include "GameNetwork/GameSpy/PeerDefs.h"

#include "mutex.h"
#include "thread.h"

#include "Common/StackDump.h"
#include "Common/SubsystemInterface.h"

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

//-------------------------------------------------------------------------

PSRequest::PSRequest()
{
	player.reset();
	requestType = PSREQUEST_READPLAYERSTATS;
	addDiscon = addDesync = FALSE;
	lastHouse = -1;
}

//-------------------------------------------------------------------------

#define DEBUG_MAP(x) for (it = stats.x.begin(); it != stats.x.end(); ++it) \
{ \
	if (it->second > 0) \
	{ \
		DEBUG_LOG(("%s(%d): %d\n", #x, it->first, it->second)); \
	} \
}

static void debugDumpPlayerStats( const PSPlayerStats& stats )
{
	DEBUG_LOG(("-----------------------------------------\n"));
	DEBUG_LOG(("Tracking player stats for player %d:\n", stats.id));
	PerGeneralMap::const_iterator it;
	DEBUG_MAP(wins);
	DEBUG_MAP(losses);
	DEBUG_MAP(games);
	DEBUG_MAP(duration);
	DEBUG_MAP(unitsKilled);
	DEBUG_MAP(unitsLost);
	DEBUG_MAP(unitsBuilt);
	DEBUG_MAP(buildingsKilled);
	DEBUG_MAP(buildingsLost);
	DEBUG_MAP(buildingsBuilt);
	DEBUG_MAP(earnings);
	DEBUG_MAP(techCaptured);
	DEBUG_MAP(discons);
	DEBUG_MAP(desyncs);
	DEBUG_MAP(surrenders);
	DEBUG_MAP(gamesOf2p);
	DEBUG_MAP(gamesOf3p);
	DEBUG_MAP(gamesOf4p);
	DEBUG_MAP(gamesOf5p);
	DEBUG_MAP(gamesOf6p);
	DEBUG_MAP(gamesOf7p);
	DEBUG_MAP(gamesOf8p);
	DEBUG_MAP(customGames);
	DEBUG_MAP(QMGames);
	
	if (stats.locale > 0)
	{
		DEBUG_LOG(("Locale: %d\n", stats.locale));
	}
	
	if (stats.gamesAsRandom > 0)
	{
		DEBUG_LOG(("gamesAsRandom: %d\n", stats.gamesAsRandom));
	}

	if (stats.options.length())
	{
		DEBUG_LOG(("Options: %s\n", stats.options.c_str()));
	}

	if (stats.systemSpec.length())
	{
		DEBUG_LOG(("systemSpec: %s\n", stats.systemSpec.c_str()));
	}

	if (stats.lastFPS > 0.0f)
	{
		DEBUG_LOG(("lastFPS: %g\n", stats.lastFPS));
	}

	if (stats.battleHonors > 0)
	{
		DEBUG_LOG(("battleHonors: %x\n", stats.battleHonors));
	}
	if (stats.challengeMedals > 0)
	{
		DEBUG_LOG(("challengeMedals: %x\n", stats.challengeMedals));
	}
	if (stats.lastGeneral >= 0)
	{
		DEBUG_LOG(("lastGeneral: %d\n", stats.lastGeneral));
	}
	if (stats.gamesInRowWithLastGeneral >= 0)
	{
		DEBUG_LOG(("gamesInRowWithLastGeneral: %d\n", stats.gamesInRowWithLastGeneral));
	}
	if (stats.builtSCUD >= 0)
	{
		DEBUG_LOG(("builtSCUD: %d\n", stats.builtSCUD));
	}
	if (stats.builtNuke >= 0)
	{
		DEBUG_LOG(("builtNuke: %d\n", stats.builtNuke));
	}
	if (stats.builtParticleCannon >= 0)
	{
		DEBUG_LOG(("builtParticleCannon: %d\n", stats.builtParticleCannon));
	}

	if (stats.winsInARow >= 0)
	{
		DEBUG_LOG(("winsInARow: %d\n", stats.winsInARow));
	}
	if (stats.maxWinsInARow >= 0)
	{
		DEBUG_LOG(("maxWinsInARow: %d\n", stats.maxWinsInARow));
	}
	if (stats.disconsInARow >= 0)
	{
		DEBUG_LOG(("disconsInARow: %d\n", stats.disconsInARow));
	}
	if (stats.maxDisconsInARow >= 0)
	{
		DEBUG_LOG(("maxDisconsInARow: %d\n", stats.maxDisconsInARow));
	}
	if (stats.lossesInARow >= 0)
	{
		DEBUG_LOG(("lossesInARow: %d\n", stats.lossesInARow));
	}
	if (stats.maxLossesInARow >= 0)
	{
		DEBUG_LOG(("maxLossesInARow: %d\n", stats.maxLossesInARow));
	}
	if (stats.desyncsInARow >= 0)
	{
		DEBUG_LOG(("desyncsInARow: %d\n", stats.desyncsInARow));
	}
	if (stats.maxDesyncsInARow >= 0)
	{
		DEBUG_LOG(("maxDesyncsInARow: %d\n", stats.maxDesyncsInARow));
	}

	if (stats.lastLadderPort >= 0)
	{
		DEBUG_LOG(("lastLadderPort: %d\n", stats.lastLadderPort));
	}

	if (stats.lastLadderHost.length())
	{
		DEBUG_LOG(("lastLadderHost: %s\n", stats.lastLadderHost.c_str()));
	}



}

//-------------------------------------------------------------------------

#define INCORPORATE_MAP(x) for (it = other.x.begin(); it != other.x.end(); ++it) \
{ \
	if (it->second > 0) \
	{ \
		x[it->first] = it->second; \
	} \
}

void PSPlayerStats::incorporate( const PSPlayerStats& other )
{
	PerGeneralMap::const_iterator it;
	INCORPORATE_MAP(wins);
	INCORPORATE_MAP(losses);
	INCORPORATE_MAP(games);
	INCORPORATE_MAP(duration);
	INCORPORATE_MAP(unitsKilled);
	INCORPORATE_MAP(unitsLost);
	INCORPORATE_MAP(unitsBuilt);
	INCORPORATE_MAP(buildingsKilled);
	INCORPORATE_MAP(buildingsLost);
	INCORPORATE_MAP(buildingsBuilt);
	INCORPORATE_MAP(earnings);
	INCORPORATE_MAP(techCaptured);

	//GS  Clear all disconnects so that we don't retain any that were
	//previously reported as 1 by updateAdditionalGameSpyDisconnections
	discons.clear();
	INCORPORATE_MAP(discons);

	INCORPORATE_MAP(desyncs);
	INCORPORATE_MAP(surrenders);
	INCORPORATE_MAP(gamesOf2p);
	INCORPORATE_MAP(gamesOf3p);
	INCORPORATE_MAP(gamesOf4p);
	INCORPORATE_MAP(gamesOf5p);
	INCORPORATE_MAP(gamesOf6p);
	INCORPORATE_MAP(gamesOf7p);
	INCORPORATE_MAP(gamesOf8p);
	INCORPORATE_MAP(customGames);
	INCORPORATE_MAP(QMGames);
	
	if (other.locale > 0)
	{
		locale = other.locale;
	}
	
	if (other.gamesAsRandom > 0)
	{
		gamesAsRandom = other.gamesAsRandom;
	}

	if (other.options.length())
	{
		options = other.options;
	}

	if (other.systemSpec.length())
	{
		systemSpec = other.systemSpec;
	}

	if (other.lastFPS > 0.0f)
	{
		lastFPS = other.lastFPS;
	}

	if (other.battleHonors > 0)
	{
		battleHonors |= other.battleHonors;
	}
	if (other.challengeMedals > 0)
	{
		challengeMedals |= other.challengeMedals;
	}
	if (other.lastGeneral >= 0)
	{
		lastGeneral = other.lastGeneral;
	}
	if (other.gamesInRowWithLastGeneral >= 0)
	{
		gamesInRowWithLastGeneral = other.gamesInRowWithLastGeneral;
	}
	if (other.builtParticleCannon >= 0)
	{
		builtParticleCannon = other.builtParticleCannon;
	}
	if (other.builtNuke >= 0)
	{
		builtNuke = other.builtNuke;
	}
	if (other.builtSCUD >= 0)
	{
		builtSCUD = other.builtSCUD;
	}
	if (other.winsInARow >= 0)
	{
		winsInARow = other.winsInARow;
	}
	if (other.maxWinsInARow >= 0)
	{
		maxWinsInARow = other.maxWinsInARow;
	}
	if (other.lossesInARow >= 0)
	{
		lossesInARow = other.lossesInARow;
	}
	if (other.maxLossesInARow >= 0)
	{
		maxLossesInARow = other.maxLossesInARow;
	}
	if (other.disconsInARow >= 0)
	{
		disconsInARow = other.disconsInARow;
	}
	if (other.maxDisconsInARow >= 0)
	{
		maxDisconsInARow = other.maxDisconsInARow;
	}
	if (other.desyncsInARow >= 0)
	{
		desyncsInARow = other.desyncsInARow;
	}
	if (other.maxDesyncsInARow >= 0)
	{
		maxDesyncsInARow = other.maxDesyncsInARow;
	}
	if (other.lastLadderPort >= 0)
	{
		lastLadderPort = other.lastLadderPort;
	}
	if (other.lastLadderHost.length())
	{
		lastLadderHost = other.lastLadderHost;
	}
}

PSPlayerStats::PSPlayerStats( const PSPlayerStats& other )
{
	incorporate(other);
	id = other.id;
	locale = other.locale;
	gamesAsRandom = other.gamesAsRandom;
	options = other.options;
	systemSpec = other.systemSpec;
	lastFPS = other.lastFPS;
	lastGeneral = other.lastGeneral;
	gamesInRowWithLastGeneral = other.gamesInRowWithLastGeneral;
	builtParticleCannon = other.builtParticleCannon;
	builtNuke = other.builtNuke;
	builtSCUD = other.builtSCUD;
	challengeMedals = other.challengeMedals;
	battleHonors = other.battleHonors;
	winsInARow = other.winsInARow;
	maxWinsInARow = other.maxWinsInARow;
	lossesInARow = other.lossesInARow;
	maxLossesInARow = other.maxLossesInARow;
	disconsInARow = other.disconsInARow;
	maxDisconsInARow = other.maxDisconsInARow;
	desyncsInARow = other.desyncsInARow;
	maxDesyncsInARow = other.maxDesyncsInARow;
	lastLadderHost = other.lastLadderHost;
	lastLadderPort = other.lastLadderPort;
}

//-------------------------------------------------------------------------

typedef std::queue<PSRequest> RequestQueue;
typedef std::queue<PSResponse> ResponseQueue;
class PSThreadClass;

class GameSpyPSMessageQueue : public GameSpyPSMessageQueueInterface
{
public:
	virtual ~GameSpyPSMessageQueue();
	GameSpyPSMessageQueue();
	virtual void startThread( void );
	virtual void endThread( void );
	virtual Bool isThreadRunning( void );

	virtual void addRequest( const PSRequest& req );
	virtual Bool getRequest( PSRequest& req );

	virtual void addResponse( const PSResponse& resp );
	virtual Bool getResponse( PSResponse& resp );

	virtual void trackPlayerStats( PSPlayerStats stats );
	virtual PSPlayerStats findPlayerStatsByID( Int id );

	PSThreadClass* getThread( void );

	Int getLocalPlayerID(void) { return m_localPlayerID; }
	void setLocalPlayerID(Int localPlayerID) { m_localPlayerID = localPlayerID; }

	std::string getEmail() { return m_email; }
	std::string getNick() { return m_nick; }
	std::string getPassword() { return m_password; }

	void setEmail(std::string email) { m_email = email; }
	void setNick(std::string nick) { m_nick = nick; }
	void setPassword(std::string password) { m_password = password; }

private:
	MutexClass m_requestMutex;
	MutexClass m_responseMutex;
	RequestQueue m_requests;
	ResponseQueue m_responses;
	PSThreadClass *m_thread;
	Int m_localPlayerID;

	std::string m_email;
	std::string m_nick;
	std::string m_password;

	std::map<Int, PSPlayerStats> m_playerStats;
};

GameSpyPSMessageQueueInterface* GameSpyPSMessageQueueInterface::createNewMessageQueue( void )
{
	return NEW GameSpyPSMessageQueue;
}

GameSpyPSMessageQueueInterface *TheGameSpyPSMessageQueue = NULL;
#define MESSAGE_QUEUE ((GameSpyPSMessageQueue *)TheGameSpyPSMessageQueue)

namespace
{
AsciiString GetPlayerStatsFilename(Int profileID)
{
	AsciiString filename;
	filename.format("GeneralsOnline\\PlayerStats%d.ini", profileID);
	return filename;
}

AsciiString GetDisconnectCarryoverFilename(Int profileID)
{
	AsciiString filename;
	filename.format("GeneralsOnline\\MiscPref%d.ini", profileID);
	return filename;
}

void LoadPendingDisconnectCounts(
	Int profileID,
	Int *addedInDesyncs2,
	Int *addedInDesyncs3,
	Int *addedInDesyncs4,
	Int *addedInDiscons2,
	Int *addedInDiscons3,
	Int *addedInDiscons4)
{
	if (addedInDesyncs2) *addedInDesyncs2 = 0;
	if (addedInDesyncs3) *addedInDesyncs3 = 0;
	if (addedInDesyncs4) *addedInDesyncs4 = 0;
	if (addedInDiscons2) *addedInDiscons2 = 0;
	if (addedInDiscons3) *addedInDiscons3 = 0;
	if (addedInDiscons4) *addedInDiscons4 = 0;

	if (profileID <= 0)
		return;

	UserPreferences pref;
	pref.load(GetDisconnectCarryoverFilename(profileID));

	if (addedInDesyncs2)
	{
		*addedInDesyncs2 = pref.getInt("0", 0);
		if (*addedInDesyncs2 < 0)
			*addedInDesyncs2 = 10;
	}
	if (addedInDesyncs3)
	{
		*addedInDesyncs3 = pref.getInt("1", 0);
		if (*addedInDesyncs3 < 0)
			*addedInDesyncs3 = 10;
	}
	if (addedInDesyncs4)
	{
		*addedInDesyncs4 = pref.getInt("2", 0);
		if (*addedInDesyncs4 < 0)
			*addedInDesyncs4 = 10;
	}
	if (addedInDiscons2)
	{
		*addedInDiscons2 = pref.getInt("3", 0);
		if (*addedInDiscons2 < 0)
			*addedInDiscons2 = 10;
	}
	if (addedInDiscons3)
	{
		*addedInDiscons3 = pref.getInt("4", 0);
		if (*addedInDiscons3 < 0)
			*addedInDiscons3 = 10;
	}
	if (addedInDiscons4)
	{
		*addedInDiscons4 = pref.getInt("5", 0);
		if (*addedInDiscons4 < 0)
			*addedInDiscons4 = 10;
	}
}

Bool HasPendingDisconnectCounts(Int profileID)
{
	Int addedInDesyncs2, addedInDesyncs3, addedInDesyncs4;
	Int addedInDiscons2, addedInDiscons3, addedInDiscons4;
	LoadPendingDisconnectCounts(
		profileID,
		&addedInDesyncs2,
		&addedInDesyncs3,
		&addedInDesyncs4,
		&addedInDiscons2,
		&addedInDiscons3,
		&addedInDiscons4);
	return addedInDesyncs2 || addedInDesyncs3 || addedInDesyncs4 || addedInDiscons2 || addedInDiscons3 || addedInDiscons4;
}

void UpdatePendingDisconnectCounts(const PSRequest& req, Int profileID)
{
	if (profileID <= 0 || (!req.addDesync && !req.addDiscon))
		return;

	UserPreferences pref;
	pref.load(GetDisconnectCarryoverFilename(profileID));

	AsciiString val;
	if (req.lastHouse == 2)
	{
		val.format("%d", pref.getInt("0", 0) + req.addDesync);
		pref["0"] = val;
		val.format("%d", pref.getInt("3", 0) + req.addDiscon);
		pref["3"] = val;
	}
	else if (req.lastHouse == 3)
	{
		val.format("%d", pref.getInt("1", 0) + req.addDesync);
		pref["1"] = val;
		val.format("%d", pref.getInt("4", 0) + req.addDiscon);
		pref["4"] = val;
	}
	else
	{
		val.format("%d", pref.getInt("2", 0) + req.addDesync);
		pref["2"] = val;
		val.format("%d", pref.getInt("5", 0) + req.addDiscon);
		pref["5"] = val;
	}
	pref.write();
}

void ClearPendingDisconnectCounts(Int profileID)
{
	if (profileID <= 0)
		return;

	UserPreferences pref;
	pref.load(GetDisconnectCarryoverFilename(profileID));
	pref.clear();
	pref.write();
}

void ApplyPendingDisconnectCounts(
	PSPlayerStats *stats,
	Int addedInDesyncs2,
	Int addedInDesyncs3,
	Int addedInDesyncs4,
	Int addedInDiscons2,
	Int addedInDiscons3,
	Int addedInDiscons4)
{
	if (!stats)
		return;

	stats->desyncs[2] += addedInDesyncs2;
	stats->games[2] += addedInDesyncs2;
	stats->discons[2] += addedInDiscons2;
	stats->games[2] += addedInDiscons2;

	stats->desyncs[3] += addedInDesyncs3;
	stats->games[3] += addedInDesyncs3;
	stats->discons[3] += addedInDiscons3;
	stats->games[3] += addedInDiscons3;

	stats->desyncs[4] += addedInDesyncs4;
	stats->games[4] += addedInDesyncs4;
	stats->discons[4] += addedInDiscons4;
	stats->games[4] += addedInDiscons4;
}

PSPlayerStats LoadStoredPlayerStats(Int profileID)
{
	PSPlayerStats stats;
	if (profileID <= 0)
		return stats;

	UserPreferences pref;
	pref.load(GetPlayerStatsFilename(profileID));
	AsciiString serialized = pref.getAsciiString("Stats", AsciiString::TheEmptyString);
	if (serialized.isEmpty() && TheGameSpyInfo && profileID == TheGameSpyInfo->getLocalProfileID())
	{
		GameSpyMiscPreferences miscPref;
		serialized = miscPref.getCachedStats();
	}

	if (serialized.isNotEmpty())
		stats = GameSpyPSMessageQueueInterface::parsePlayerKVPairs(serialized.str());

	stats.id = profileID;
	if (pref.getBool("Preorder", FALSE) && TheGameSpyInfo)
		TheGameSpyInfo->markPlayerAsPreorder(profileID);

	return stats;
}

void SaveStoredPlayerStats(const PSPlayerStats& stats)
{
	if (stats.id <= 0)
		return;

	UserPreferences pref;
	pref.load(GetPlayerStatsFilename(stats.id));
	pref.setAsciiString("Stats", GameSpyPSMessageQueueInterface::formatPlayerKVPairs(stats).c_str());
	pref.write();

	if (TheGameSpyInfo && stats.id == TheGameSpyInfo->getLocalProfileID())
	{
		GameSpyMiscPreferences miscPref;
		miscPref.setCachedStats(GameSpyPSMessageQueueInterface::formatPlayerKVPairs(stats).c_str());
		miscPref.write();
	}
}

Bool LoadStoredPreorder(Int profileID)
{
	if (profileID <= 0)
		return FALSE;

	UserPreferences pref;
	pref.load(GetPlayerStatsFilename(profileID));
	return pref.getBool("Preorder", FALSE);
}

void SaveStoredPreorder(Int profileID, Bool preorder)
{
	if (profileID <= 0)
		return;

	UserPreferences pref;
	pref.load(GetPlayerStatsFilename(profileID));
	pref.setBool("Preorder", preorder);
	pref.write();

	if (preorder && TheGameSpyInfo)
		TheGameSpyInfo->markPlayerAsPreorder(profileID);
}
}

//-------------------------------------------------------------------------

class PSThreadClass : public ThreadClass
{

public:
	PSThreadClass() : ThreadClass() {}

	void Thread_Function();
};


//-------------------------------------------------------------------------

GameSpyPSMessageQueue::GameSpyPSMessageQueue()
{
	m_thread = NULL;
	m_localPlayerID = 0;
}

GameSpyPSMessageQueue::~GameSpyPSMessageQueue()
{
	endThread();
}

void GameSpyPSMessageQueue::startThread( void )
{
	if (!m_thread)
	{
		m_thread = NEW PSThreadClass;
		m_thread->Execute();
	}
	else
	{
		if (!m_thread->Is_Running())
		{
			m_thread->Execute();
		}
	}
}

void GameSpyPSMessageQueue::endThread( void )
{
	if (m_thread)
		delete m_thread;
	m_thread = NULL;
}

Bool GameSpyPSMessageQueue::isThreadRunning( void )
{
	return (m_thread) ? m_thread->Is_Running() : false;
}

void GameSpyPSMessageQueue::addRequest( const PSRequest& req )
{
	MutexClass::LockClass m(m_requestMutex);
	if (m.Failed())
		return;

	m_requests.push(req);
}

Bool GameSpyPSMessageQueue::getRequest( PSRequest& req )
{
	MutexClass::LockClass m(m_requestMutex, 0);
	if (m.Failed())
		return false;

	if (m_requests.empty())
		return false;
	req = m_requests.front();
	m_requests.pop();
	return true;
}

void GameSpyPSMessageQueue::addResponse( const PSResponse& resp )
{
	MutexClass::LockClass m(m_responseMutex);
	if (m.Failed())
		return;

	m_responses.push(resp);
}

Bool GameSpyPSMessageQueue::getResponse( PSResponse& resp )
{
	MutexClass::LockClass m(m_responseMutex, 0);
	if (m.Failed())
		return false;

	if (m_responses.empty())
		return false;
	resp = m_responses.front();
	m_responses.pop();
	return true;
}

PSThreadClass* GameSpyPSMessageQueue::getThread( void )
{
	return m_thread;
}

void GameSpyPSMessageQueue::trackPlayerStats( PSPlayerStats stats )
{
#ifdef DEBUG_LOGGING
	debugDumpPlayerStats( stats );
	DEBUG_ASSERTCRASH(stats.id != 0, ("Tracking stats with ID of 0\n"));
#endif
	PSPlayerStats newStats;
	std::map<Int, PSPlayerStats>::iterator it = m_playerStats.find(stats.id);
	if (it != m_playerStats.end())
	{
		newStats = it->second;
		newStats.incorporate(stats);
		m_playerStats[stats.id] = newStats;
	}
	else
	{
		m_playerStats[stats.id] = stats;
	}
}

PSPlayerStats GameSpyPSMessageQueue::findPlayerStatsByID( Int id )
{
	std::map<Int, PSPlayerStats>::iterator it = m_playerStats.find(id);
	if (it != m_playerStats.end())
	{
		return it->second;
	}

	PSPlayerStats empty;
	empty.id = 0;
	return empty;
}

void PSThreadClass::Thread_Function()
{
	try {
	_set_se_translator( DumpExceptionInfo ); // Hook that allows stack trace.
	PSRequest req;
	while ( running )
	{
		if (TheGameSpyPSMessageQueue->getRequest(req))
		{
			switch (req.requestType)
			{
			case PSRequest::PSREQUEST_SENDGAMERESTOGAMESPY:
				{
					DEBUG_LOG(("PSREQUEST_SENDGAMERESTOGAMESPY ignored by local backend, results='%s'\n", req.results.c_str()));
				}
				break;
			case PSRequest::PSREQUEST_READPLAYERSTATS:
				{
					if (!MESSAGE_QUEUE->getLocalPlayerID())
					{
						MESSAGE_QUEUE->setLocalPlayerID(req.player.id); // first request is for ourselves
						MESSAGE_QUEUE->setEmail(req.email);
						MESSAGE_QUEUE->setNick(req.nick);
						MESSAGE_QUEUE->setPassword(req.password);
						DEBUG_LOG(("Setting email/nick/password = %s/%s/%s\n", req.email.c_str(), req.nick.c_str(), req.password.c_str()));
					}

					PSResponse resp;
					resp.responseType = PSResponse::PSRESPONSE_PLAYERSTATS;
					resp.player = LoadStoredPlayerStats(req.player.id);

					if (req.player.id == MESSAGE_QUEUE->getLocalPlayerID() && TheGameSpyGame && TheGameSpyGame->getUseStats() && HasPendingDisconnectCounts(req.player.id))
					{
						PSRequest updateReq;
						updateReq.requestType = PSRequest::PSREQUEST_UPDATEPLAYERSTATS;
						updateReq.email = MESSAGE_QUEUE->getEmail();
						updateReq.nick = MESSAGE_QUEUE->getNick();
						updateReq.password = MESSAGE_QUEUE->getPassword();
						updateReq.player = resp.player;
						updateReq.player.id = req.player.id;
						updateReq.addDesync = FALSE;
						updateReq.addDiscon = FALSE;
						updateReq.lastHouse = 0;
						TheGameSpyPSMessageQueue->addRequest(updateReq);
					}

					TheGameSpyPSMessageQueue->addResponse(resp);
				}
				break;
			case PSRequest::PSREQUEST_UPDATEPLAYERLOCALE:
				{
					PSPlayerStats stats = LoadStoredPlayerStats(req.player.id);
					stats.id = req.player.id;
					stats.locale = req.player.locale;
					SaveStoredPlayerStats(stats);
					if (TheGameSpyPSMessageQueue)
						TheGameSpyPSMessageQueue->trackPlayerStats(stats);
				}
				break;
			case PSRequest::PSREQUEST_UPDATEPLAYERSTATS:
				{
					Int addedInDesyncs2, addedInDesyncs3, addedInDesyncs4;
					Int addedInDiscons2, addedInDiscons3, addedInDiscons4;
					LoadPendingDisconnectCounts(
						MESSAGE_QUEUE->getLocalPlayerID(),
						&addedInDesyncs2,
						&addedInDesyncs3,
						&addedInDesyncs4,
						&addedInDiscons2,
						&addedInDiscons3,
						&addedInDiscons4);

					UpdatePendingDisconnectCounts(req, MESSAGE_QUEUE->getLocalPlayerID());
					if ((req.addDesync || req.addDiscon) && req.password.size() == 0)
						break;

					if (!req.player.id)
					{
						DEBUG_LOG(("Bailing because ID is NULL!\n"));
						break;
					}

					ApplyPendingDisconnectCounts(
						&req.player,
						addedInDesyncs2,
						addedInDesyncs3,
						addedInDesyncs4,
						addedInDiscons2,
						addedInDiscons3,
						addedInDiscons4);
					SaveStoredPlayerStats(req.player);
					ClearPendingDisconnectCounts(req.player.id);
					if (TheGameSpyPSMessageQueue)
						TheGameSpyPSMessageQueue->trackPlayerStats(req.player);
				}
				break;
			case PSRequest::PSREQUEST_READCDKEYSTATS:
				{
					UnsignedInt preorderRegistry = 0;
					GetUnsignedIntFromRegistry("", "Preorder", preorderRegistry);

					PSResponse resp;
					resp.responseType = PSResponse::PSRESPONSE_PREORDER;
					resp.preorder = (preorderRegistry != 0) || LoadStoredPreorder(MESSAGE_QUEUE->getLocalPlayerID());
					if (resp.preorder)
						SaveStoredPreorder(MESSAGE_QUEUE->getLocalPlayerID(), TRUE);
					TheGameSpyPSMessageQueue->addResponse(resp);
				}
				break;
			}
		}

		// end our timeslice
		Switch_Thread();
	}
	} catch ( ... ) {
		DEBUG_CRASH(("Exception in storage thread!"));
	}
}

//-------------------------------------------------------------------------
PSPlayerStats::PSPlayerStats( void )
{
	reset();
}

void PSPlayerStats::reset( void )
{
	id = 0;
	locale = 0;
	gamesAsRandom = 0;
	lastFPS = 0;
	lastGeneral = 0;
	gamesInRowWithLastGeneral = 0;
	builtNuke = 0;
	builtSCUD = 0;
	builtParticleCannon = 0;
	challengeMedals = 0;
	battleHonors = 0;
	winsInARow = 0;
	maxWinsInARow = 0;
	lossesInARow = 0;
	maxLossesInARow = 0;
	disconsInARow = 0;
	maxDisconsInARow = 0;
	desyncsInARow = 0;
	maxDesyncsInARow = 0;
	lastLadderPort = 0;

	//Added By Sadullah Nader
	maxQMwinsInARow = 0;
	QMwinsInARow = 0;
	//
}

//-------------------------------------------------------------------------
#define CHECK(x) if (k == #x && generalMarker >= 0) { s.x[generalMarker] = atoi(v.c_str()); continue; }

PSPlayerStats GameSpyPSMessageQueueInterface::parsePlayerKVPairs( std::string kvPairs )
{
	PSPlayerStats s;
	kvPairs.append("\\");

	Int offset = 0;
	while (1)
	{
		Int firstMarker = kvPairs.find_first_of('\\', offset);
		if (firstMarker < 0)
			break;
		Int secondMarker = kvPairs.find_first_of('\\', firstMarker + 1);
		if (secondMarker < 0)
			break;
		Int thirdMarker = kvPairs.find_first_of('\\', secondMarker + 1);
		if (thirdMarker < 0)
			break;
		Int generalMarker = kvPairs.find_last_not_of("0123456789", secondMarker - 1);
		std::string k, v, g;
		if (generalMarker == secondMarker - 1)
		{
			k = kvPairs.substr(firstMarker + 1, secondMarker - firstMarker - 1);
			generalMarker = -1;
		}
		else
		{
			k = kvPairs.substr(firstMarker + 1, generalMarker - firstMarker);
			g = kvPairs.substr(generalMarker + 1, secondMarker - generalMarker - 1);
			generalMarker = atoi(g.c_str());
		}
		v = kvPairs.substr(secondMarker + 1, thirdMarker - secondMarker - 1);
		//DEBUG_LOG(("%d [%s] [%s]\n", generalMarker, k.c_str(), v.c_str()));
		offset = thirdMarker - 1;

		CHECK(wins);
		CHECK(losses);
		CHECK(games);
		CHECK(duration);
		CHECK(unitsKilled);
		CHECK(unitsLost);
		CHECK(unitsBuilt);
		CHECK(buildingsKilled);
		CHECK(buildingsLost);
		CHECK(buildingsBuilt);
		CHECK(earnings);
		CHECK(techCaptured);
		CHECK(discons);
		CHECK(desyncs);
		CHECK(surrenders);
		CHECK(gamesOf2p);
		CHECK(gamesOf3p);
		CHECK(gamesOf4p);
		CHECK(gamesOf5p);
		CHECK(gamesOf6p);
		CHECK(gamesOf7p);
		CHECK(gamesOf8p);
		CHECK(customGames);
		CHECK(QMGames);
		
		if (k == "locale" && generalMarker < 0)
		{
			s.locale = atoi(v.c_str());
			continue;
		}

		if (k == "random" && generalMarker < 0)
		{
			s.gamesAsRandom = atoi(v.c_str());
			continue;
		}

		if (k == "options" && generalMarker < 0)
		{
			s.options = v;
			continue;
		}

		if (k == "systemSpec" && generalMarker < 0)
		{
			s.systemSpec = v;
			continue;
		}

		if (k == "fps" && generalMarker < 0)
		{
			s.lastFPS = atof(v.c_str());
			continue;
		}
		
		if (k == "lastGeneral" && generalMarker < 0)
		{
			s.lastGeneral = atoi(v.c_str());
			continue;
		}
		if (k == "genInRow" && generalMarker < 0)
		{
			s.gamesInRowWithLastGeneral = atoi(v.c_str());
			continue;
		}
		if (k == "builtNuke" && generalMarker < 0)
		{
			s.builtNuke = atoi(v.c_str());
			continue;
		}
		if (k == "builtSCUD" && generalMarker < 0)
		{
			s.builtSCUD = atoi(v.c_str());
			continue;
		}
		if (k == "builtCannon" && generalMarker < 0)
		{
			s.builtParticleCannon = atoi(v.c_str());
			continue;
		}
		if (k == "challenge" && generalMarker < 0)
		{
			s.challengeMedals = atoi(v.c_str());
			continue;
		}
		if (k == "battle" && generalMarker < 0)
		{
			s.battleHonors = atoi(v.c_str());
			continue;
		}

		if (k == "WinRow" && generalMarker < 0)
		{
			s.winsInARow = atoi(v.c_str());
			continue;
		}
		if (k == "WinRowMax" && generalMarker < 0)
		{
			s.maxWinsInARow = atoi(v.c_str());
			continue;
		}

		if (k == "LossRow" && generalMarker < 0)
		{
			s.lossesInARow = atoi(v.c_str());
			continue;
		}
		if (k == "LossRowMax" && generalMarker < 0)
		{
			s.maxLossesInARow = atoi(v.c_str());
			continue;
		}

		if (k == "DSRow" && generalMarker < 0)
		{
			s.desyncsInARow = atoi(v.c_str());
			continue;
		}
		if (k == "DSRowMax" && generalMarker < 0)
		{
			s.maxDesyncsInARow = atoi(v.c_str());
			continue;
		}

		if (k == "DCRow" && generalMarker < 0)
		{
			s.disconsInARow = atoi(v.c_str());
			continue;
		}
		if (k == "DCRowMax" && generalMarker < 0)
		{
			s.maxDisconsInARow = atoi(v.c_str());
			continue;
		}

		if (k == "ladderPort" && generalMarker < 0)
		{
			s.lastLadderPort = atoi(v.c_str());
			continue;
		}
		if (k == "ladderHost" && generalMarker < 0)
		{
			s.lastLadderHost = v;
			continue;
		}

		//DEBUG_ASSERTCRASH(generalMarker >= 0, ("Unknown KV Pair in persistent storage: [%s] = [%s]\n", k.c_str(), v.c_str()));
		//DEBUG_ASSERTCRASH(generalMarker  < 0, ("Unknown KV Pair in persistent storage for PlayerTemplate %d: [%s] = [%s]\n", generalMarker, k.c_str(), v.c_str()));
	}

	return s;
}

#define ITERATE_OVER(x) for (it = stats.x.begin(); it != stats.x.end(); ++it) \
{ \
	if (it->second > 0) \
	{ \
		sprintf(kvbuf, "\\" #x "%d\\%d", it->first, it->second); \
		s.append(kvbuf); \
	} \
}

#include "Common/PlayerTemplate.h"

std::string GameSpyPSMessageQueueInterface::formatPlayerKVPairs( PSPlayerStats stats )
{
	char kvbuf[256];
	std::string s = "";
	PerGeneralMap::iterator it;

	ITERATE_OVER(wins);
	ITERATE_OVER(losses);
	ITERATE_OVER(games);
	ITERATE_OVER(duration);
	ITERATE_OVER(unitsKilled);
	ITERATE_OVER(unitsLost);
	ITERATE_OVER(unitsBuilt);
	ITERATE_OVER(buildingsKilled);
	ITERATE_OVER(buildingsLost);
	ITERATE_OVER(buildingsBuilt);
	ITERATE_OVER(earnings);
	ITERATE_OVER(techCaptured);

	//GS  Report all disconnects, even if zero, because might have been 
	//previously reported as 1 by updateAdditionalGameSpyDisconnections
//	ITERATE_OVER(discons);
	for (Int ptIdx = 0; ptIdx < ThePlayerTemplateStore->getPlayerTemplateCount(); ++ptIdx)
	{
//		const PlayerTemplate* pTemplate = ThePlayerTemplateStore->getNthPlayerTemplate(ptIdx);
//		const GeneralPersona* pGeneral = TheChallengeGenerals->getGeneralByTemplateName(pTemplate->getName());
//		BOOL isReported = pGeneral ? pGeneral->isStartingEnabled() : FALSE;
//		if( !isReported )
//			continue;  //don't report unplayable templates (observer, boss, etc.)

		sprintf(kvbuf, "\\discons%d\\%d", ptIdx, stats.discons[ptIdx]);
		s.append(kvbuf);
	}

	ITERATE_OVER(desyncs);
	ITERATE_OVER(surrenders);
	ITERATE_OVER(gamesOf2p);
	ITERATE_OVER(gamesOf3p);
	ITERATE_OVER(gamesOf4p);
	ITERATE_OVER(gamesOf5p);
	ITERATE_OVER(gamesOf6p);
	ITERATE_OVER(gamesOf7p);
	ITERATE_OVER(gamesOf8p);
	ITERATE_OVER(customGames);
	ITERATE_OVER(QMGames);
	
	if (stats.locale > 0)
	{
		sprintf(kvbuf, "\\locale\\%d", stats.locale);
		s.append(kvbuf);
	}

	if (stats.gamesAsRandom > 0)
	{
		sprintf(kvbuf, "\\random\\%d", stats.gamesAsRandom);
		s.append(kvbuf);
	}

	if (stats.options.length())
	{
		_snprintf(kvbuf, 256, "\\options\\%s", stats.options.c_str());
		kvbuf[255] = 0;
		s.append(kvbuf);
	}

	if (stats.systemSpec.length())
	{
		_snprintf(kvbuf, 256, "\\systemSpec\\%s", stats.systemSpec.c_str());
		kvbuf[255] = 0;
		s.append(kvbuf);
	}

	if (stats.lastFPS > 0.0f)
	{
		sprintf(kvbuf, "\\fps\\%g", stats.lastFPS);
		s.append(kvbuf);
	}
	if (stats.lastGeneral >= 0)
	{
		sprintf(kvbuf, "\\lastGeneral\\%d", stats.lastGeneral);
		s.append(kvbuf);
	}
	if (stats.gamesInRowWithLastGeneral >= 0)
	{
		sprintf(kvbuf, "\\genInRow\\%d", stats.gamesInRowWithLastGeneral);
		s.append(kvbuf);
	}
	if (stats.builtParticleCannon >= 0)
	{
		sprintf(kvbuf, "\\builtCannon\\%d", stats.builtParticleCannon);
		s.append(kvbuf);
	}
	if (stats.builtNuke >= 0)
	{
		sprintf(kvbuf, "\\builtNuke\\%d", stats.builtNuke);
		s.append(kvbuf);
	}
	if (stats.builtSCUD >= 0)
	{
		sprintf(kvbuf, "\\builtSCUD\\%d", stats.builtSCUD);
		s.append(kvbuf);
	}
	if (stats.challengeMedals > 0)
	{
		sprintf(kvbuf, "\\challenge\\%d", stats.challengeMedals);
		s.append(kvbuf);
	}
	if (stats.battleHonors > 0)
	{
		sprintf(kvbuf, "\\battle\\%d", stats.battleHonors);
		s.append(kvbuf);
	}

	//if (stats.winsInARow > 0)
	{
		sprintf(kvbuf, "\\WinRow\\%d", stats.winsInARow);
		s.append(kvbuf);
	}
	if (stats.maxWinsInARow > 0)
	{
		sprintf(kvbuf, "\\WinRowMax\\%d", stats.maxWinsInARow);
		s.append(kvbuf);
	}

	//if (stats.lossesInARow > 0)
	{
		sprintf(kvbuf, "\\LossRow\\%d", stats.lossesInARow);
		s.append(kvbuf);
	}
	if (stats.maxLossesInARow > 0)
	{
		sprintf(kvbuf, "\\LossRowMax\\%d", stats.maxLossesInARow);
		s.append(kvbuf);
	}

	//if (stats.disconsInARow > 0)
	{
		sprintf(kvbuf, "\\DCRow\\%d", stats.disconsInARow);
		s.append(kvbuf);
	}
	if (stats.maxDisconsInARow > 0)
	{
		sprintf(kvbuf, "\\DCRowMax\\%d", stats.maxDisconsInARow);
		s.append(kvbuf);
	}

	//if (stats.desyncsInARow > 0)
	{
		sprintf(kvbuf, "\\DSRow\\%d", stats.desyncsInARow);
		s.append(kvbuf);
	}
	if (stats.maxDesyncsInARow > 0)
	{
		sprintf(kvbuf, "\\DSRowMax\\%d", stats.maxDesyncsInARow);
		s.append(kvbuf);
	}

	if (stats.lastLadderPort > 0)
	{
		sprintf(kvbuf, "\\ladderPort\\%d", stats.lastLadderPort);
		s.append(kvbuf);
	}
	if (stats.lastLadderHost.length())
	{
		_snprintf(kvbuf, 256, "\\ladderHost\\%s", stats.lastLadderHost.c_str());
		kvbuf[255] = 0;
		s.append(kvbuf);
	}

	DEBUG_LOG(("Formatted persistent values as '%s'\n", s.c_str()));
	return s;
}

//-------------------------------------------------------------------------
