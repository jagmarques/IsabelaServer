#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <signal.h> 
#include <netdb.h> 
#include <string.h> 
//ISABELLA
#include <stdio.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include <string.h>

#define SERVER_PORT 9000
#define BUF_SIZE 1024

#define MAX_STRING 1000
#define MAX_CLIENTS 33

pid_t grandchild;
int pids[1000];
int n=0;

//Struct for each client
typedef struct client{
    int id;
    char activity[50];
    char location[50];
    int callsduration;
    int callsmade;
    int callsmissed;
    int callsreceived;
    char department[50];
    int smsreceived;
    int smssent;
	int subscription;
	int single_sub;
	int sub_callsduration;
    int sub_callsmade;
    int sub_callsmissed;
    int sub_callsreceived;
    int sub_smsreceived;
    int sub_smssent;
}Client;

typedef struct average{
    double callsduration;
    double callsmade;
    double callsmissed;
    double callsreceived;
    double smsreceived;
    double smssent;
}Average;

Client single_client;
Client *array_clients;
Average average_data;

//ISABELLA

/*
This example project use the Json-C library to decode the objects to C char arrays and 
use the C libcurl library to request the data to the API.
*/


struct string {
	char *ptr;
	size_t len;
};

//Write function to write the payload response in the string structure
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr + s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

//Initilize the payload string
void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

//Get the Data from the API and return a JSON Object
struct json_object *get_student_data(){
	struct string s;
	struct json_object *jobj;

	//Intialize the CURL request
	CURL *hnd = curl_easy_init();

	//Initilize the char array (string)
	init_string(&s);

	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
	//To run on department network uncomment this request and comment the other
	//curl_easy_setopt(hnd, CURLOPT_URL, "http://10.3.4.75:9014/v2/entities?options=keyValues&type=student&attrs=activity,calls_duration,calls_made,calls_missed,calls_received,department,location,sms_received,sms_sent&limit=1000");
    //To run from outside
	curl_easy_setopt(hnd, CURLOPT_URL, "http://socialiteorion2.dei.uc.pt:9014/v2/entities?options=keyValues&type=student&attrs=activity,calls_duration,calls_made,calls_missed,calls_received,department,location,sms_received,sms_sent&limit=1000");

	//Add headers
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "cache-control: no-cache");
	headers = curl_slist_append(headers, "fiware-servicepath: /");
	headers = curl_slist_append(headers, "fiware-service: socialite");

	//Set some options
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writefunc); //Give the write function here
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &s); //Give the char array address here

	//Perform the request
	CURLcode ret = curl_easy_perform(hnd);
	if (ret != CURLE_OK){
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));

		/*jobj will return empty object*/
		jobj = json_tokener_parse(s.ptr);

		/* always cleanup */
		curl_easy_cleanup(hnd);
		return jobj;

	}
	else if (CURLE_OK == ret) {
		jobj = json_tokener_parse(s.ptr);
		free(s.ptr);

		/* always cleanup */
		curl_easy_cleanup(hnd);
		return jobj;
	}

}