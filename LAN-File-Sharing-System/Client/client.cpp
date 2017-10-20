#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <map>
#include <vector>
#define BACKLOG 10
#define DOWN_PORT 30001
#define handle_error(msg) \
	do{ perror(msg); exit(EXIT_FAILURE);}while(0);

using namespace std;

int service(int arg, char * argv[],int);
void download(string ip_addr,int asock_fd);
void downloadServer();
void clienting(string path,string ip);
void createLog(string,string);

int main(int argc, char *argv[]) {
int pid = fork();
   if(pid<0){
	handle_error("Client not forked");
	return 0;
   }   
   else if(pid==0){
	   downloadServer();
	
   }else{
	   int sock_fd, portno, n;
	   struct sockaddr_in serv_addr;
	   struct hostent *server;
	
	   if (argc < 3) {
		handle_error("Invalid format of input");      
		exit(0);
	   }
	
	   portno = atoi(argv[2]);
   
	   sock_fd = socket(AF_INET, SOCK_STREAM, 0);
   
	   if (sock_fd < 0) {
	      handle_error("Error in opening socket at client");	
	      exit(1);
	   }
		
	   server = gethostbyname(argv[1]);
   
	   if (server == NULL) {
	      handle_error("Error, no such host\n");
	      exit(0);
	   }
	   
	   bzero((char *) &serv_addr, sizeof(serv_addr));
	   serv_addr.sin_family = AF_INET;
	   bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	   serv_addr.sin_port = htons(portno);
   
	   if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {	
	      handle_error("Error in connecting,  at client");
	      exit(1);
	   }
   	
	   service(argc,argv,sock_fd);
  } 

   return 0;
}

/************************************DOWNLOAD SERVER*********************************************/
void downloadServer(){
  int sockfd, asock_fd, portno,Lfd;
  struct sockaddr_in serv_addr, cli_addr;
   int n, pid;
   
   //create socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      perror("Cannot open socket at download server");
      exit(1);
   }
   
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = DOWN_PORT;
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   
   // bind
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("Error on binding at download server");
      exit(1);
   }
      
   listen(sockfd,5);

   socklen_t clilen = sizeof(cli_addr);
   Lfd = open("DownloadServerLog.txt",O_CREAT | O_RDWR | O_APPEND, 0777);

   while (1) {
      asock_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      string ip_addr = inet_ntoa(cli_addr.sin_addr);
	
      if (asock_fd < 0) {
         handle_error("Error on accept at download server");
         exit(1);
      }

      //create child process      
      pid = fork();
		
      if (pid < 0) {
         handle_error("Error on fork at download server");
         exit(1);
      }
      
      if (pid == 0) {
         close(sockfd);
         download(ip_addr,asock_fd);
         exit(0);
      }
      else {
         close(asock_fd);
      }
		
   }
}

/******************************************CREATE LOG FILE*****************************************/
void createLog(string msg,string ip_addr){
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
	int Lfd = open("DownloadServerLog.txt",O_RDWR | O_APPEND,0777);
	int wr_fd = write(Lfd,ts,timestamp.length());
	if(wr_fd <0){
		handle_error("Error in writing Log at download server");
	}
	close(Lfd);
}

/*****************************************DOWNLOAD*********************************************/
void download(string ip_addr,int asock_fd){

	int read_cont=0;
	char buffer[256];
	buffer[0]='\0';
	int rd;
	//reading fle path
	read_cont = read(asock_fd,&buffer,sizeof(buffer));
	if(read_cont == 0){
		handle_error("no file path has been read");
		return ;
	}else{
		createLog("Download Request from ",ip_addr);
		//string file_path = string(buffer);
		int fd = open(buffer,O_RDWR,0777);
		//READ FILE
		char buf[1];
		if(fd>=0){
			while((rd = read(fd,&buf,1))>0){
				write(asock_fd,buf,1);
			}
			createLog("File sent to ",ip_addr);
		}
		else{
			createLog("Error message sent to ",ip_addr);
			return;
		}
		close(fd);
	}		
}

/******************************CLIENT ACTING AS A CLIENT**********************************************/
void clienting(string name,string path,string ip){
int sock_fd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;

   portno = DOWN_PORT;
   
   sock_fd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sock_fd < 0) {
      handle_error("Error opening socket");
      exit(1);
   }
  
   server = gethostbyname(ip.c_str());
   
   if (server == NULL) {
      handle_error("Error,download server down\n");
      exit(0);
   }
   
   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
   serv_addr.sin_port = htons(portno);
   
   if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      handle_error("Error connecting");
      exit(1);
   }
  

   //write file path to server
   int write_fd = write(sock_fd,path.c_str(),path.length());
   if(write_fd < 0){
	handle_error("Request not sent to Download Server");
   }

   char buffer[1];
   buffer[0]='\0';
      

   int reading;
  // int rd = read(sock_fd , &buffer , sizeof(buffer));cout<<rd<<endl;

   int flag=0;
   int fd = open(name.c_str(),O_CREAT | O_APPEND | O_RDWR,0777);
   while((reading = read(sock_fd , &buffer , sizeof(buffer)))>0){flag=1;	write(fd,buffer,1);}

   if(flag==1) cout<<"File Transfer complete."<<endl;
   else cout<<"File path invalid"<<endl;

   //delete file opened
   close(fd);
   close(sock_fd);
   
}

/******************************CLIENT FUNCTION***************************************************/
int service(int arg, char * argv[],int sock_fd){

	int option,write_fd,read_fd;
	char buffer[512];
	buffer[0]='\0';
	option=1;
	while(1){
		//cout<<endl;
		cout<<"Select : "<<endl;
		cout<<"1. Share"<<endl;
		cout<<"2. Search"<<endl;
		cout<<"3. Quit"<<endl;
		cout<<">> ";
		scanf("%d",&option);
		switch(option){
			case 3 :
			{
				close(sock_fd);
				return 0;
			}
			case 2 :
			{
				 cout<<"\nType file name you want to search :\t";
				 scanf("%s",buffer);
				 string name = string(buffer);
                 cout<<endl;
				 strcat(buffer,"#");

				 //REQUEST SENT
	             write_fd = write(sock_fd,buffer,strlen(buffer));
				 if(write_fd < 0){
					handle_error("File not shared..");
			     }

				 //RESPONSE MIRROR
				 read_fd = read(sock_fd,&buffer,sizeof(buffer));
				 if(read_fd < 0){
					handle_error("No Acknowledgement of Sharing service");
				 }else  if(read_fd == 0){
					cout<<"No mirror found\n"<<endl;
				 }else {
					buffer[read_fd] = '\0';
					cout<<"Select the mirror you want :"<<endl;				 
					write(0,buffer,strlen(buffer));
					string mirror;
					cout<<">>";
					cin>>mirror;
					string path = "",ip = "";

					//tokenize mirror buffer
					char * token = strtok (buffer," ");
					while(token!=NULL){
						//string str(token);
						if(string(token).compare(mirror)==0){
							path = strtok(NULL," ");
							ip = strtok(NULL," ");
							int len = ip.length();
							ip = ip.substr(0,len-1);
							break;
						}else{
							token = strtok(NULL," ");
						}
					}					
					if(ip.compare("")==0) cout<<"No such mirror listed\n"<<endl;					
					else clienting(name,path,ip);			
		 	        }
			 break;
			}

			case 1:
			 {	
				cout<<"Path :\t";
				scanf("%s",buffer);
				strcat(buffer,";");
	            write_fd = write(sock_fd,buffer,strlen(buffer));
				if(write_fd < 0){
					handle_error("File name not sent..");
				}
				read_fd = read(sock_fd,&buffer,sizeof(buffer));
				if(read_fd < 0){
					handle_error("No Acknowledgement of Sharing service");
					return 0;
				}
				buffer[read_fd]='\0';
				write(0,buffer,strlen(buffer));
				cout<<endl;
				break;
			}
			default : cout<<"Invalid input";
		}
	}
return 0;
}


