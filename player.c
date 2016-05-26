#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define maxSize 8196 

int playerSocketFD;
int prevPlayerSocketFD;
int nextPlayerSocketFD;

int getAvailablePort()
{
    int port_no = -1;
    int p, isBinded;
    int maxPortNum = 65535; // [(2^16) - 1]
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFD < 0) 
    {
        printf("Error : Cannot create socket.\n");
        perror("Error : Unable to create socket : ");
        return -1;
    }
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    for(p = 1024; p <= maxPortNum; p++)
    {
        serv_addr.sin_port = htons(p);
        isBinded = bind(socketFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if(isBinded < 0)
        {
            continue;
        }
        else
        {
            port_no = p;
            break;
        }
    }
    playerSocketFD = socketFD;
    return port_no;
}

int main(int argc, char *argv[])
{
    if (argc != 3) 
    {
        printf("Error : Invalid Arguments - takes 2 arguments - [master_hostname] [port_number]\n");
        exit(1);
    }
    struct sockaddr_in player;
    struct hostent* player_hostent;
    int player_id, numOfPlayers, hops;
    int dataTransfer;
    int port_number = atoi(argv[2]);
    if(port_number < 1024 || port_number > 65535)
    {
        printf("Error : Invalid port number.\n");
        exit(1);
    }
    int playerMasterSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (playerMasterSocketFD < 0)
    {
        perror("Error : Unable to create socket : ");
        exit(1);
    }
    player_hostent = gethostbyname(argv[1]);
    if (player_hostent == NULL) 
    {
        perror("Error : Invalid hostname : ");
        exit(1);
    }
    bzero((char *) &player, sizeof(player));
    player.sin_family = AF_INET;
    bcopy((char *)player_hostent->h_addr, (char *)&player.sin_addr.s_addr, player_hostent->h_length);
    player.sin_port = htons(port_number);
    int isConnected = connect(playerMasterSocketFD, (struct sockaddr *) &player, sizeof(player));
    if (isConnected < 0) 
    {
        printf("Error : Unable to connect to master on %s\n", player_hostent->h_name);
        perror("Error : Unable to connect : ");
        exit(1);
    }
    dataTransfer = recv(playerMasterSocketFD, &player_id, sizeof(int), 0);
    if (dataTransfer < 0) 
    {
        printf("Error : Unable to read Player ID from master on %s\n", player_hostent->h_name);
        perror("Error : Unable to read from socket : ");
        exit(1);
    }
    printf("Connected as player %d\n", player_id);
    dataTransfer = recv(playerMasterSocketFD, &numOfPlayers, sizeof(int), 0);
    if (dataTransfer < 0) 
    {
        printf("Error : Unable to read No. of Players from master on %s\n", player_hostent->h_name);
        perror("Error : Unable to read from socket : ");
        exit(1);
    }
    dataTransfer = recv(playerMasterSocketFD, &hops, sizeof(int), 0);
    if (dataTransfer < 0) 
    {
        printf("Error : Unable to read hops from master on %s\n", player_hostent->h_name);
        perror("Error : Unable to read from socket : ");
        exit(1);
    }
    int port = getAvailablePort();
    if(port == -1)
    {
        printf("Error : Unable to find available port.\n");
        exit(1);
    }
    dataTransfer = send(playerMasterSocketFD, &port, sizeof(int), 0);
    if (dataTransfer < 0) 
    {
        printf("Error : Unable to send port to master on %s\n", player_hostent->h_name);
        perror("Error : Unable to send to socket : ");
        exit(1);
    }
    int connection = 0;
    char msg[500];
    while(connection < 2)
    {
        bzero(msg, 500);
        dataTransfer = recv(playerMasterSocketFD, &msg, sizeof(msg), 0);
        if (dataTransfer < 0) 
        {
            printf("Error : Unable to read from master on %s\n", player_hostent->h_name);
            perror("Error : Unable to read from socket : ");
            exit(1);
        }
        int isListening;
        if(strcmp(msg, "shutdown") == 0)
        {
            exit(0);
        }
        else if(strcmp(msg, "accept") == 0)
        {
            struct sockaddr_in neighbor;
            bzero((char *) &neighbor, sizeof(neighbor));
            int isListening = listen(playerSocketFD, 2);
            if(isListening < 0)
            {
                perror("Error : Unable to listen to the socket : ");
                exit(1);
            }
            socklen_t n_len = sizeof(neighbor);     
            int ack_recv = 0;
            int ack = send(playerMasterSocketFD, &ack_recv, sizeof(ack_recv), 0);
            nextPlayerSocketFD = accept(playerSocketFD, (struct sockaddr *) &neighbor, &n_len);
            if(nextPlayerSocketFD < 0)
            {
                perror("Error : Unable to accept connection : ");
                exit(1);
            }
        }
        else
        {
            struct sockaddr_in neighbor;
            char* tmp = strtok(msg, ";");
            char* host;
            int port;
            if(tmp)
            {
               host = tmp;
            }
            tmp = strtok(NULL, ";");
            if(tmp)
            {
                port = atoi(tmp);
            }
            prevPlayerSocketFD = socket(AF_INET, SOCK_STREAM, 0);
            if (prevPlayerSocketFD < 0)
            {
                perror("Error : Unable to create socket : ");
                exit(1);
            }
            player_hostent = gethostbyname(host);
            if (player_hostent == NULL) 
            {
                printf("Error : Invalid neighbor hostname : %s\n", host);
                perror("Error : Invalid hostname : ");
                exit(1);
            }
            bzero((char *) &neighbor, sizeof(neighbor));
            neighbor.sin_family = AF_INET;
            bcopy((char *)player_hostent->h_addr, (char *)&neighbor.sin_addr.s_addr, player_hostent->h_length);
            neighbor.sin_port = htons(port);  
            int ack_recv = 0;
            int ack = send(playerMasterSocketFD, &ack_recv, sizeof(ack_recv), 0);
            int isConnected = connect(prevPlayerSocketFD, (struct sockaddr *) &neighbor, sizeof(neighbor));
            if (isConnected < 0) 
            {
                printf("Error : Unable to connect to neighbor on %s\n", player_hostent->h_name);
                perror("Error : Unable to connect : ");
                exit(1);
            }
        }
        connection++;
    }
	//srand(time(NULL));
    srand(player_id);
    fd_set readfds;
    int maxFD;
    char* hotPotato = malloc(maxSize * sizeof(char));
    int len = (2 * hops) + 50;
    while(1) 
    {
        len = (2 * hops) + 50;
        memset(hotPotato ,0 , len);
        FD_ZERO(&readfds);
        FD_SET(playerMasterSocketFD, &readfds);
        maxFD = playerMasterSocketFD;
        FD_SET(prevPlayerSocketFD, &readfds);
        if(maxFD < prevPlayerSocketFD)
        {
            maxFD = prevPlayerSocketFD;
        }
        FD_SET(nextPlayerSocketFD, &readfds);
        if(maxFD < nextPlayerSocketFD)
        {
            maxFD = nextPlayerSocketFD;
        }
        dataTransfer = select(maxFD + 1, &readfds, NULL, NULL, NULL);
        if (dataTransfer < 0) 
        {
            perror("Error : Select Failed : ");
            exit(1);
        }
        if(FD_ISSET(playerMasterSocketFD, &readfds)) 
        {
            dataTransfer = recv(playerMasterSocketFD, hotPotato, len, 0);
            if (dataTransfer < 0) 
            {
                perror("Error : Unable to receive potato : ");
                exit(1);
            }
        }
        else if(FD_ISSET(prevPlayerSocketFD, &readfds))
        {
            dataTransfer = recv(prevPlayerSocketFD, hotPotato, len, 0);
            if (dataTransfer < 0) 
            {
                perror("Error : Unable to receive potato : ");
                exit(1);
            }
        }
        else if(FD_ISSET(nextPlayerSocketFD, &readfds))
        {
            dataTransfer = recv(nextPlayerSocketFD, hotPotato, len, 0);
            if (dataTransfer < 0) 
            {
                perror("Error : Unable to receive potato : ");
                exit(1);
            }
        }
        len = strlen(hotPotato);
        char potato[len];
        strcpy(potato, hotPotato);
        char* tmp = strtok(potato, "|");
        char* trace;
        int hops;
        if(tmp)
        {
           hops = atoi(tmp);
        }
        tmp = strtok(NULL, "|");
        if(tmp)
        {
            trace = tmp;
        };
        if(hops > 1)
        {
            hops--;
            if(strcmp(trace, "Trace : ") == 0)
            {
                sprintf(trace, "%s%d", trace, player_id);
            }
            else
            {
                sprintf(trace, "%s,%d", trace, player_id);
            }
            //bzero(hotPotato, len);
            sprintf(hotPotato, "%d|%s", hops, trace);
            len = strlen(hotPotato);
            int random_player = rand() % 2;
            int id;
            if(random_player == 0)
            {
                int id;
                if(player_id == 0)
                {
                    id = numOfPlayers - 1;
                }
                else
                {
                    id = player_id - 1;
                }
                printf("Sending potato to %d\n", id);
                dataTransfer = send(prevPlayerSocketFD, hotPotato, len, 0);
                if (dataTransfer < 0) 
                {
                    printf("Error : Unable to send potato to player %d\n", id);
                    perror("Error : Unable to send to socket : ");
                    exit(1);
                }
            }
            else if(random_player == 1)
            {
                int id;
                if(player_id == (numOfPlayers - 1))
                {
                    id = 0;
                }
                else
                {
                    id = player_id + 1;
                }
                printf("Sending potato to %d\n", id);
                dataTransfer = send(nextPlayerSocketFD, hotPotato, len, 0);
                if (dataTransfer < 0) 
                {
                    printf("Error : Unable to send potato to player %d\n", id);
                    perror("Error : Unable to send to socket : ");
                    exit(1);
                }
            }
        }
        else if(hops == 1)
        {
            hops--;
            if(strcmp(trace, "Trace : ") == 0)
            {
                sprintf(trace, "%s%d", trace, player_id);
            }
            else
            {
                sprintf(trace, "%s,%d", trace, player_id);
            }
            //bzero(hotPotato, len);
            sprintf(hotPotato, "%d|%s", hops, trace);
            len = strlen(hotPotato);
            printf("I'm it.\n");
            dataTransfer = send(playerMasterSocketFD, hotPotato, len, 0);
            if (dataTransfer < 0) 
            {
                printf("Error : Unable to send potato to master on %s\n", player_hostent->h_name);
                perror("Error : Unable to send to socket : ");
                exit(1);
            }
        }
        else if(hops == 0)
        {
            close(playerMasterSocketFD);
            close(playerSocketFD);
            close(nextPlayerSocketFD);
            close(prevPlayerSocketFD);
            break;
        }
    }
    return 0;
}