/* run using: ./load_gen localhost <server port> <number of concurrent users>
   <think time (in s)> <test duration (in s)> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>

float time_up;
FILE *log_file;
struct timeval i_start;
// user info struct
struct user_info {
  // user id
  int id;

  // socket info
  int portno;
  char *hostname;
  float think_time;

  // user metrics
  int total_count;
  float total_rtt;
};

// error handling function
void error(char *msg) {
  perror(msg);
  exit(0);
}

// time diff in seconds
float time_diff(struct timeval *t2, struct timeval *t1) {
  return (t2->tv_sec - t1->tv_sec) + (t2->tv_usec - t1->tv_usec) / 1e6;
}

// user thread function
void *user_function(void *arg) {
  /* get user info */
  struct user_info *info = (struct user_info *)arg;
  // printf("came in \n");
  int sockfd, n;
  char buffer[1024];
  // char readres[1000];
  struct timeval start,end;
  float temp=0;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  while (1) {
    /* start timer */
    gettimeofday(&start, NULL);
    // printf("inside the loop\n");
    /* TODO: create socket */
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
      error("error opening the socket");
    server=gethostbyname(info->hostname);
    // printf("opened a socket\nserver is %s\n",server);
    if(server==NULL)
      error("ERROR,no such host");

    /* TODO: set server attrs */
     // puts(server->h_addr);
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  // (char *) serv_addr.sin_addr.s_addr=(char *) server->h_addr;
  serv_addr.sin_port=htons(info->portno);
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  // serv_addr.sin_port = htons(info->portno);

    /* TODO: connect to server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");

    /* TODO: send message to server */
    bzero(buffer,1024);
    strcpy(buffer,"GET index.html HTTP/1.0  ");
    n=write(sockfd,buffer,strlen(buffer));
    if(n<0)
      error("Error on writing to socket");

    /* TODO: read reply from server */
    bzero(buffer,1024);
    n=read(sockfd,buffer,1024);
    if(n<0)
      error("Error on reading from socket");
    // printf("Server Response : %s \n",readres);

    /* TODO: close socket */
    close(sockfd);
    /* end timer */
    gettimeofday(&end, NULL);
    float crtt=0;
    crtt=time_diff(&end,&start);    // this total run time till now 
    /* if time up, break */
    

    /* TODO: update user metrics */
    info->total_rtt=(info->total_rtt)+(time_diff(&end,&start));
    info->total_count=(info->total_count)+1;
        // printf("%f\n",info->total_rtt);

    if (time_up)
      break;
    /* TODO: sleep for think time */
    sleep(1);
  }
  /* exit thread */
  fprintf(log_file, "User #%d finished\n", info->id);
  fflush(log_file);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  int user_count, portno, test_duration;
  float think_time;
  char *hostname;

  if (argc != 6) {
    fprintf(stderr,
            "Usage: %s <hostname> <server port> <number of concurrent users> "
            "<think time (in s)> <test duration (in s)>\n",
            argv[0]);
    exit(0);
  }

  hostname = argv[1];
  portno = atoi(argv[2]);
  user_count = atoi(argv[3]);
  think_time = atof(argv[4]);
  test_duration = atoi(argv[5]);
  time_up=test_duration;
  // printf("Hostname: %s\n", hostname);
  // printf("Port: %d\n", portno);
  // printf("User Count: %d\n", user_count);
  // printf("Think Time: %f s\n", think_time);
  // printf("Test Duration: %d s\n", test_duration);

  /* open log file */
  log_file = fopen("load_gen.log", "w");

  pthread_t threads[user_count];
  struct user_info info[user_count];
  struct timeval start, end;

  /* start timer */
  gettimeofday(&start, NULL);
  time_up = 0;
   bzero(threads,user_count);
  for (int i = 0; i < user_count; ++i) {
    /* TODO: initialize user info */
    info[i].id=i;
    info[i].portno=portno;
    info[i].hostname=hostname;
    info[i].think_time=think_time;
    info[i].total_count=0;
    info[i].total_rtt=0;
    // pthread_create(&threads[i],NULL,&user_function,(void *)(info+i));
    pthread_create(&threads[i],NULL,user_function,&(info[i]));
    /* TODO: create user thread */
    fprintf(log_file, "Created thread %d\n", i);                                                                        
  }
  i_start=start;
  /* TODO: wait for test duration */
  sleep(test_duration);
  fprintf(log_file, "Woke up\n");
  void *r;
  /* end timer */
  time_up = 1;
  gettimeofday(&end, NULL);

  /* TODO: wait for all threads to finish */
  for(int k=0;k<user_count;k++)
  {
    pthread_join(threads[k],r);
  }

  /* TODO: print results */
  float sumrtt=0;
  int tot_reqs=0;
  for(int h=0;h<user_count;h++)
  {
    sumrtt=sumrtt+(info[h].total_rtt);
    tot_reqs=tot_reqs+(info[h].total_count);
  }
  float thput=0;
  float restm=0;
  thput=tot_reqs/test_duration;
  restm=sumrtt/tot_reqs;
  // restm=sumrtt/tot_reqs;
  // printf("total response time is %d \n",sumrtt);
  // printf("total requests handled is %d \n",tot_reqs);
  // printf("response time is %lf \n",restm);
  // printf("througput is %lf\n\n\n",thput);
  printf("%d,%f,%d,%d\n",user_count,sumrtt,tot_reqs,test_duration);

  /* close log file */
  fclose(log_file);

  return 0;
}