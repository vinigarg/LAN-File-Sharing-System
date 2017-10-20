#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <vector>
#define BACKLOG 100
#define handle_error(msg) \
	do{ perror(msg); exit(EXIT_FAILURE);}while(0);


int sockfd, asock_fd, portno,Rfd,Lfd;
struct sockaddr_in serv_addr, cli_addr;
using namespace std;
  
void createRepo();
void createLog();
void loadmap();
void createlog(string msg,string ip_addr);
void search(string);
void share(char buffer[],int size,string);
void service(string ip_addr);

/***********************************MAIN FUNCTION***************************************************/
int main( int argc, char *argv[] ) {

	// input : ./server <port_number>

   if(argc<2){
	handle_error("Invalid input");
   }

   char buffer[256];
   int n, pid;
   
   /*
    create socket
    int socket(int domain, int type, int protocol);
    AF_INET     :IPv4 Internet protocols
    SOCK_STREAM :Provides sequenced, reliable, two-way, connection-based
                byte  streams.  An out-of-band data transmission mecha‐
                nism may be supported.

	*/
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      handle_error("ERROR opening socket");
      exit(1);
   }
   
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = atoi(argv[1]);
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   
   // bind	
   // assigning a name to a socket with portnumber = argv[1]
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      handle_error("ERROR on binding");
      exit(1);
   }
      
   /*
    listen() marks the socket referred to by sockfd as a passive socket.
    That is, as a socket that will be used to accept incoming connection req.
    The  backlog  argument  defines the maximum length to which the queue 
    of pending connections for sockfd may grow.
    Max size permitted by most system is 5
   */
   listen(sockfd,5);
   cout<<"Waiting for client."<<endl;
      
   socklen_t clilen = sizeof(cli_addr);
   createRepo();
   createLog();
   while (1) {

	/*
	It extracts the first connection request on the queue of pending 
	connections for the listening socket, sockfd, creates a new connected socket, 
	and returns a new file descriptor referring to that socket.  
	The newly created socket is  not  in  the listening state.  
	The original socket sockfd is unaffected by this call.
	*/

      asock_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      string ip_addr = inet_ntoa(cli_addr.sin_addr);
      if (asock_fd < 0) {
         handle_error("ERROR on accept");
         exit(1);
      }

      //create child process      
      pid = fork();
		
      if (pid < 0) {
         handle_error("ERROR on fork");
         exit(1);
      }
      
      if (pid == 0) {
      	// child process handling service on asock_fd
      	// closing sockfd as its not required to handle the service
         close(sockfd);
         service(ip_addr);
         exit(0);
      }
      else {
      	// after closing the accepted file descriptor we continue accepting new connections
         close(asock_fd);
      }
		
   }
}


/********************************************DO PROCESSING**********************************************/
void service(string ip_addr){
	char buffer[256];
	int content_read;
	buffer[0]='\0';	

	while((content_read = read(asock_fd , &buffer , 255 ))>0){
		if(buffer[content_read-1] == '#') {
			buffer[content_read-1] = '\0';
			createlog("Search request from ",ip_addr);
			string buf(buffer);
			search(buf);
			createlog("Search response send to ",ip_addr);
		}
		else if(buffer[content_read-1] == ';'){
			buffer[content_read-1] = '\0';
			 createlog("Share request from ",ip_addr);
			 share(buffer,content_read-1,ip_addr);
			 createlog("Share ack sent to ",ip_addr);
		}		
	}
}

/******************************************************CREATE REPO FUNCTION***************************************************/

void createRepo(){
        Rfd = open("Repo.txt",O_CREAT | O_RDWR | O_APPEND, 0777);
}

/******************************************************CREATE Log FUNCTION***************************************************/

void createLog(){
        Lfd = open("Log.txt",O_CREAT | O_RDWR | O_APPEND, 0777);
}

/*********************************************************CREATE LOG********************************************************/
void createlog(string msg,string ip_addr){
	time_t now = time(0);
	string time, date;
	tm *ltm = localtime(&now);

   // print various components of tm structure.
	int year =  1900 + ltm->tm_year;
	int mnth =  1 + ltm->tm_mon;
	int d =  ltm->tm_mday;
	int hr =  1 + ltm->tm_hour;
	int min = 1 + ltm->tm_min;
	int sec =  1 + ltm->tm_sec;

	string temp;
	temp = to_string(year);
	date = temp;
 	temp = to_string(mnth);
	if(mnth<10) temp = "0" +temp;
	date = "-"+temp+"-"+date;
	temp = to_string(d);
	if(d<10) temp = "0" +temp;
	date = temp + date;
	
	temp = to_string(sec);
	time = temp;
	if(sec<10) time = "0" + time;

 	temp = to_string(min);
	if(min<10) temp = "0" +temp;
	time = "-"+temp+"-"+time;

	temp = to_string(hr);
	if(d<10) temp = "0" +temp;
	time = temp + time;
	
	string timestamp = date + "  " + time + " : " + msg + " " + ip_addr + "\n";

	//write in log file
	const void * ts = timestamp.c_str();
	int Lfd = open("Log.txt",O_RDWR | O_APPEND | O_CREAT,0777);
	int wr_fd = write(Lfd,ts,timestamp.length());
	if(wr_fd <0){
		handle_error("Create Log function not editing");
	}
	close(Lfd);
}

/*******************************************************SEARCH FUNCTION*******************************************************/

void search(string key){			

	map<string , vector<pair<string,string> > > record;
	string filen;
	string path;
	string ip;
	int byte_read,at=0;
	char c;
	int Rfd = open("Repo.txt",O_RDONLY,0777);
	while((byte_read = read(Rfd,&c,1))>0){
		if(c=='\n') {
			//insert record in map
			record[filen].push_back(make_pair(path,ip));
			//initiallize file,path and ip for next record
			filen="";path="";ip="";
			at=0;
		}
		else if(c!='@' && at==0) filen  = filen +c;
		else if(c=='@') at++;
		else if(c!='@' && at==1) path = path +c;
		else if(c!='@' && at==2) ip = ip+c;		
	}	
	close(Rfd);
	
	//search
	map<string , vector<pair<string,string> > > :: iterator it;
	it = record.find(key);
	string const &file = it -> first;
	vector<pair<string,string> > &v = it->second;
	string mirror="";
	if(v.size() > 0){
		int i=0;
		for(i=0;i<v.size();i++){
			string num = to_string(i+1);
			mirror = mirror +" "+ num +"  "+ v[i].first+key+"  "+v[i].second+"\n";
		}	
	}
	const void* t = mirror.c_str();
	
	int wr_fd = write(asock_fd,t,mirror.length());
	if(wr_fd <0){
		handle_error("Mirror not sent to client");
	}
}


/**********************************************************UPDATE REPO FUNCTION*****************************************************/
int updateRepo(char file[] , char buffer[], string ip_addr){
	string f = string(file);
	string buf = string(buffer);
	f = f + "@";
	f = f + buf;
	f = f + "@";
	f = f + ip_addr+"\n";

	const void * temp = f.c_str();

	int Rfd = open("Repo.txt",O_RDWR | O_APPEND | O_CREAT,0777);
	int wr_fd = write(Rfd,temp,f.length());
	if(wr_fd <0){
		handle_error("Update Repo function not editing");
	}
	close(Rfd);
	return wr_fd;
}

/**********************************************************SHARE FUNCTION**************************************************************/

void share(char buffer[],int size,string ip_addr){
	//get name and path of file
	char file[20];
	file[0]='\0';
	int i,j;

	for(i=size-1;i>=0;i--){
		if(buffer[i] == '/') break;
	}
	int temp=i+1;
	for(j=0;j<20 && i<size;j++){
		file[j]=buffer[++i];
	}
	file[j]='\0';
	buffer[temp]='\0';

	//file write
	int wr_fd =  updateRepo(file,buffer,ip_addr);

	//ack that file is shared successfully
	string f = "Successful sharing done";
	const void* t = f.c_str();
	if(wr_fd > 0) write(asock_fd,t,f.length());
	return ;
}
