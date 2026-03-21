#include "network.h"
#include "console.h"
#include <cstring>

inline constexpr int NET_MAX_CONNECTIONS = 16;
inline constexpr int PORT = 7777;

#ifdef COMPILER_SDL_NET
#include "SDL/SDL_net.h"
#include "win.h"

TCPsocket sock_server;
TCPsocket sock_client[NET_MAX_CONNECTIONS];
IPaddress address;
SDLNet_SocketSet set;
char dat[NET_MAX_CONNECTIONS][1024 * 64];
int datpos[NET_MAX_CONNECTIONS];
int owners[NET_MAX_CONNECTIONS];
bool initnetwort = false;
IPaddress localhost;
#endif

Var* allowallips = nullptr;
int ownercount = 0;
bool isserver = false;

void initnetwork() {
	ownercount = 0;
	allowallips = reinterpret_cast<Var*>(setVar("ALLOWALLIPS", 0));
#ifdef COMPILER_SDL_NET
	SDLNet_Init();
	if (SDLNet_ResolveHost(&address, nullptr, PORT) == -1) return;

	set = SDLNet_AllocSocketSet(NET_MAX_CONNECTIONS);
	for (int i = 0; i < NET_MAX_CONNECTIONS; i++) datpos[i] = -1;

	initnetwort = true;

	if (!(sock_server = SDLNet_TCP_Open(&address))) return;

	isserver = true;
	SDLNet_ResolveHost(&localhost, "localhost", PORT);
#endif
}

void checknetwork() {
#ifdef COMPILER_SDL_NET
	if (!initnetwort) return;

	int freeSlot = -1;
	SDLNet_CheckSockets(set, 0);

	for (int i = 0; i < NET_MAX_CONNECTIONS; i++) {
		if (datpos[i] >= 0) {
			if (SDLNet_SocketReady(sock_client[i])) {
				const int received = SDLNet_TCP_Recv(sock_client[i], dat[i] + datpos[i], static_cast<int>(sizeof(dat[i])) - datpos[i]);

				if (received > 0) {
					datpos[i] += received;
					if (dat[i][datpos[i] - 1] == '\n') {
						dat[i][datpos[i] - 1] = '\0';
						parsechar(dat[i], owners[i], "Network");
						datpos[i] = 0;
					}
				} else {
					SDLNet_TCP_DelSocket(set, sock_client[i]);
					SDLNet_TCP_Close(sock_client[i]);
					sock_client[i] = nullptr;
					datpos[i] = -1;
					owners[i] = 0;
				}
			}
		} else freeSlot = i;
	}

	if (isserver && (freeSlot >= 0)) {
		sock_client[freeSlot] = SDLNet_TCP_Accept(sock_server);
		if (sock_client[freeSlot]) {
			IPaddress* ip = SDLNet_TCP_GetPeerAddress(sock_client[freeSlot]);
			if ((allowallips->value == 0) && ip && (ip->host != localhost.host)) {
				SDLNet_TCP_Close(sock_client[freeSlot]);
				sock_client[freeSlot] = nullptr;
			} else {
				datpos[freeSlot] = 0;
				owners[freeSlot] = ++ownercount;
				SDLNet_TCP_AddSocket(set, sock_client[freeSlot]);
				sendowner("BS2CE", ownercount);
			}
		}
	}
#endif
}

void sendowner(char* text, int owner, int size) {
#ifdef COMPILER_SDL_NET
	if (size == -1) size = (int)strlen(text);

	static const char newline[] = "\n";

	for (int i = 0; i < NET_MAX_CONNECTIONS; i++) {
		if (owners[i] == owner) {
			SDLNet_TCP_Send(sock_client[i], text, size);
			SDLNet_TCP_Send(sock_client[i], newline, 1);
		}
	}
#endif
}

int connect(char* host, int port) {
#ifdef COMPILER_SDL_NET
	int freeSlot = -1;
	for (int i = 0; i < NET_MAX_CONNECTIONS; i++)
		if (datpos[i] < 0) freeSlot = i;

	if (freeSlot < 0) {
		error("Too many connections.", 0);
		return 0;
	}

	if (isserver && !strcmp(host, "localhost") && (port == PORT)) {
		print(const_cast<char*>("Cannot connect to myself."), 0);
		return 0;
	}

	IPaddress addr;
	if (SDLNet_ResolveHost(&addr, host, static_cast<Uint16>(port)) < 0) {
		print(const_cast<char*>("Couldn't resolve hostname."), 0);
		return 0;
	}

	TCPsocket connection = SDLNet_TCP_Open(&addr);
	if (!connection) {
		print(const_cast<char*>("Couldn't connect."), 0);
		return 0;
	}

	sock_client[freeSlot] = connection;
	datpos[freeSlot] = 0;
	owners[freeSlot] = ++ownercount;
	SDLNet_TCP_AddSocket(set, sock_client[freeSlot]);
	return ownercount;
#endif
	return 0;
}

void disconnect() {
	SDL_Delay(500);
}