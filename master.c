#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#define maxPlayers 100
#define maxSize 8196

struct player
{
	int player_id;
	char hostname[100];
   	int listening_port;
    int player_master_socket_fd;
};

struct player players[maxPlayers];

int main(int argc, void* argv[])
{
    if(argc != 4)
    {
        printf("Error : Invalid Arguments - takes 3 arguments - [port_number] [number_of_players] [hops]\n");
        exit(1);
    }
    int dataTransfer;
    int port_number = atoi(argv[1]);
    int numOfPlayers = atoi(argv[2]);
    int hops = atoi(argv[3]);
    if(port_number < 1024 || port_number > 65535)
    {
    	printf("Error : Invalid port number.\n");
        exit(1);
    }
    if(numOfPlayers > 1)
    {
        if(hops >= 0)
        {
            char hostname[100];
            gethostname(hostname,sizeof(hostname));
            printf("Potato Master on %s\nPlayers = %d\nHops = %d\n", hostname, numOfPlayers, hops);
            struct sockaddr_in master;
            int masterSocketFD = socket(AF_INET, SOCK_STREAM, 0);
            if(masterSocketFD < 0)
            {
                perror("Error : Unable to create socket for master : ");
                exit(1);
            }
            int flag = 1;
            int isSocOpt = setsockopt(masterSocketFD, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
            if (isSocOpt < 0) 
            {
	            perror("Error : SockOpt : ");
                exit(1);
        	}
            bzero((char *) &master, sizeof(master));
            master.sin_family = AF_INET;
            master.sin_addr.s_addr = INADDR_ANY;
            master.sin_port = htons(port_number);
            int isBinded = bind(masterSocketFD, (struct sockaddr *) &master, sizeof(master));
            if(isBinded < 0)
            {
                perror("Error : Unable to bind socket : ");
                exit(1);
            }
            int isListening = listen(masterSocketFD, numOfPlayers);
            if(isListening < 0)
            {
                perror("Error : Unable to listen to the socket : ");
                exit(1);
            }
            int playerCount = 0;
            while(playerCount < numOfPlayers)
            {
                struct sockaddr_in player;
                socklen_t p_len = sizeof(player);
                int playerMasterSocketFD = accept(masterSocketFD, (struct sockaddr *) &player, &p_len);
                if(playerMasterSocketFD < 0)
                {
                    printf("Error : Cannot accept connection for player %d\n", playerCount);
                    perror("Error : Unable to accept connection : ");
                    exit(1);
                }
                dataTransfer = send(playerMasterSocketFD, &playerCount, sizeof(int), 0);
                if(dataTransfer < 0)
                {
                    printf("Error : Cannot send Player ID to player %d\n", playerCount);
                    perror("Error : Unable to send data : ");
                    exit(1);
                }
                dataTransfer = send(playerMasterSocketFD, &numOfPlayers, sizeof(int), 0);
                if(dataTransfer < 0)
                {
                    printf("Error : Cannot send No. of Players to player %d\n", playerCount);
                    perror("Error : Unable to send data : ");
                    exit(1);
                }
                dataTransfer = send(playerMasterSocketFD, &hops, sizeof(int), 0);
                if(dataTransfer < 0)
                {
                    printf("Error : Cannot send hops to player %d\n", playerCount);
                    perror("Error : Unable to send data : ");
                    exit(1);
                }
                struct hostent* player_hostent = gethostbyaddr((char *)&player.sin_addr, sizeof(struct in_addr), AF_INET);
                int playerPort;
                dataTransfer = recv(playerMasterSocketFD, &playerPort, sizeof(int), 0);
                if(dataTransfer < 0)
                {
                    printf("Error : Cannot receive data from player %d\n", playerCount);
                    perror("Error : Unable to receive data : ");
                    exit(1);
                }
                printf("Player %d is on %s\n", playerCount, player_hostent->h_name);
                players[playerCount].player_id = playerCount;
                players[playerCount].player_master_socket_fd = playerMasterSocketFD;
                strcpy(players[playerCount].hostname, player_hostent->h_name);
                players[playerCount].listening_port = playerPort;
                playerCount++;
            }
            if(hops == 0)
            {
            	printf("Hops = 0\nShutting down.\n");
            	char gameover[100];
            	sprintf(gameover, "%s", "shutdown");
                for(playerCount = 0; playerCount < numOfPlayers; playerCount++)
                {
                        dataTransfer = send(players[playerCount].player_master_socket_fd, gameover, sizeof(gameover), 0);
                        if(dataTransfer < 0)
		                {
		                    printf("Error : Cannot send data to player %d\n", playerCount);
		                    perror("Error : Unable to send data : ");
		                }
                        close(players[playerCount].player_master_socket_fd);        
                }
                exit(0);
            }
            char msg[500];
            for(playerCount = 0; playerCount < numOfPlayers; playerCount++)
            {
            	bzero(msg, 500);
            	sprintf(msg, "accept");
            	dataTransfer = send(players[playerCount].player_master_socket_fd, msg, sizeof(msg), 0);
            	if(dataTransfer < 0)
                {
                    printf("Error : Cannot send message to player %d\n", playerCount);
                    perror("Error : Unable to send message : ");
                    exit(1);
                }
                int ack_recv = 0;
                int ack = recv(players[playerCount].player_master_socket_fd, &ack_recv, sizeof(ack_recv), 0);
                if(playerCount != (numOfPlayers - 1))
                {
	                bzero(msg, 500);
	                sprintf(msg, "%s;%d", players[playerCount].hostname, players[playerCount].listening_port);
	                dataTransfer = send(players[playerCount+1].player_master_socket_fd, msg, sizeof(msg), 0);
	            	if(dataTransfer < 0)
	                {
	                    printf("Error : Cannot send message to player %d\n", playerCount+1);
	                    perror("Error : Unable to send message : ");
	                    exit(1);
	                }
	                ack = recv(players[playerCount+1].player_master_socket_fd, &ack_recv, sizeof(ack_recv), 0);
	            }
	            else
	            {
	            	bzero(msg, 500);
	                sprintf(msg, "%s;%d", players[playerCount].hostname, players[playerCount].listening_port);
	                dataTransfer = send(players[0].player_master_socket_fd, msg, sizeof(msg), 0);
	            	if(dataTransfer < 0)
	                {
	                    printf("Error : Cannot send message to player %d\n", playerCount+1);
	                    perror("Error : Unable to send message : ");
	                    exit(1);
	                }
                	ack = recv(players[0].player_master_socket_fd, &ack_recv, sizeof(ack_recv), 0);
	            };              
            }
            srand(time(NULL));
            //srand(31);
            int random_player = rand() % numOfPlayers;
            printf("All players present, sending potato to player %d\n", random_player);
            char* hotPotato = malloc(maxSize * sizeof(char));
            sprintf(hotPotato, "%d|%s", hops, "Trace : ");
            int len = strlen(hotPotato);
            dataTransfer = send(players[random_player].player_master_socket_fd, hotPotato, len, 0);
        	if(dataTransfer < 0)
            {
                printf("Error : Cannot send potato to player %d\n", random_player);
                perror("Error : Unable to send potato : ");
                exit(1);
            }
            memset(hotPotato ,0 , ((hops * 2) + 50));
            fd_set readfds;
    		int maxFD;
    		FD_ZERO(&readfds);
			FD_SET(players[0].player_master_socket_fd, &readfds);
    		maxFD = players[0].player_master_socket_fd;
			for(playerCount = 1; playerCount < numOfPlayers; playerCount++)
			{
				FD_SET(players[playerCount].player_master_socket_fd, &readfds);
				if(maxFD < players[playerCount].player_master_socket_fd)
				{
					maxFD = players[playerCount].player_master_socket_fd;
				}
			}
			dataTransfer = select(maxFD + 1, &readfds, NULL, NULL, NULL);
			if (dataTransfer < 0) 
			{
			    perror("Error : Select Failed : ");
			    exit(1);
			}
			for(playerCount = 0; playerCount < numOfPlayers; playerCount++)
			{
				if(FD_ISSET(players[playerCount].player_master_socket_fd, &readfds))
				{
					dataTransfer = recv(players[playerCount].player_master_socket_fd, hotPotato, ((hops * 2) + 50), 0);
			        if (dataTransfer < 0) 
			        {
			            perror("Error : Unable to receive potato : ");
			            exit(1);
			        }
			        break;
				}
			}
			char* tmp = strtok(hotPotato, ":");
	        char* trace;
	        tmp = strtok(NULL, ":");
	        if(tmp)
	        {
	            trace = tmp;
	        }
            printf("Trace of potato:\n%s\n", trace+1);
			for(playerCount = 0; playerCount < numOfPlayers; playerCount++)
			{
				dataTransfer = send(players[playerCount].player_master_socket_fd, hotPotato, strlen(hotPotato), 0);
	        	if(dataTransfer < 0)
	            {
	                printf("Error : Cannot send potato to player %d\n", playerCount);
	                perror("Error : Unable to send potato : ");
	                exit(1);
	            }
	            close(players[playerCount].player_master_socket_fd);
			}
        }
        else
        {
            printf("Error : Hops should be atleast 0.\n");
            exit(1);
        }
    }
    else
    {
        printf("Error : Number of players should be greater than 1.\n");
        exit(1);
    }
    return 0;
}
