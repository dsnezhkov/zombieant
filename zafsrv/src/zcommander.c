#include "zcommander.h"

// TODO: is there a better way to handle commands as the grow. Also, refactor
int processCommandReq (int sock, node_t * head) {
   int  n = 0;
   char buffer[MAX_BUF];
   int  status = 0;

   cJSON * root         = NULL;
   cJSON * cmdName      = NULL;

   cJSON * loadmod_arg  = NULL;
   cJSON * loadmod_args = NULL;
   cJSON * modUrl       = NULL;
   cJSON * modName      = NULL;


   bzero(buffer,MAX_BUF);
   n = read(sock,buffer,MAX_BUF-1);

   if (n < 0) {
      log_error("Error reading from socket");
      return -1;
   }

   log_info("Request payload: >> %s <<",buffer);

   // Rudimentary checks for JSON. this library is SIGSEGV if invalid. We take that chance for now.
   // TODO: test if needed since wwe have implemented cJSON_Parse()
   if ( strchr(buffer, '}') == NULL \
                || strchr(buffer, '{') == NULL  \
                || strchr(buffer, '"') == NULL ){

        log_debug("Possibly invalid JSON, not parsing: %s", buffer); // not processing
        return 1;
   }

   root = cJSON_Parse(buffer);

   if (root == NULL)
   {
        log_warn("JSON: Error parsing");
        status=1;
        goto end;
   }

   cmdName = cJSON_GetObjectItemCaseSensitive(root, "command");

   if (cJSON_IsString(cmdName) && (cmdName->valuestring != NULL))
   {
     if (strcmp(cmdName->valuestring, "listmod") == 0) // TODO: allow short condensed string
     {
       log_debug("[Command] : %s", cmdName->valuestring);
       write_modlist(head, sock);
       status=1;
       goto end;

     }
     else if (strcmp(cmdName->valuestring, "loadmod") == 0)
     {

       loadmod_args = cJSON_CreateArray();
       if (loadmod_args == NULL)
       {
           log_debug("[Command] loadmod : cJSON_CreateArray()");
           status=1;
           goto end;
       }

       log_debug("[Command] : %s", cmdName->valuestring);
       loadmod_args = cJSON_GetObjectItemCaseSensitive(root, "args");
       cJSON_ArrayForEach(loadmod_arg, loadmod_args)
       {
           modUrl = cJSON_GetObjectItemCaseSensitive(loadmod_arg, "modurl");
           log_debug("[Command] : modurl (%s)", modUrl->valuestring);
           load_mod(head, modUrl->valuestring);
           goto end;
       }
     }
     else if (strcmp(cmdName->valuestring, "unloadmod") == 0)
     {
       loadmod_args = cJSON_CreateArray();
       if (loadmod_args == NULL)
       {
           log_debug("[Command] loadmod : cJSON_CreateArray()");
           status=1;
           goto end;
       }

       log_debug("[Command] : %s", cmdName->valuestring);
       loadmod_args = cJSON_GetObjectItemCaseSensitive(root, "args");
       cJSON_ArrayForEach(loadmod_arg, loadmod_args)
       {
           modName = cJSON_GetObjectItemCaseSensitive(loadmod_arg, "modname");
           log_debug("[Command] : modname (%s)", modName->valuestring);
           unload_mod(head, modName->valuestring);
           goto end;
       }
     }
     else {
       log_debug("Command request (%s)", cmdName->valuestring);
     }
   }

end:
   cJSON_Delete(root);

   return status;

}

// Command socket setup
int setupCmdP(void){

    // TODO: Allow Unix domain sockets
    unsigned int portno, clilen, sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr; // AF_INET
    // struct sockaddr_un serv_addr_un; // AF_UNIX
    int one = 1;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET
    // sockfd = socket(AF_UNIX, SOCK_STREAM, 0);   // AF_UNIX

    if (sockfd < 0) {
      log_error("Error opening socket()");
      return -1;
    }


    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 0; // random port // TODO: parameterize

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(0x7f000001L); // 127.0.0.1 // TODO: parameterize
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      log_error("Error on bind()");
      return -1; // TODO: make a pass to check all return codes to make them consistent
    }
    setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
    listen(sockfd,5); // TODO: paramterize

    log_info("Listening on port"); // TODO: provide the port
    clilen = sizeof(cli_addr);

    /* AF_UNIX 
    bzero((char *) &serv_addr_un, sizeof(serv_addr_un));
    strcpy(serv_addr_un.sun_path, "/tmp/zsock");
    listen(sockfd,5);
    */

    while (1) {
        
      log_info("Accepting requests");
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

      if (newsockfd < 0) {
         log_error("Error on accept()");
         continue;
      }

      // We are not forking as we want to to preserve the PID for /proc/ FD's
      processCommandReq(newsockfd, head);
      close(newsockfd);
    }

    return 0;
}

