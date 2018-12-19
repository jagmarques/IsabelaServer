/*******************************************************************
 * SERVIDOR no porto 9000, à escuta de novos clientes.  Quando surjem
 * novos clientes os dados por eles enviados são lidos e descarregados no ecran.
 *******************************************************************/
#include "server.h"

void process_client(int fd);
void erro(char *msg);
void update_array_clients();
void update_average_data();
void subs_socket(int subs_port, int single_sub);
void create_new_fork(char buffer[], int client_fd, int single_sub);


int main(){
	int fd, client;
  	struct sockaddr_in addr, client_addr;
  	int client_addr_size;


	array_clients=(Client*)malloc(sizeof(Client)*MAX_CLIENTS);

	if (!array_clients) { /*If array_clients == 0 after the call to malloc, allocation failed for some reason*/
		perror("Error allocating memory");
		abort();
	}

	/*At this point, we know that data points to a valid block of memory.
  	Remember, however, that this memory is not initialized in any way - it contains garbage.
  	Let's start by clearing it.*/
  	memset(array_clients, 0, sizeof(Client)*MAX_CLIENTS);
  	/*Now our array contains all zeroes.*/
	
	
	bzero((void *) &addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); //Uso do big-endian, recebe através de qualquer das placas
	addr.sin_port = htons(SERVER_PORT);

	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) //Cria o socket
		erro("na funcao socket");
	if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0) //Ligas o socket
		erro("na funcao bind");
	if( listen(fd, 34) < 0) //Até 34 utilizadores
		erro("na funcao listen");

	while (1) {
		client_addr_size = sizeof(client_addr);
		//aceita a ligação criada do cliente
		client = accept(fd,(struct sockaddr *)&client_addr,(socklen_t *)&client_addr_size);
		if (client > 0) {
			if (fork() == 0) {
				close(fd);
				process_client(client);
				exit(0);
			}
		close(client);
		}
	}
	return 0;
}

void process_client(int client_fd){

	int nread = 0;
	int sum = 0;
	double average = 0;
	int x = 1;
	char buffer[BUF_SIZE];

	char str_clients_id[MAX_STRING];
	char str_groups_id[MAX_STRING];
	int i=0;
	int verify_id=0;
	int id_client, id_group;
	pid_t grandchild;

	update_array_clients();
	
	while(verify_id == 0){
		write(client_fd, "ID:", 1 + strlen("ID:"));

        nread = read(client_fd, buffer, BUF_SIZE - 1);
		buffer[nread] = '\0';
		
        id_client = atoi(buffer);
		printf("Enter ID: %d\n", id_client);
        
        while(array_clients[i].id!=id_client && array_clients[i].id!=0 && id_client!=0){
			i++;
		}
		if(array_clients[i].id==id_client)
			verify_id = 1;

        if(verify_id == 1)
            write(client_fd, "ID Accepted!", 1 + strlen("ID Accepted!"));
        else
            write(client_fd, "ID Denied!", 1 + strlen("ID Denied!"));
		
		
		nread = read(client_fd, buffer, BUF_SIZE - 1);
		buffer[nread] = '\0';
		printf("State: %s\n", buffer);

        i = 0;
    } 
	
	i=0;
	while(array_clients[i].id!=id_client && array_clients[i].id!=0 && id_client!=0){
		i++;
	}
	
	single_client=array_clients[i];
	//array_clients[i].subscription=0;
	i=0;

	char data[MAX_STRING];
	int exit_server=0;
	int single_sub;

	while(exit_server==0) {

		nread = read(client_fd, buffer, BUF_SIZE - 1);
		buffer[nread] = '\0';
		printf("Menssage received: %s\n", buffer);

        write(client_fd, "\nMENU:\n1)Data\n2)Average Data of all Clients\n3)Subscription\n4)Subscrive a single data\nEXIT\n>", 1 + strlen("\nMENU:\n\n1)Data\n2)Average Data of all Clients\n3)Subscription\nSubscrive a single data\nEXIT\n>"));
        
        nread = read(client_fd, buffer, BUF_SIZE - 1);
        buffer[nread] = '\0';
        
        if(strcmp(buffer, "1")==0){
			update_array_clients();

            sprintf(data, "\nAtivicty: %s\nLocation: %s\nCall duration: %d\nCalls made: %d\nCalls missed: %d\nCalls received: %d\nDepartment: %s\nSms received: %d\nSms sent: %d", single_client.activity, single_client.location, single_client.callsduration, single_client.callsmade, single_client.callsmissed, single_client.callsreceived, single_client.department, single_client.smsreceived, single_client.smssent);
            write(client_fd, data, strlen(data) + 1);
            
        }else if(strcmp(buffer, "2")==0){
			
			update_array_clients();
			update_average_data();

			sprintf(data, "\nCall duration: %lf\nCalls made: %lf\nCalls received: %lf\nCalls missed: %lf\nSms received: %lf\nSms sent: %lf", average_data.callsduration, average_data.callsmade, average_data.callsmissed, average_data.callsreceived, average_data.smsreceived, average_data.smssent);
			write(client_fd, data, 1 + strlen(data));

		}else if(strcmp(buffer, "3")==0){

			while(array_clients[i].id!=id_client && array_clients[i].id!=0 && id_client!=0){
				i++;
			}

			if(array_clients[i].subscription==1){
				sprintf(data, "\nClient already subscribed!\n");
				write(client_fd, data, 1 + strlen(data));
			}else{
				single_sub=0;
				
				array_clients[i].subscription=1;
				sprintf(data, "\nClient subscribed!\n");
				write(client_fd, data, 1 + strlen(data));

				nread = read(client_fd, buffer, BUF_SIZE - 1);
				buffer[nread] = '\0';

				create_new_fork(buffer, client_fd, single_sub);

			}
		}else if(strcmp(buffer, "4")==0){

			while(array_clients[i].id!=id_client && array_clients[i].id!=0 && id_client!=0){
				i++;
			}
				
			sprintf(data, "\n1)Calls duration\n2)Calls made\n3)Calls missed\n4)Calls received\n5)Sms received\n6)Sms sent\n>");
			write(client_fd, data, 1 + strlen(data));

			nread = read(client_fd, buffer, BUF_SIZE - 1);
			buffer[nread] = '\0';

			if(strcmp(buffer, "1")==0){
				if(array_clients[i].sub_callsduration==1){
					sprintf(data, "\nData already subscribed!\n");
					write(client_fd, data, 1 + strlen(data));
				}else{
					single_sub=1;
					array_clients[i].sub_callsduration=1;
					array_clients[i].single_sub=1;

					sprintf(data, "\nSingle data subscribed\n");
					write(client_fd, data, 1 + strlen(data));
					nread = read(client_fd, buffer, BUF_SIZE - 1);
					buffer[nread] = '\0';
								
					create_new_fork(buffer, client_fd, single_sub);
				}
			}else if(strcmp(buffer, "2")==0){
				if(array_clients[i].sub_callsmade==1){
					sprintf(data, "\nData already subscribed!\n");
					write(client_fd, data, 1 + strlen(data));
				}else{
					single_sub=2;
					array_clients[i].sub_callsmade=1;
					array_clients[i].single_sub=1;

					sprintf(data, "\nSingle data subscribed\n");
					write(client_fd, data, 1 + strlen(data));
					nread = read(client_fd, buffer, BUF_SIZE - 1);
					buffer[nread] = '\0';
					
					create_new_fork(buffer, client_fd, single_sub);
				}
			}else if(strcmp(buffer, "3")==0){
				if(array_clients[i].sub_callsmissed==1){
					sprintf(data, "\nData already subscribed!\n");
					write(client_fd, data, 1 + strlen(data));
				}else{
					single_sub=3;
					array_clients[i].sub_callsmissed=1;
					array_clients[i].single_sub=1;

					sprintf(data, "\nSingle data subscribed\n");
					write(client_fd, data, 1 + strlen(data));
					nread = read(client_fd, buffer, BUF_SIZE - 1);
					buffer[nread] = '\0';
							
					create_new_fork(buffer, client_fd, single_sub);
				}
			}else if(strcmp(buffer, "4")==0){
				if(array_clients[i].sub_callsreceived==1){
					sprintf(data, "\nData already subscribed!\n");
					write(client_fd, data, 1 + strlen(data));
				}else{
					single_sub=4;
					array_clients[i].sub_callsreceived=1;
					array_clients[i].single_sub=1;

					sprintf(data, "\nSingle data subscribed\n");
					write(client_fd, data, 1 + strlen(data));
					nread = read(client_fd, buffer, BUF_SIZE - 1);
					buffer[nread] = '\0';
					
					create_new_fork(buffer, client_fd, single_sub);
				}
			}else if(strcmp(buffer, "5")==0){
				if(array_clients[i].sub_smsreceived==1){
					sprintf(data, "\nData already subscribed!\n");
					write(client_fd, data, 1 + strlen(data));
				}else{
					single_sub=5;
					array_clients[i].sub_smsreceived=1;
					array_clients[i].single_sub=1;

					sprintf(data, "\nSingle data subscribed\n");
					write(client_fd, data, 1 + strlen(data));
					nread = read(client_fd, buffer, BUF_SIZE - 1);
					buffer[nread] = '\0';
					
					create_new_fork(buffer, client_fd, single_sub);
				}
			}else if(strcmp(buffer, "6")==0){
				if(array_clients[i].sub_smssent==1){
					sprintf(data, "\nData already subscribed!\n");
					write(client_fd, data, 1 + strlen(data));
				}else{
					single_sub=6;
					array_clients[i].sub_smssent=1;
					array_clients[i].single_sub=1;

					sprintf(data, "\nSingle data subscribed\n");
					write(client_fd, data, 1 + strlen(data));
					nread = read(client_fd, buffer, BUF_SIZE - 1);
					buffer[nread] = '\0';
						
					create_new_fork(buffer, client_fd, single_sub);
				}
			}		
		}else if(strcmp(buffer, "EXIT")==0){
			exit_server=1;
			while(array_clients[i].id!=id_client && array_clients[i].id!=0 && id_client!=0){
				i++;
			}
			if(array_clients[i].subscription==1 || array_clients[i].single_sub==1)
				if(pids[1]!=grandchild){
					for(int i=1; i<=n; i++)
						kill(pids[i], SIGKILL);
				}else
					kill(grandchild, SIGKILL);

		}else{
            write(client_fd,"ERROR: COMMAND NOT FOUND",1 + strlen("ERROR: COMMAND NOT FOUND"));
        }
		
		i=0;
    	fflush(stdout);
		
    }

	close(client_fd);
	exit(0);
}

void create_new_fork(char buffer[], int client_fd, int single_sub){

	char data[MAX_STRING];

	if ((grandchild=fork()) == 0){
		sprintf(data, "Created subs process with sucess!\n");
		write(client_fd, data, 1 + strlen(data));
		//printf("Grandchild’s PID: %d\n", getpid());
		//printf("Value of n: %d\n", n);
		pids[n]=getpid();
		subs_socket(atoi(buffer), single_sub);
		exit(0);
	}

	//printf("Value of the pid in array pids[%d]: %d\n", n, pids[n]);
	//printf("Grandchild: %d\n", grandchild);

	if(pids[n]!=grandchild){
		n++;
		pids[n]=grandchild;
	}
}

void update_array_clients(){

	struct json_object *jobj_array, *jobj_obj;
	struct json_object *jobj_object_id, *jobj_object_type, *jobj_object_activity, *jobj_object_location, *jobj_object_latlong, *jobj_object_callsduration, 
	*jobj_object_callsmade, *jobj_object_callsmissed, *jobj_object_callsreceived, *jobj_object_department, *jobj_object_smsreceived, *jobj_object_smssent;
	enum json_type type = 0;
	int arraylen = 0;
	int i;
	
	//Get the student data
	jobj_array = get_student_data();
	
	//Get array length
	arraylen = json_object_array_length(jobj_array);

	printf("Number of clients: %d\n", arraylen);
	
	//Example of howto retrieve the data
	for (i = 0; i < arraylen; i++) {
		//get the i-th object in jobj_array
		jobj_obj = json_object_array_get_idx(jobj_array, i);
		
		
		//get the name attribute in the i-th object
		jobj_object_id = json_object_object_get(jobj_obj, "id");
		jobj_object_type = json_object_object_get(jobj_obj, "type");
		jobj_object_activity = json_object_object_get(jobj_obj, "activity");
		jobj_object_location = json_object_object_get(jobj_obj, "location");
		jobj_object_callsduration = json_object_object_get(jobj_obj, "calls_duration");
		jobj_object_callsmade = json_object_object_get(jobj_obj, "calls_made");
		jobj_object_callsmissed = json_object_object_get(jobj_obj, "calls_missed");
		jobj_object_callsreceived= json_object_object_get(jobj_obj, "calls_received");
		jobj_object_department = json_object_object_get(jobj_obj, "department");
		jobj_object_smsreceived = json_object_object_get(jobj_obj, "sms_received");
		jobj_object_smssent = json_object_object_get(jobj_obj, "sms_sent");

		
		//FAZER TYPE E IMPLEMENTAR NO RESTO

		array_clients[i].id=i+1;
		
		if(jobj_object_callsduration!=NULL && 
		jobj_object_callsmade!=NULL && 
		jobj_object_callsmissed!=NULL && 
		jobj_object_callsreceived!=NULL && 
		jobj_object_smsreceived!=NULL && 
		jobj_object_smssent!=NULL)
		{
						
			strcpy(array_clients[i].activity, json_object_get_string(jobj_object_activity));
			strcpy(array_clients[i].location, json_object_get_string(jobj_object_location));
			array_clients[i].callsduration=atoi(json_object_get_string(jobj_object_callsduration));
			array_clients[i].callsmade=atoi(json_object_get_string(jobj_object_callsmade));
			array_clients[i].callsmissed=atoi(json_object_get_string(jobj_object_callsmissed));
			array_clients[i].callsreceived=atoi(json_object_get_string(jobj_object_callsreceived));
			strcpy(array_clients[i].department, json_object_get_string(jobj_object_department));
			array_clients[i].smsreceived=atoi(json_object_get_string(jobj_object_smsreceived));
			array_clients[i].smssent=atoi(json_object_get_string(jobj_object_smssent));
			
		}else{
			strcpy(array_clients[i].activity, "null");
			strcpy(array_clients[i].location, "null");
			array_clients[i].callsduration=0;
			array_clients[i].callsmade=0;
			array_clients[i].callsmissed=0;
			array_clients[i].callsreceived=0;
			strcpy(array_clients[i].department, "null");
			array_clients[i].smsreceived=0;
			array_clients[i].smssent=0;
		}

		/*
		//print out the name attribute
		printf("ID: %d\n", array_clients[i].id);
		printf("Ativity: %s\n", array_clients[i].activity);
		printf("Location: %s\n", array_clients[i].location);
		printf("Calls duration: %d\n", array_clients[i].callsduration);
		printf("Calls made: %d\n", array_clients[i].callsmade);
		printf("Calls missed: %d\n", array_clients[i].callsmissed);
		printf("Calls received: %d\n", array_clients[i].callsreceived);
		printf("Department: %s\n", array_clients[i].department);
		printf("Sms received: %d\n", array_clients[i].smsreceived);
		printf("Sms sent: %d\n\n", array_clients[i].smssent);
		printf("\n");
		*/
		
	}
	
}

void update_average_data(){

	double average_calls_duration=0;
	double average_calls_made=0;
	double average_calls_missed=0;
	double average_calls_received=0;
	double average_sms_received=0;
	double average_sms_sent=0;
	int number_of_clients=0;

	for(int i = 0; i < MAX_CLIENTS; ++i){
		if(array_clients[i].id!=0){

			average_calls_duration+=array_clients[i].callsduration;
			average_calls_made+=array_clients[i].callsmade;
			average_calls_missed+=array_clients[i].callsmissed;
			average_calls_received+=array_clients[i].callsreceived;
			average_sms_received+=array_clients[i].smsreceived;
			average_sms_sent+=array_clients[i].smssent;

			number_of_clients++;
		}
	}
		
	average_calls_duration/=number_of_clients;
	average_calls_made/=number_of_clients;
	average_calls_missed/=number_of_clients;
	average_calls_received/=number_of_clients;
	average_sms_received/=number_of_clients;
	average_sms_sent/=number_of_clients;

	average_data.callsduration=average_calls_duration;
	average_data.callsmade=average_calls_made;
	average_data.callsmissed=average_calls_missed;
	average_data.callsreceived=average_calls_received;
	average_data.smsreceived=average_sms_received;
	average_data.smssent=average_sms_sent;
		
}


void subs_socket(int subs_port, int single_sub){

	int fd;
  	struct sockaddr_in addr;
	struct json_object *jobj_array, *jobj_obj;
	struct json_object *jobj_object_id, *jobj_object_type, *jobj_object_activity, *jobj_object_location, *jobj_object_latlong, *jobj_object_callsduration, 
	*jobj_object_callsmade, *jobj_object_callsmissed, *jobj_object_callsreceived, *jobj_object_department, *jobj_object_smsreceived, *jobj_object_smssent;
	enum json_type type = 0;
	int arraylen = 0;
	int i;
	double sum_calls_duration, sum_calls_made, sum_calls_missed, sum_calls_received, sum_sms_sent, sum_sms_received;
  	double average_calls_duration, average_calls_made, average_calls_missed, average_calls_received, average_sms_sent, average_sms_received;
  	struct sockaddr_in subs_addr;
	int subs_addr_size, subs;
	char buffer[BUF_SIZE];
	char data[MAX_STRING];
	int nread=0;

	printf("SUBS PORT received: %d\n", subs_port);

	printf("Inside subs process!\n");

	bzero((void *) &addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); //Uso do big-endian, recebe através de qualquer das placas
	addr.sin_port = htons(subs_port);

	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) //Cria o socket
		erro("na funcao socket subs");
	if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0) //Ligas o socket
		erro("na funcao bind subs");
	if( listen(fd, 1) < 0) 
		erro("na funcao listen");

	average_calls_duration=0; printf("%s",data);
	average_calls_made=0;
	average_calls_missed=0;
	average_calls_received=0;
	average_sms_sent=0;
	average_sms_received=0;
	
    subs_addr_size = sizeof(subs_addr);
    subs = accept(fd,(struct sockaddr *)&subs_addr,(socklen_t *)&subs_addr_size);
	close(fd);
    if (subs > 0) {

	    while(1){
			printf("Update\n");
			jobj_array = get_student_data();
  			arraylen = json_object_array_length(jobj_array);

			sum_calls_duration=0;
			sum_calls_made=0;
			sum_calls_missed=0;
			sum_calls_received=0;
			sum_sms_received=0;
			sum_sms_sent=0;

	  		for (i = 0; i < arraylen; i++){
	  			jobj_obj = json_object_array_get_idx(jobj_array, i);
				jobj_object_callsduration = json_object_object_get(jobj_obj, "calls_duration");
				jobj_object_callsmade = json_object_object_get(jobj_obj, "calls_made");
				jobj_object_callsmissed = json_object_object_get(jobj_obj, "calls_missed");
				jobj_object_callsreceived= json_object_object_get(jobj_obj, "calls_received");
				jobj_object_smsreceived = json_object_object_get(jobj_obj, "sms_received");
				jobj_object_smssent = json_object_object_get(jobj_obj, "sms_sent");

				if(jobj_object_callsduration!=NULL && 
				jobj_object_callsmade!=NULL && 
				jobj_object_callsmissed!=NULL && 
				jobj_object_callsreceived!=NULL && 
				jobj_object_smsreceived!=NULL && 
				jobj_object_smssent!=NULL)
				{
					sum_calls_duration += atoi(json_object_get_string(jobj_object_callsduration));
					sum_calls_made += atoi(json_object_get_string(jobj_object_callsmade));
					sum_calls_missed += atoi(json_object_get_string(jobj_object_callsmissed));
					sum_calls_received += atoi(json_object_get_string(jobj_object_callsreceived));
					sum_sms_received += atoi(json_object_get_string(jobj_object_smsreceived));
					sum_sms_sent += atoi(json_object_get_string(jobj_object_smssent));
				}
			}
			if(average_calls_duration!=sum_calls_duration/arraylen || 
			average_calls_made!=sum_calls_made/arraylen || 
			average_calls_missed!=sum_calls_missed/arraylen || 
			average_calls_received!=sum_calls_received/arraylen || 
			average_sms_received!=sum_sms_received/arraylen || 
			average_sms_sent!=sum_sms_sent/arraylen)
			{
				average_calls_duration=sum_calls_duration/arraylen;
				average_calls_made=sum_calls_made/arraylen;
				average_calls_missed=sum_calls_missed/arraylen;
				average_calls_received=sum_calls_received/arraylen;
				average_sms_received=sum_sms_received/arraylen;
				average_sms_sent=sum_sms_sent/arraylen;
				
				if(single_sub == 0){
					sprintf(buffer,"\nAVERAGE DATA UPDATE:\nCalls duration: %lf\nCalls made: %lf\nCalls missed: %lf\nCalls received: %lf\nSms received: %lf\nSms sent: %lf\n", average_calls_duration, average_calls_made, average_calls_missed, average_calls_received, average_sms_received, average_sms_sent);
					write(subs,buffer,BUF_SIZE+1);
				}else if(single_sub == 1){
					sprintf(buffer,"\nAVERAGE DATA UPDATE:\nCalls duration: %lf\n", average_calls_duration);
					write(subs,buffer,BUF_SIZE+1);
				}else if(single_sub == 2){
					sprintf(buffer,"\nAVERAGE DATA UPDATE:\nCalls made: %lf\n", average_calls_made);
					write(subs,buffer,BUF_SIZE+1);
				}else if(single_sub == 3){
					sprintf(buffer,"\nAVERAGE DATA UPDATE:\nCalls missed: %lf\n", average_calls_missed);
					write(subs,buffer,BUF_SIZE+1);
				}else if(single_sub == 4){
					sprintf(buffer,"\nAVERAGE DATA UPDATE:\nCalls received: %lf\n", average_calls_received);
					write(subs,buffer,BUF_SIZE+1);
				}else if(single_sub == 5){
					sprintf(buffer,"\nAVERAGE DATA UPDATE:\nSms received: %lf\n", average_sms_received);
					write(subs,buffer,BUF_SIZE+1);
				}else if(single_sub == 6){
					sprintf(buffer,"\nAVERAGE DATA UPDATE:\nSms sent: %lf\n", average_sms_sent);
					write(subs,buffer,BUF_SIZE+1);
				}

				nread = read(subs, data, MAX_STRING - 1);   
				data[nread] = '\0';
				printf("Message received: %s\n", data);
			}
			sleep(3);
		}
    	close(subs);
		exit(0);
    }
}

void erro(char *msg){
	printf("Error: %s\n", msg);
	exit(-1);
}