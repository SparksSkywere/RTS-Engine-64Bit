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

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "GameNetwork/IPEnumeration.h"

// -------------------------------------------------------------------------
// IP address enumeration via GetIpAddrTable from iphlpapi.dll.
//
// Prior approach used WSAIoctl(SIO_GET_INTERFACE_LIST) which requires
// creating a UDP socket.  On some machines (VPN software, certain antivirus,
// misconfigured Hyper-V/WSL networking) the socket() call can block for
// several seconds, causing the Options menu to hard-freeze.
//
// GetIpAddrTable queries the Windows IP routing table directly through the
// IP Helper API — no Winsock socket is created, no DNS lookup is performed,
// and the call returns immediately.
// -------------------------------------------------------------------------


IPEnumeration::IPEnumeration( void )
{
	m_IPlist = NULL;
	m_isWinsockInitialized = false;
}

IPEnumeration::~IPEnumeration( void )
{
	if (m_isWinsockInitialized)
	{
		WSACleanup();
		m_isWinsockInitialized = false;
	}

	EnumeratedIP *ip = m_IPlist;
	while (ip)
	{
		ip = ip->getNext();
		m_IPlist->deleteInstance();
		m_IPlist = ip;
	}
}

EnumeratedIP * IPEnumeration::getAddresses( void )
{
	if (m_IPlist)
		return m_IPlist;

	// -----------------------------------------------------------------------
	// MIB_IPADDRROW / MIB_IPADDRTABLE inline definitions.
	// These mirror the Windows structures but are declared here to avoid
	// pulling in iphlpapi.h (which can transitively include winsock2.h and
	// conflict with the winsock.h already included via udp.h).
	// -----------------------------------------------------------------------
#pragma pack(push, 4)
	struct CNC_MIB_IPADDRROW {
		DWORD dwAddr;       // IP address  (network byte order)
		DWORD dwIndex;      // interface index
		DWORD dwMask;       // subnet mask (network byte order)
		DWORD dwBCastAddr;  // broadcast address
		DWORD dwReasmSize;  // max reassembled size
		WORD  unused1;
		WORD  wType;        // MIB_IPADDR_PRIMARY=0x0001, etc.
	};
	struct CNC_MIB_IPADDRTABLE {
		DWORD            dwNumEntries;
		CNC_MIB_IPADDRROW table[64]; // 64 adapters is far more than any real machine
	};
#pragma pack(pop)
	typedef DWORD (__stdcall *PFN_GetIpAddrTable)(CNC_MIB_IPADDRTABLE*, ULONG*, BOOL);

	// Prefer an already-loaded module handle to avoid ref-count churn.
	bool    bDoFree = false;
	HMODULE hIphlp  = GetModuleHandleA("iphlpapi.dll");
	if (!hIphlp) { hIphlp = LoadLibraryA("iphlpapi.dll"); bDoFree = true; }

	PFN_GetIpAddrTable pfnGetIpAddrTable = hIphlp
		? reinterpret_cast<PFN_GetIpAddrTable>(GetProcAddress(hIphlp, "GetIpAddrTable"))
		: nullptr;

	if (!pfnGetIpAddrTable) {
		if (bDoFree && hIphlp) FreeLibrary(hIphlp);
		DEBUG_LOG(("GetIpAddrTable not found in iphlpapi.dll\n"));
		return NULL;
	}

	CNC_MIB_IPADDRTABLE tbl = {};
	ULONG sz  = static_cast<ULONG>(sizeof(tbl));
	DWORD rc  = pfnGetIpAddrTable(&tbl, &sz, FALSE /*unsorted*/);
	if (bDoFree) FreeLibrary(hIphlp);

	if (rc != NO_ERROR) {
		DEBUG_LOG(("GetIpAddrTable failed: %lu\n", rc));
		return NULL;
	}

	DWORD numRows = (tbl.dwNumEntries < 64u) ? tbl.dwNumEntries : 64u;
	for (DWORD i = 0; i < numRows; ++i) {
		DWORD addr_nb = tbl.table[i].dwAddr;  /* network byte order */
		DWORD addr_ho = ntohl(addr_nb);        /* host byte order    */

		if (addr_ho == 0)             continue; /* 0.0.0.0   — skip  */
		if ((addr_ho >> 24) == 127)   continue; /* 127.x.x.x — skip  */

		const unsigned char *b = reinterpret_cast<const unsigned char *>(&addr_nb);
		AsciiString str;
		str.format("%d.%d.%d.%d", (int)b[0], (int)b[1], (int)b[2], (int)b[3]);

		EnumeratedIP *newIP = newInstance(EnumeratedIP);
		newIP->setIPstring(str);
		newIP->setIP(addr_ho);
		DEBUG_LOG(("IP: 0x%8.8X (%s)\n", addr_ho, str.str()));

		// Insert into sorted list (ascending IP order)
		if (!m_IPlist || newIP->getIP() < m_IPlist->getIP()) {
			newIP->setNext(m_IPlist);
			m_IPlist = newIP;
		} else {
			EnumeratedIP *p = m_IPlist;
			while (p->getNext() && p->getNext()->getIP() < newIP->getIP())
				p = p->getNext();
			newIP->setNext(p->getNext());
			p->setNext(newIP);
		}
	}

	return m_IPlist;
}

AsciiString IPEnumeration::getMachineName( void )
{
	if (!m_isWinsockInitialized)
	{
		WORD verReq = MAKEWORD(2, 2);
		WSADATA wsadata;

		int err = WSAStartup(verReq, &wsadata);
		if (err != 0) {
			return NULL;
		}

		if ((LOBYTE(wsadata.wVersion) != 2) || (HIBYTE(wsadata.wVersion) !=2)) {
			WSACleanup();
			return NULL;
		}
		m_isWinsockInitialized = true;
	}

	// get the local machine's host name
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)))
	{
		DEBUG_LOG(("Failed call to gethostname; WSAGetLastError returned %d\n", WSAGetLastError()));
		return NULL;
	}

	return AsciiString(hostname);
}


