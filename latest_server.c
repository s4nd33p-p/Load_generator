/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>

#define MAX_TOKEN_SIZE 200
#define MAX_NUM_TOKENS 200
#define MAX_SOCKBUFF_SIZE 10
int workers_list[MAX_SOCKBUFF_SIZE];
int incptr=0,decptr=0;
int count=0;
pthread_mutex_t mutex_var = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t emptys = PTHREAD_COND_INITIALIZER;
pthread_cond_t fulls = PTHREAD_COND_INITIALIZER;
int clientnum=0,clientcount=0;

char **tokenize(char *line);
struct HTTTPresponse {
  char *version;
  char *status;
  char *content_type;
  char *response;
  

};


char *sendresponse(char *path,int a);

void error(char *msg) {
  perror(msg);
  exit(1);
}
void *child(void *arg)
{
  while(1)
  {
  pthread_mutex_lock(&mutex_var);
    while(count==0)
      pthread_cond_wait(&fulls,&mutex_var);
  int hf=workers_list[decptr];
  decptr=(decptr+1)%MAX_SOCKBUFF_SIZE;
  count=count-1;
  pthread_cond_signal(&emptys);
  pthread_mutex_unlock(&mutex_var);
  worker_thread_fn(hf);
  }
}

void worker_thread_fn( int f)
{
  char buffer[2000];
  int mynum=clientnum;
 // while(1)
 //  {
    int newsockfd=f;
    bzero(buffer,2000);
    int n = read(newsockfd, buffer,2000);
    if (n < 0)
      {error("ERROR reading from socket");
      }
    if(n==0)
    {
      clientcount=clientcount-1;
      if(clientcount==0)
      {
        printf("All clients have been disconnected\n");
      }
    }
  if(n>0)  // a valid client message to be processsed
  {
      // printf("Here is the message: %s \n", buffer);

    /* send reply to client */
      if(strncmp("GET",buffer,3)==0)                    // GET http method handler
      {
        
        char **tokens=NULL;
        buffer[strlen(buffer)]='\n';
        char *response=NULL;
        tokens=tokenize(buffer);
        struct stat f;
        if(strncmp("HTTP/1.1",tokens[2],8)==0 || strncmp("HTTP/1.0",tokens[2],8)==0)
        {
        if((tokens[1][0]=='/')&&(strlen(tokens[1])>1))
        {
          tokens[1]=tokens[1]+1;
        }
        if(stat(tokens[1],&f)==0)
        {
            if( S_ISREG(f.st_mode) !=0)
            {
                                                      //this is a file
              
              response=sendresponse(tokens[1],1);
              n=write(newsockfd,response,1000);
              if(n<0)
                 error("Error writing to socket");

            }
            else if( S_ISDIR(f.st_mode) !=0)
            {
                                                      // this is directory
              int p=0;
              if(strlen(tokens[1])==1)
              {
               p=1;
               response=sendresponse("index.html",1);
               n=write(newsockfd,response,1000);
              }
              else
              {
                if(tokens[strlen(tokens[1])-1]=="/")
                      strncat(tokens[1],"index.html      ",10);
                else
                {
                  strncat(tokens[1],"/index.html      ",11);
                }
              response=sendresponse(tokens[1],1);
              n=write(newsockfd,response,1000);
              if(n<0)
                   error("Error writing to socket");
              }

            }

          }
          else
          {
            response=sendresponse("/home/sandy/Desktop/DECS/multiclientserver/notes/notfound.html",0); //html file for error notfound
            // char res[]="HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nConnection: close\r\n\n<!DOCTYPE html>\n<html>\n<head>\n<title>Document</title>\n</head>\n<body>\n<h1>404 Page Not Found </h1>\n</body>\n</html>\0";
            n=write(newsockfd,response,1000); 
            if(n<0)
            error("ERROR writing to socket");        
           }

    
           for(int k=0;k<MAX_NUM_TOKENS;k++)
           {
            free(tokens[k]);
           }
           free(tokens);
           free(response);
           // free(sample);
      }
      else
      {
        n=write(newsockfd,"Please use HTTP/1.0 or HTTP/1.1",32);
        if(n<0)
          error("ERROR writing to socket");
      }

   }
        else        // not GET method so just echo the message back 
            {
              n = write(newsockfd, buffer, 255);
              if (n < 0)
              error("ERROR writing to socket");
            }
  }
  close(newsockfd);
}


void handler(int sig)                                                      //SIGINT'S handler function
{
  exit(0);
}

// char *res=(char *)malloc(sizeof(char)*2048);

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;


  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }
  
  struct sigaction san;             //sigaction stucture.
    san.sa_handler=handler;
    san.sa_flags=0;
    sigemptyset (&san.sa_mask) ;
    sigaction(SIGINT,&san,NULL);

  /* create socket */

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
   */
 
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* bind socket to this port number on this machine */

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  /* listen for incoming connection requests */

  listen(sockfd, 50);
  clilen = sizeof(cli_addr);

  /* accept a new request, create a newsockfd */
   int workers=0;
  printf("enter the no.of worker threads in the pool :");
  scanf("%d",&workers);
  // static pthread_t wid[workers];
  while(workers>0)
  {
    pthread_t wid;
    pthread_create(&wid,NULL,child,NULL);
    workers-=1;

  }
  
while(1)
{

  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0)
    error("ERROR on accept");
  else
  {
    pthread_mutex_lock(&mutex_var);
    while(count==MAX_SOCKBUFF_SIZE)
      pthread_cond_wait(&emptys,&mutex_var);
    workers_list[incptr]=newsockfd;
    incptr=(incptr+1)%MAX_SOCKBUFF_SIZE;
    count=count+1;
    pthread_cond_signal(&fulls);
    pthread_mutex_unlock(&mutex_var);
    // printf("a new thred of sock fd : %d \n",newsockfd);

  }

}
  return 0;
}


char *sendresponse(char *path,int a)
{
            struct HTTTPresponse *fres=(struct HTTTPresponse *)malloc(sizeof(struct HTTTPresponse));
            bzero(fres,sizeof(struct HTTTPresponse));
            fres->response=(char *)malloc(sizeof(char)*1024);
            bzero(fres->response,1024);
            // bzero(fres,sizeof(struct HTTTPresponse));
            fres->version="HTTP/1.1 ";
            fres->content_type="Content-Type: text/html\r\n";
            char *conn=NULL;
            conn="Connection: close\r\n\n";
            FILE* p;
            p=fopen(path,"r");
            if(a==1)
            fres->status="200 OK\r\n";
            else
              fres->status="404 Not Found\r\n";
            int ind=0;
            char c;
            while(c!=EOF)
            {
              c=fgetc(p);
              fres->response[ind++]=c;
            }
            fclose(p);
            char str[20]={};
            sprintf(str,"Content-Length: %d\r\n",ind-1);
            char *res=(char *)malloc(2048);
            // static char res[2048];
            bzero(res,2048);
           
            strncat(res,fres->version,strlen(fres->version));
            strncat(res,fres->status,strlen(fres->status));
            strncat(res,str,strlen(str));
            strncat(res,fres->content_type,strlen(fres->content_type));
            strncat(res,conn,strlen(conn));
            strncat(res,fres->response,strlen(fres->response)-1);
            free(fres->response);
            free(fres);
            return res;
}


char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  bzero(tokens,MAX_NUM_TOKENS);
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  bzero(token,MAX_TOKEN_SIZE);
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
  tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
  // tokens[tokenNo]=NULL;
  bzero(tokens[tokenNo],MAX_TOKEN_SIZE);
  if(strcmp(token,"&")==0 && i+1==strlen(line)){
    continue;
  }
  strcpy(tokens[tokenNo++], token);
  tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}
