/*Ahmet Emin Kaplan 131044042 Sistem programlama final projesi*/
/*client.c*/

#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<fcntl.h>
#include<signal.h>
#include<sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
	
	int id;		
	int serverSocket;
	char filename[255];
}Package;

int flag;
int flag2;
int callSocket(char *hostname, unsigned short portnum);
void help();
void listLocal();
void listServer(int serverSocket);
void requestForClients(int serverSocket);
void* sendFile(void* param);
void* receive(void* param);

int doneflag=0;

pthread_t receiver;
pthread_t sender;

int Rcontrol=0;
int Scontrol=0;

int ss;
int myid;

static void SignalHandler(int signo) {

	doneflag=1;	
	int die=-1;
	int five=5;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	
	fprintf(stderr,"CTRL+C basildi send ve receive islemlerinin bitmesi bekleniyor!\n");

	if(Rcontrol==1)
	{
			
		
		pthread_join(receiver,NULL);


	
	}
	else{

		pthread_mutex_lock(&mutex);

		write(ss,&five,sizeof(int));
		
		pthread_mutex_unlock(&mutex);
		//perror("receiver bekleniyor 01");		
		
		pthread_join(receiver,NULL);


	}

}


int main(int argc,char *argv[]){

	int serverSocket;
	int number,i;
	struct stat file_stat;
	char hostName[HOST_NAME_MAX];
	char buffer;
	int fd;
	int cid;

	if(argc!=3)
	{
		fprintf(stderr,"Usage:<server ip adress> <portNumber>\n");
		fprintf(stderr,"Sample Usage:192.168.28.12  8080\n");
		exit(1);
	
	}

	//gethostname(hostName,HOST_NAME_MAX);
	flag=1;

	struct sigaction act;
	act.sa_handler = SignalHandler;
	act.sa_flags = 0;
	Package sendPackage;

	if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1)) 
	{
		perror("Failed to set CTRL+C handler");
		exit(1);
	}


	serverSocket=callSocket(argv[1],atoi(argv[2]));
	ss=serverSocket;
	if(ss==-1)
	{
		fprintf(stderr,"Baglantı kurulamadi!\n");
		exit(1);	
	}
	
	read(serverSocket,&myid,sizeof(int));
		fprintf(stderr,"CLIENT NUMBER:%d\n",myid);


	pthread_create(&receiver,NULL,receive,&serverSocket);

	while(!doneflag)
	{

		//sleep(1);
		fprintf(stderr,"Komut bekleniyor\n");
		
		char command[100];
		char filename[100];
		int cl=-1;
		int m=0;
		char *ffilename;
		char *sci;
		char *save;

		strcpy(command,"no");		
		//scanf("%s",command);
       
		fgets(command,100,stdin);
		save=command;
		strtok_r(command, " ", &save);

		ffilename=strtok_r(NULL, " ", &save);

		sci=strtok_r(NULL, " ", &save);

	//	fprintf(stderr,"%s--%s--%s--\n",command,ffilename,sci);
		if(strcmp(command,"no")!=0)
		fprintf(stderr,"komut:%s\n",command);
		
		//exit(1);
			
		if(strcmp(command,"listLocal\n")==0)
			listLocal();
		else if(strcmp(command,"lsClients\n")==0)
		{
			Scontrol=1;
			//perror("deneme");	
			pthread_mutex_lock(&mutex);
			 requestForClients(serverSocket);
			pthread_mutex_unlock(&mutex);	
		//	perror("deneme2");
			Scontrol=0;			
		}
		else if(strcmp(command,"listServer\n")==0)
		{
			Scontrol=1;
			pthread_mutex_lock(&mutex);
			listServer(serverSocket);
			pthread_mutex_unlock(&mutex);
			Scontrol=0;
		}
		else if(strcmp(command,"help\n")==0){
			help();
		}
		else if(strcmp(command,"sendFile")==0)
		{

			sprintf(filename,"%s",ffilename);
			if(sci!=NULL){
				cl=atoi(sci);
							
			}
			else{
					
				filename[strlen(filename)-1]='\0';		
				cl=-999;
			
			}	
			if(cl==myid)
			{
				fprintf(stderr,"Kendine dosya yollayamazsin!\n");
			}
			else{

			if(cl!=-999)
			fprintf(stderr,"Girilen file:%s id:%d\n",filename,cl );
			
			else
			fprintf(stderr,"Girilen file:%s id:Server\n",filename);
			
		//	sendFile(filename,cl,serverSocket);
			sendPackage.id=cl;
			strcpy(sendPackage.filename,filename);
			sendPackage.serverSocket=serverSocket;
			Scontrol=1;
			pthread_create(&sender,NULL,sendFile,&sendPackage);

			pthread_join(sender,NULL);
			Scontrol=0;

			}
		}/*
		else if(strcmp("no",command)==0)
			break;*/
		else if(doneflag==1)
			fprintf(stderr,"Program kapandi!\n");
		else
			fprintf(stderr,"Hatali giris\n\n");
	}
	
	
}

void requestForClients(int serverSocket)
{
	int i=1;
	write(serverSocket,&i,sizeof(int));
	fprintf(stderr,"lsClients sonucu bekleniyor!\n");
	flag=1;
	while(flag!=0);
}
void listServer(int serverSocket)
{
	int i=2;
	write(serverSocket,&i,sizeof(int));
	fprintf(stderr,"listServer sonucu bekleniyor!\n");
	flag2=1;
	while(flag2!=0);
}

int callSocket(char *hostname, unsigned short portnum)
{
	struct sockaddr_in sa;
	
	struct hostent *hp;
	
	int a, s;
	
	if ((hp= gethostbyname(hostname)) == NULL)
	{

		return(-1);
	}
	
	memset(&sa,0,sizeof(sa));



	sa.sin_family= AF_INET;

	sa.sin_port= htons((u_short)portnum);

	inet_pton(AF_INET, hostname, &sa.sin_addr.s_addr);


	
	if ((s= socket(AF_INET,SOCK_STREAM,0)) < 0)
		return(-1);

	if (connect(s,(struct sockaddr *)&sa,sizeof sa) < 0)
	{
	 /* connect */
		close(s);
		return(-1);
	} /* if connect */

	return(s);
}

void* receive(void* param)
{
	
	int serverSocket=*(int*)param;	
	struct stat file_stat;
	int fd;
	int i=0;

 	char buffer='x';
	char filename[255];
	char path[255];
	
	getcwd(path,255);
	int junk=0;
	char temp;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	while(!doneflag)
	{
		junk=0;
		buffer='x';
		filename[0]='\0';
		i=0;
		Rcontrol=0;
		read(serverSocket,&temp,sizeof(char));
		Rcontrol=1;		

		if(temp=='f')
		{					
			while(buffer!='\0')
			{
				read(serverSocket,&buffer,sizeof(char));
				filename[i]=buffer;
				//fprintf(stderr,"%c",buffer);
				++i;
			}
		
			fprintf(stderr,"filename:%s\n",filename);
			strcat(path,"/");
			strcat(path,filename);

			fprintf(stderr,"%s\n\n\n",path);

			fd=open(filename, O_CREAT | O_WRONLY | O_TRUNC,0777);

			if (fd == -1)
    		{
				perror("s");
          	//  fprintf(stderr, "Error opening file --> %s", strerror(errno));
            	exit(EXIT_FAILURE);
    		}

			read(serverSocket,&file_stat, sizeof(file_stat));


	
			for(i=0;i<file_stat.st_size;++i)
			{		
			//fprintf(stderr,".");
				read(serverSocket,&buffer,1);
			write(fd,&buffer,1);	
			}

			close(fd);
			fprintf(stderr,"Dosya alindi!\n");
		}
	
		else if(temp=='l')
		{
			fprintf(stderr,"\n\nCurrent Clients:\n");
			while(junk!=-1)
			{
				read(serverSocket,&junk,sizeof(int));
				if(junk!=-1){				

				if(junk==myid)
					fprintf(stderr,"Client %d  <----You\n",junk);
				else				
					fprintf(stderr,"Client %d\n",junk);
							

				}			
			}
			fprintf(stderr,"\n\n");
			flag=0;
					
		}
	
		else if(temp=='s')
		{
			buffer='x';
			fprintf(stderr,"\n\nListServer:\n");
			while(buffer!='\0')
			{

				read(serverSocket,&buffer,sizeof(char));

				if(buffer!='\0')
				fprintf(stderr,"%c",buffer);
			}
						fprintf(stderr,"\n\n");
			flag2=0;

		}
		else if(temp=='y' )
		{
			char n='\n';
			fprintf(stderr,"CTRL+C geldi!\n");
		/*	write(STDIN_FILENO,&n,sizeof(char));
			fprintf(stderr,"buffera yazdim!\n");*/
			doneflag=1;
			flag=0;
			flag2=0;
			return NULL;
		}

		else if(temp=='k')
		{
			fprintf(stderr,"Server Kapandi!\n");
			kill(getpid(),2);
			flag=0;
			flag2=0;
			return NULL;
		}
	}

	close(ss);

}


void* sendFile(void* param)
{
	
	pthread_mutex_lock(&mutex);
	Package p=*(Package*)param;
	struct stat file_stat;
	int fd;
	int i=0;
    fd = open(p.filename, O_RDONLY);
	if(fd==-1)
	{	
		fprintf(stderr,"Bu dosyaya sahip degilsin!\n");
		pthread_mutex_unlock(&mutex);
		return NULL;
	}
    char null='\0';
	char buffer;  

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	if (fd == -1)
    {
          //  fprintf(stderr, "Error opening file --> %s", strerror(errno));
            exit(EXIT_FAILURE);
    }

	 if (fstat(fd, &file_stat) < 0)
    {
         //       fprintf(stderr, "Error fstat --> %s", strerror(errno));

                exit(EXIT_FAILURE);
    }

	fprintf(stderr,"Girilen dosya:%s\n",p.filename);
	Scontrol=0;
	write(p.serverSocket,&i,sizeof(int));
	write(p.serverSocket,&p.id,sizeof(int));
	Scontrol=1;
	i=0;
	while(p.filename[i]!='\0'){
			fprintf(stderr,"%c\n",p.filename[i]);
			write(p.serverSocket,&p.filename[i],sizeof(char));
			++i;	
	}


	write(p.serverSocket,&null,sizeof(char));
	
	write(p.serverSocket,&file_stat, sizeof(file_stat));

	
	for(i=0;i<file_stat.st_size;++i){

	read(fd,&buffer,1);
	write(p.serverSocket,&buffer,1);

	}


	close(fd);	

	pthread_mutex_unlock(&mutex);
}



void help()
{
	fprintf(stderr,"***COMMANDS***\n");
	fprintf(stderr,"  __________\n");
	fprintf(stderr,"  listLocal\n");
	fprintf(stderr,"  listServer\n");
	fprintf(stderr,"  lsClients\n");
	fprintf(stderr,"  sendFile <filename> <clientid>\n");

}

void listLocal(){


	int fileNameSize;

	char newPath[255];
	char path[255];
	
	DIR *pathDir;

	struct dirent *pathDirent;

	getcwd(path,255);

	pathDir = opendir (path);
	
	

    if (pathDir == NULL) 
	{
            fprintf (stderr,"Cannot open directory '%s'\n", path);
            exit(0);
    }

	while ((pathDirent = readdir(pathDir)) != NULL  )
	{
			
			strcpy(newPath,path);	
			
			fileNameSize=strlen(pathDirent->d_name);
			

			if(pathDirent->d_name[0]!='.' && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				
				fprintf(stderr,"%s\n",pathDirent->d_name);
			}

	}

	closedir (pathDir);

}
