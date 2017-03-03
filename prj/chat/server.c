#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <errno.h>
#include <string.h>   
#include <arpa/inet.h>    
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 
  
#define TRUE   1
#define FALSE  0
#define PORT 5555 
#define MAXMSG 1024 //size of buffer 
#define MAXNICK 16

int main(int argc , char *argv[])
{
    int opt = TRUE;
    int ms , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , k , valread , sd;
    char  client_name[max_clients][MAXNICK];
    int max_sd;
    struct sockaddr_in address;
      
    char buffer[MAXMSG];
    char mes[MAXMSG+MAXNICK+10]; //message to send

    fd_set readfds;
    
    //set array of sockets with 0
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }
      
    if( (ms = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections 
    if( setsockopt(ms, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
   
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    if ( bind(ms, (struct sockaddr *)&address, sizeof(address)) < 0 ) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);
     
    if (listen(ms, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
    
    char *message = "Session started! \nset your nickname: ";
     
    while(TRUE) 
    {
        FD_ZERO(&readfds); //clear set
        FD_SET(ms, &readfds);//add master socket to set
        max_sd = ms;
          
        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++) 
        {
            sd = client_socket[i];
             
            //if valid socket descriptor then add to read list
            if( sd > 0 )
                FD_SET( sd , &readfds );
             
            //compare socket descriptor on max value
            if(sd > max_sd)
                max_sd = sd;
        }
  
        //waiting for change on one of the sockets
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) //EINTR - call was interrupted
        {
            perror("select error");
        }
          
        //if master socket changed - incoming connection - accept it
        if (FD_ISSET(ms, &readfds)) 
        {
            if ((new_socket = accept(ms, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
          
            printf("New connection, socket fd : %d , ip : %s , port : %d \n" , \
                    new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
        
            //echo message to connected client

            message = "Session started! \nset your nickname: ";
            if( send(new_socket, message, strlen(message), 0) != strlen(message) ) 
            {
                perror("send failed");
            }
              
            puts("...welcome message sent");

            //add new socket
            for (i = 0; i < max_clients; i++) 
            {
                //searching on empty position
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                    
                    /*
                      here it is some bug - server hungs while client enter his nick - need to fix
                      ideas: 1) to move read nick block to send message block
                             2) to use fork() and to left all nicking job on son
                             3) to create alarm() signal and to kick client, if he entering nick too long
                    */
                    char sym;

                    for   (k = 0; k < MAXNICK; ++k ) 
                     if ( (valread = read( new_socket, &sym, 1 )) == 0 || sym == '\n' || sym == ' ') break; 
                     else
                     {
                        client_name[i][k] = sym;
                        write(1,&sym,1); //cout nick to server log
                     } 
                        write(1,"\n",1); 
                    
                    if ( k == 0 || valread == 0 || sym == ' ')
                    {
                       //end of file - somebody disconnected | empty nick 
                       
                       if ( sym == ' ')
                            message = "kicked: nickname with spaces\n";
                       else if (k)
                            message = "kicked: empty nickname\n";
                       else 
                            message = "\ndisconected\n";
                       send(new_socket, message, strlen(message), 0); 

                       getpeername(new_socket , (struct sockaddr*)&address , (socklen_t*)&addrlen); //address of peer connected from sd
                       printf("Host disconnected , ip : %s , port : %d , name : %s\n" , 
                            inet_ntoa(address.sin_addr) , ntohs(address.sin_port) , client_name[i]);
                     
                       shutdown( new_socket ,2); // close connection
                       close( new_socket );
                       client_socket[i] = 0;
                       strcpy(client_name[i], "\0");
                    }
                    else //nick entered right
                    {
                        message = " joined the chat!\n\0";
                        strncpy(mes,client_name[i],k);
                        mes[k] = '\0';
                        strcat(mes,message);
                        for (k = 0; k < max_clients; k++) //send join info
                        {
                            if (client_socket[k] == 0) continue;
                            sd = client_socket[k]; 
                            send(sd , mes , strlen(mes) , 0 );
                        }
                    }
                    break;
                }
            }
        } 
        //else - existing socket
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];

            if (FD_ISSET( sd , &readfds)) 
            {
                if ((valread = read( sd , buffer, MAXMSG)) == 0)
                {
                    //end of file - somebody disconnected
                    message = "\ndisconected\n";
                    send(sd, message, strlen(message), 0); 

                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen); //address of peer connected to sd
                    printf("Host disconnected , ip : %s , port : %d , name : %s\n" , 
                            inet_ntoa(address.sin_addr) , ntohs(address.sin_port) , client_name[i]);
                      
                    shutdown(sd,2); // close connection
                    close( sd );
                    client_socket[i] = 0;
                    message = " left the chat!\n\0";
                    for(k = 0; k < MAXNICK; ++k) //length of nick
                        if ( client_name[i][k] == '\n' || client_name[i][k] == '\0' )
                            break;
                    strncpy(mes,client_name[i],k);
                    mes[k] = '\0';
                    strcat(mes,message);
                    for (k = 0; k < max_clients; k++) //send join info
                    {
                        if (client_socket[k] == 0) continue;
                        sd = client_socket[k]; 
                        send(sd , mes , strlen(mes) , 0 );
                    }
                    strcpy(client_name[i], "\0");
                }
                  
                //message forwarding - redirect
                else
                { 
                    if ( buffer[0] == '\n' ) continue;
                    buffer[valread] = '\0';
                    
                    //skipping spaces from start
                    int skip;
                    for(skip = 0; buffer[skip] == ' '; ++skip)
                    {}

                    //message creation
                    for(k = 0; k < MAXNICK; ++k) //length of nick
                        if ( client_name[i][k] == '\n' || client_name[i][k] == '\0' )
                            break;
                    strncpy(mes, client_name[i], k);
                    client_name[i][k] = '\n';
                    mes[k] = '\0';

                    strcat(mes, ": \0" );
                    strcat(mes, buffer+skip);
                    //message send
                    for (k = 0; k < max_clients; k++)
                    {
                        sd = client_socket[k]; //recv socket
                        if (k == i) //skip send to yourself
                        {
                            continue;
                        }
                        send(sd , mes , strlen(mes) , 0 );
                    }
            
                }
            }
        }
    }
      
    return 0;
} 
