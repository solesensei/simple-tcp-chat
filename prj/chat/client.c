#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXMSG 1024

int main (int argc, char const *argv[])
{
    int port, err, n, sd, rval, sval;
    struct addrinfo hints, *result, *rp;

    if (argc != 3) 
    {
        fprintf(stderr, "not enough arguments, usage:\n./client ip port\n");
        exit(1);
    }

    // Prepare the hints datastructure to help find a host.
    memset (&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // hostname, service (port), hints and results
    err = getaddrinfo(argv[1], argv[2], &hints, &result);

    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    
    //read port and its length
    /* if ( sscanf(argv[2],"%d%n",&port, &n) != 1
        || argv[2][n] || port <=0 || port > 65535 ) 
    {
        fprintf(stderr, "bad port number: %s\n",argv[2]);
        exit(1);
    } */
    
    // iterate through 'result' using 'rp' until hosts cannot be found
    // or break from the loop on success.
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sd == -1)
            continue;

        if (connect(sd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; // success connected

        close(sd);
    }

    if (rp == NULL)
    {
        fprintf(stderr, "Could not connect socket to address.\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    int rbuf[MAXMSG]; //read
    int sbuf[MAXMSG]; //send

    int pid = fork(); //here creating son
    sval = rval = 1;
    while (sval && rval)
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
