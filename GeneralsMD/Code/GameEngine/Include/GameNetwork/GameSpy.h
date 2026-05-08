#ifndef __GAMESPY_STUB_H__
#define __GAMESPY_STUB_H__

// Transitional in-engine GameSpy compatibility surface.

#include "Common/AsciiString.h"
#include "GameSpy/Peer/Peer.h"

class GameSpyChatStub
{
public:
    Bool isConnected(void) const { return FALSE; }
    void reconnectProfile(void) {}
    Int getCurrentGroupRoomID(void) const { return 0; }
    PEER getPeer(void) const { return NULL; }
    AsciiString getLoginName(void) const { return AsciiString::TheEmptyString; }
};

static GameSpyChatStub g_GameSpyChatStub;
static GameSpyChatStub *TheGameSpyChat = &g_GameSpyChatStub;

#endif // __GAMESPY_STUB_H__
