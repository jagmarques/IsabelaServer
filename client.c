#include "client.h"

void erro(char *msg);
int check_input_number(int *number);
void subs_socket(int rand_port);

int main(int argc, char *argv[]) {
	
	char endServer[100];
  char buffer[BUF_SIZE];
  int nread = 0;

  int fd;
  struct sockaddr_in addr;
  struct hostent *hostPtr;

	int verify_id=0;
  int enter_id=0;
  subscribed=0;

  strcpy(endServer, argv[1]);
  strcpy(host, argv[1]);
  strcpy(port, argv[2]);

	if ((hostPtr = gethostbyname(endServer)) == 0)
    	erro("I couldn't get this address");

  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

	if((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	  erro("Socket");
  if( connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
	  erro("Connect");

  char str_enter_id[MAX_STRING];

	while(verify_id == 0){
    nread = read(fd, buffer, BUF_SIZE - 1);
		buffer[nread] = '\0';
    
    printf("%s", buffer);
    check_input_number(&enter_id);

    sprintf(str_enter_id, "%d", enter_id);
    write(fd, str_enter_id, 1 + strlen(str_enter_id));

    nread = read(fd, buffer, BUF_SIZE - 1); 
    buffer[nread] = '\0';
    printf("%s\n", buffer);

    if(strcmp(buffer, "ID Accepted!") == 0){
      write(fd, "Leaving ID loop", 1 + strlen("Leaving ID loop"));
      verify_id = 1;
    }else if(strcmp(buffer, "ID Denied!") == 0){
      write(fd, "In ID loop", 1 + strlen("In ID loop"));
		}

  }
  
	char menu[MAX_STRING];
	char command[MAX_STRING];
	char data[MAX_STRING];
  char SUBS_PORT[MAX_STRING];
  int rand_port=0;


	int i=0;
	while(1) {
    sprintf(data, "Enter Menu with Sucess!");
    write(fd, data, 1 + strlen(data));

    nread = read(fd, menu, MAX_STRING - 1);
    menu[nread] = '\0';

    printf("%s", menu); 
    scanf("%s",command);
    write(fd, command, 1 + strlen(command));

    if(strcmp(command, "1")==0 || strcmp(command, "2")==0){
      nread = read(fd, data, MAX_STRING - 1);   
      data[nread] = '\0';
      printf("%s\n",data);
    }else if(strcmp(command, "3")==0 || strcmp(command, "4")==0){

      nread = read(fd, data, MAX_STRING - 1);   
      data[nread] = '\0';
      printf("%s",data);  

      if(strcmp(command, "4")==0){
        scanf("%s",command);
        write(fd, command, 1 + strlen(command));

        nread = read(fd, data, MAX_STRING - 1);   
        data[nread] = '\0';
        printf("%s", data);  

      }
      if(strcmp(data, "\nClient subscribed!\n")==0 || strcmp(data, "\nSingle data subscribed\n")==0){
        subscribed=1;
        srand(time(NULL));
        rand_port=atoi(port)+rand()%100;
        printf("SUBS PORT (calculate randomly): %d\n", rand_port);
        sprintf(SUBS_PORT, "%d", rand_port);
        write(fd, SUBS_PORT, 1 + strlen(SUBS_PORT));
        
        nread = read(fd, data, MAX_STRING - 1);   
        data[nread] = '\0';
        printf("%s",data);  

        
        if ((child=fork()) == 0){
          //printf("Child’s PID: %d\n", getpid());
          //printf("Value of n: %d\n", n);
          pids[n]=getpid();
					subs_socket(rand_port);
          exit(0);
				}
        
        //printf("Value of the pid in array pids[%d]: %d\n", n, pids[n]);
        //printf("Child: %d\n", child);
        
        if(pids[n]!=child){
          n++;
          pids[n]=child;
        }
      }
    
    }else if(strcmp(command, "EXIT")==0){
      if(subscribed==1){
        if(pids[1]!=child){
          for(int i=1; i<=n; i++)
            kill(pids[i], SIGKILL);
        }else
          kill(child, SIGKILL);
      }
      exit(0);
    }else{
      nread = read(fd, data, MAX_STRING - 1);   
      data[nread] = '\0';
      printf("\n%s\n",data);
    }
  }
}

void subs_socket(int rand_port){

  char endServer[100];
  char SUBS_PORT[100];
  char buffer[BUF_SIZE];
  char data[MAX_STRING];
  int nread = 0;

  int fd;
  struct sockaddr_in addr2;
  struct hostent *hostPtr;

  strcpy(endServer, host);

  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("I couldn't get subs address");

  bzero((void *) &addr2, sizeof(addr2));
  addr2.sin_family = AF_INET;
  addr2.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr2.sin_port = htons(rand_port);
  
  if((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    erro("Subs Socket");
  if( connect(fd,(struct sockaddr *)&addr2,sizeof (addr2)) < 0){
    erro("Subs Connect");
  }

  while(1){
    nread = read(fd, data, MAX_STRING - 1);   
    data[nread] = '\0';
    printf("%s",data);

    sprintf(data, "Average Data Updated with Sucess!");
    write(fd, data, 1 + strlen(data));
  }

}

int check_input_number(int *number){
	if (scanf("%d",number)==1){
		return 0;
  }else{
		scanf("%*s"); //<--this will read and discard whatever caused scanf to fail 
    printf(">>>Wrong format, try again!\n");
    printf(">");
    fflush(stdout);
    check_input_number(number); // start over
  }
  return 0;
}

void erro(char *msg){
	printf("Error: %s\n", msg);
	exit(-1);
}