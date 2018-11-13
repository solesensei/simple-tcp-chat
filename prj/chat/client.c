#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define MAXMSG 1024

int main (int argc, char const *argv[])
{
    int port, n, sd, rval, sval;
    struct hostent *phe;
    struct sockaddr_in sin;

    if (argc != 3) 
    {
        fprintf(stderr, "not enough arguments, usage:\n./client ip port\n");
        exit(1);
    }
    //get host address via second argument
    if ( !(phe = gethostbyname(argv[1])) ) 
    {
        fprintf(stderr, "bad host address: %s\n", argv[1]);
        exit(1);
    }
    //read port and its length
    if ( sscanf(argv[2],"%d%n",&port, &n) != 1
        || argv[2][n] || port <=0 || port > 65535 ) 
    {
        fprintf(stderr, "bad port number: %s\n",argv[2]);
        exit(1);
    }

    if ( (sd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    memcpy( &sin.sin_addr, phe->h_addr_list[0], sizeof(sin.sin_addr));

    if (connect(sd, (struct sockaddr*) &sin, sizeof(sin)) < 0)
    {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    
    int rbuf[MAXMSG]; //read
    int sbuf[MAXMSG]; //send

    int pid = fork(); //here creating son
    sval = rval = 1;
    while (sval && rval )
    {   
        //recv
        if (pid == 0) //son
        {
            if ( (rval = read(sd , &rbuf , sizeof(rbuf))) ) 
            {
                rbuf[rval] = '\0';
                write(1, &rbuf, rval);
            }
        }else{ //send | father
            if ( (sval = read(0 , &sbuf , sizeof(sbuf))) ) 
            {
                sbuf[sval] = '\0';
                send(sd, &sbuf, sval,0);
            }
        }
        
    }
    /* here should to add message about server disconection*/
    if (pid == 0) exit(0); //exit son
    kill(pid,SIGKILL); //kill son
    wait(NULL); // wait for killing
    shutdown(sd,2);
    close(sd);

    return 0;

}
