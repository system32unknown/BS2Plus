#include "network.h"
#include "console.h"

#define NET_MAX_CONNECTIONS 16
#define PORT 7777

#ifdef COMPILER_SDL_NET
#include "SDL/SDL_net.h"
#include "win.h"

TCPsocket sock_server, sock_client[NET_MAX_CONNECTIONS];
IPaddress address;
SDLNet_SocketSet set;
char dat[NET_MAX_CONNECTIONS][1024 * 64];
int datpos[NET_MAX_CONNECTIONS];
int owners[NET_MAX_CONNECTIONS];
bool initnetwort;
IPaddress localhost;

#endif

Var *allowallips;
int ownercount;
bool isserver = false;

void initnetwork()
{
	ownercount = 0;
	allowallips = (Var *)setVar("ALLOWALLIPS", 0);
#ifdef COMPILER_SDL_NET
	SDLNet_Init();
	if (SDLNet_ResolveHost(&address, NULL, 7777) == -1)
		return;
	set = SDLNet_AllocSocketSet(NET_MAX_CONNECTIONS);
	for (int i = 0; i < NET_MAX_CONNECTIONS; i++)
		datpos[i] = -1;
	initnetwort = true;
	if (!(sock_server = SDLNet_TCP_Open(&address)))
		return;
	isserver = true;
	SDLNet_ResolveHost(&localhost, "localhost", 7777);
#endif
}

void checknetwork()
{
#ifdef COMPILER_SDL_NET
	if (initnetwort)
	{
		int free = -1, tmp;
		SDLNet_CheckSockets(set, 0);
		for (int i = 0; i < NET_MAX_CONNECTIONS; i++)
		{
			if (datpos[i] >= 0)
			{
				if (SDLNet_SocketReady(sock_client[i]))
				{
					tmp = SDLNet_TCP_Recv(sock_client[i], (dat[i]) + datpos[i], 1024 * 64 - datpos[i]);
					if (tmp > 0)
					{
						datpos[i] += tmp;
						if (dat[i][datpos[i] - 1] == '\n')
						{
							dat[i][datpos[i] - 1] = 0;
							parsechar(dat[i], owners[i], "Network");
							datpos[i] = 0;
						}
					}
					else
					{
						SDLNet_TCP_Recv(sock_client[i], (dat[i]) + datpos[i], 1024 * 64 - datpos[i]);
						SDLNet_TCP_DelSocket(set, sock_client[i]);
						datpos[i] = -1;
						owners[i] = 0;
					}
				}
			}
			else
				free = i;
		}
		if (isserver && (free >= 0))
		{
			sock_client[free] = SDLNet_TCP_Accept(sock_server);
			if (sock_client[free])
			{
				IPaddress *ip = SDLNet_TCP_GetPeerAddress(sock_client[free]);
				if ((allowallips->value == 0) && (ip->host != localhost.host))
				{
					SDLNet_TCP_Close(sock_client[free]);
				}
				else
				{
					datpos[free] = 0;
					owners[free] = ++ownercount;
					SDLNet_TCP_AddSocket(set, sock_client[free]);
					sendowner("BS2Plus", ownercount);
				}
			}
		}
	}
#endif
}

void sendowner(char *text, int owner, int size)
{
#ifdef COMPILER_SDL_NET
	if (size == -1)
		size = strlen(text);
	static char *newline = "\n";
	for (int i = 0; i < NET_MAX_CONNECTIONS; i++)
		if (owners[i] == owner)
		{
			SDLNet_TCP_Send(sock_client[i], text, size);
			SDLNet_TCP_Send(sock_client[i], newline, 1);
		}
#endif
}

int connect(char *address, int port)
{
#ifdef COMPILER_SDL_NET
	TCPsocket connection = 0;
	int free = -1;
	for (int i = 0; i < NET_MAX_CONNECTIONS; i++)
		if (datpos[i] < 0)
			free = i;
	if (free >= 0)
	{
		if (isserver && !strcmp(address, "localhost") && (port == 7777))
		{
			char *t = "Cannot connect to myself.";
			print(t, 0, strlen(t));
			return 0;
		}
		IPaddress addr;
		if (SDLNet_ResolveHost(&addr, address, port) < 0)
		{
			char *t = "Couldn't resolve hostname.";
			print(t, 0, strlen(t));
			return 0;
		}
		connection = SDLNet_TCP_Open(&addr);
		if (connection == NULL)
		{
			char *t = "Couldn't connect.";
			print(t, 0, strlen(t));
			return 0;
		}

		sock_client[free] = connection;
		if (sock_client[free])
		{
			datpos[free] = 0;
			owners[free] = ++ownercount;
			SDLNet_TCP_AddSocket(set, sock_client[free]);
		}
		return ownercount;
	}
	error("Too many connections.", 0);
#endif
	return 0;
}

void disconnect(int t)
{
	t = 0;
	SDL_Delay(500);
}
