											/*ftpServer.c*/
/*Server file which runs on the provided port.Sever listens for the persistent control connections from the clients and create
new process to service the client and continues to listen for other connections. It also opens non-persistent FTP 
data connections for sharing the files.*/

/*****************************************************************************************************************/
#include "ftpServer.h"


/*This function sends the requested file over the newly created data connection*/
void serverSendFile(int dataSocket,char *path){
	int j=0;
	char buffer[MAXLEN];

	
	fp = fopen(path,"r");

	if(fp==NULL){
		printf("File is not found\n");
		return ;
	}

	char str[MAXLEN];
	bzero(str,MAXLEN);
	bzero(buffer,MAXLEN);
	
	/*read the content of the file into the buffer*/
	while((str[0]=fgetc(fp)) != EOF){
		strcat(buffer,str);	
	}

	
	printf("File sending started....\n");
	
	/*send the content of the file through the data socket*/
	int count = write(dataSocket, buffer, sizeof(buffer));	
	printf("File has been sent successfully...\n");

	fclose(fp);
	return ;
}

/*This function receives the requested file over the newly created data connection*/
int recieveFile(int dataSocket,char *path){
	char buff[MAXLEN];
	bzero(buff,MAXLEN);
	printf("File receiving started...\n");

	
	/*receive the content of the file into the buffer */
	int c = read(dataSocket,buff,sizeof(buff));

	/*write the content into the file*/
	fp = fopen(path,"w");
	fprintf(fp, "%s\n",buff);
	printf("File has been recieved successfully...\n");
	
	fclose(fp);
	return c;
}


/*This function opens a data connection and send or recieve the file according to the command received*/
void dataTransfer(SA clientAddr,unsigned long int port,char* path,int mode){
	SA clientAddr2;
	
	/*create a new data socket*/
	int dataSocket= socket(AF_INET,SOCK_STREAM,0);
	if(dataSocket==-1){
		printf("Data Socket cannot be opened\n");
		return ;
	}

 	bzero(&clientAddr2, sizeof(clientAddr2));

	clientAddr2.sin_family = AF_INET;
	clientAddr2.sin_addr.s_addr = clientAddr.sin_addr.s_addr;
	clientAddr2.sin_port = port;

	/*send client to open a ftp data connection over the given listening port*/
	int connectStatus = connect(dataSocket,(struct sockaddr*) &clientAddr2,sizeof(clientAddr2));

	if(connectStatus<0){
		printf("Connection Error\n");
		return ;
	}
	
	/*mode=1 means PUT command is given and mode=0 means GET command is given*/
	if(mode==1) recieveFile(dataSocket,path);
	else serverSendFile(dataSocket,path);

	/*As data connections as non-persistent hence close the connection after sending*/
	close(dataSocket);
}

/*This function gets the path of the file in server and call function to create 
data connection*/
void getFile(int controlSocket, SA clientAddr, char* fileName){
	int i;
	char path[100]="homeDir/";

	char buff[MAXLEN];
	bzero(buff,MAXLEN);

	int len = strlen(path);

	/*get the path of the file*/
	for (i = 0; i < strlen(fileName); ++i)
	{
		path[i+len]=fileName[i];
	}

	/*check if requested file is present or not*/
	fp = fopen(path,"r");

	if(fp==NULL){
		printf("File %s not found\n",fileName);
		char replyBuff[MAXLEN]="0";

		/*inform the client that requested file is not found on server*/
		int c = write(controlSocket,replyBuff,sizeof(replyBuff));
	}
	else{
		char replyBuff[MAXLEN]="1";

		/*inform the client that requested file is found and now sharing will began*/
		int c = write(controlSocket,replyBuff,sizeof(replyBuff));
		char buff[MAXLEN]="\0";
		bzero(buff,MAXLEN);

		/*read the listening port number on which client is going to listen
		  for data connection request*/
		c = read(controlSocket,buff,sizeof(buff));
		unsigned long int port = atoi(buff);
		dataTransfer(clientAddr,port,path,0);
	}
}

/*This function reads the command from the control socket and service according 
  to the command*/
int recieveCommand(int controlSocket,SA clientAddr){
	int i=0,j,dataSocket;
	int k=0,m=0;
	char command[5]="\0",fileName[20]="\0",path[100]="homeDir/";
	time_t t;
 	srand((unsigned)time(&t));

	char buff[MAXLEN];
	bzero(buff,MAXLEN);

	/*read the command from the control socket*/
	int c = read(controlSocket,buff,sizeof(buff));
	
	/*extract command and requested file name from the message*/
	while(buff[i]!=':'){
		command[m++]=buff[i];
		i++;
	}
	
	for(j=i+1;buff[j]!='#';j++){
		fileName[k++]=buff[j];
	}

	int len = strlen(path);

	/*get the path of the requested file*/
	for (i = 0; i < strlen(fileName); ++i)
	{
		path[i+len]=fileName[i];
	}

	fp = fopen(path,"r");

	/*service according to the command*/
	if(strcmp(command,"PUT")==0){
		if(fp==NULL){
			char dataBuff[MAXLEN] = "1";
			
			/*send 1 if file is not found on the server that is sender can send 
			  the file no need to ask for overwrite permission */								 
			int c = write(controlSocket,dataBuff,sizeof(dataBuff));
			bzero(buff,MAXLEN);
			
			/*read the listening port number on which client is going to listen
			  for data connection request*/
			c = read(controlSocket,buff,sizeof(buff)); 
			unsigned long int port = atoi(buff);

			/*call function to recieve data*/
			dataTransfer(clientAddr,port,path,1);
		}
		else{
			fclose(fp);
			char dataBuff[MAXLEN] = "0";

			/*send 0 if the file is already present on the server to ask client for
			  overwrite permission*/
			int c = write(controlSocket,dataBuff,sizeof(dataBuff));
			bzero(buff,MAXLEN);

			/*read if the client wants to overwrite on the current existing file*/
			c = read(controlSocket,buff,sizeof(buff));
			
			/*buff[0]=1 means client has requested to overwrite on current existing*/
			if(buff[0]=='1'){
				bzero(buff,MAXLEN);

				/*read the listening port number on which client is going to listen
			  	  for data connection request*/
				c = read(controlSocket,buff,sizeof(buff));
				unsigned long int port = atoi(buff);

				/*call function to recieve data*/
				dataTransfer(clientAddr,port,path,1);
			}

		}

	}
	else if(strcmp(command,"GET")==0){
		/*call to send the requested file*/
		getFile(controlSocket, clientAddr, fileName);	
	}
	else if(strcmp(command,"MGET")==0){
		/*open the directory to send all the files of the requested extension*/
		DIR *d;
		struct dirent *dir;
		d = opendir("homeDir/");
		if(!d){
			printf("Directory cannot be opened\n");
		}
		else{
			/*read the files from the directory*/
			while ((dir = readdir(d)) != NULL){
				char *file = dir->d_name;
				if(file[strlen(file)-1]==fileName[strlen(fileName)-1]){
					char replyBuff[MAXLEN]="1#";
					strcat(replyBuff,file);
					strcat(replyBuff,"#");

					/*send the filename of the file into the control socket*/
					int c = write(controlSocket,replyBuff,sizeof(replyBuff));
					
					bzero(replyBuff,sizeof(replyBuff));

					/*get the reply from the client to send the file*/
					c = read(controlSocket,replyBuff,sizeof(replyBuff));

					/*send the file content*/
					getFile(controlSocket,clientAddr,file);
					bzero(buff,MAXLEN);

					/*read the reply from the client through control socket if it is ready to recieve next file*/
					c = read(controlSocket,buff,sizeof(buff));
				}

			}
			char replyBuff[MAXLEN]="0#";

			/*inform the client that server has finished sending all the files of requested extension*/
			int c = write(controlSocket,replyBuff,sizeof(replyBuff));
 
    		closedir(d);

		}
	}
	return c;
}

/*This function service the client*/
void serviceClient(int controlSocket,SA clientAddr){
	
	/*get the commands from the server*/
	while(recieveCommand(controlSocket,clientAddr)!=0){
		printf("The File transfer is completed successfully\n");
	}

}


/*argv contains the port on which server is going to run*/
int main(int argc,char* argv[]){
	
	/*variables to store server and client information ,i.e. IP address,port etc.*/
	SA serverAddr,clientAddr;
	
	/*listen port listens the ftp connection requests from different clients*/
	int listenPort;	
	
	/*pid stores the pid's of the processes created for different clients*/
	pid_t pid;
	char buff[MAXLEN];	

    /*variables to store the IP address of the machine on which server is running*/
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    getifaddrs(&ifAddrStruct);

    /*create a socket for listening*/
	listenPort = socket(AF_INET,SOCK_STREAM,0);
	if(listenPort==-1) {
		printf("Error opening the listen port\n");
		return 0;
	}

	char ip[20];

	/*get the IP*/
	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) {
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if(strncmp(ifa->ifa_name,"wlp",3)==0){
            	strcpy(ip,addressBuffer);
            }
        }
    }
   
    /*translate the IP address into network byte order*/
    unsigned long int  ipAddr = inet_addr(ip);

	bzero(&serverAddr, sizeof(serverAddr));
	

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ipAddr;
	serverAddr.sin_port = htons(atoi(argv[1]));

	/*bind the socket with server address*/
	int bindStatus = bind(listenPort,(struct sockaddr*) &serverAddr,sizeof(serverAddr));
	
	if(bindStatus==-1){
		printf("Bind Error\n");
		return 0;
	}

	printf("Server started on IP %s at port %s\n",ip,argv[1]);
	
	/*listen for incoming tcp connections*/
	int listenStatus = listen(listenPort,LISTENQ);
	if(listenStatus==-1){
		printf("Listen Error\n");
		return 0;
	}

	while(1){
		int len = sizeof(clientAddr);

		/*create a control tcp connection of the ftp*/
		int acceptSocket = accept(listenPort,(struct sockaddr*) &clientAddr,&len);
		printf("connection is from %s, port %d\n",inet_ntop(AF_INET, &clientAddr.sin_addr, buff, sizeof(buff)),ntohs(clientAddr.sin_port));
		
		/*fork to create a child process to serve the client*/
		if ((pid = fork()) < 0) printf("Child cannot be created\n");
		else if (pid == 0) {
			/*close the listen port for the child process*/
			close(listenPort);
			serviceClient(acceptSocket,clientAddr);
			printf("connection from %s, port %d has been disconnected\n",inet_ntop(AF_INET, &clientAddr.sin_addr, buff, sizeof(buff)),ntohs(clientAddr.sin_port));
			
			/*close the control connection when the client leaves*/
			close(acceptSocket);
			exit(0);
		}
		/*close the control socket for server process*/
		close(acceptSocket);
		/*increase the number of child processes to keep count of how many client are currently active*/
		childProcCount++;

		/*kill all the zombie processes running*/
		while(childProcCount){

			/*get if any zombie process is running and reap it when found*/
			pid = waitpid((pid_t) -1, NULL, WNOHANG);
			if (pid < 0) printf("Error\n");
			else if (pid == 0) break;
			else childProcCount--;
		}
	}
	return 0;

}
