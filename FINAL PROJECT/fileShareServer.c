/*Ahmet Emin Kaplan 131044042 Sistem programlama final projesi*/
/*fileShareServer.c*/

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
#include<pthread.h>
#include <arpa/inet.h>

struct  timeval STARTTIME;


typedef struct{

	int sendClientSocket;
	int clientId;
	int receiveClientSocket;
}Package;

typedef struct{
	int clientSocket;
	int clientId;
}ClientPackage;


int mainSocketNumber1;
int doneflag=0;
int newSocketNumber[100];
int numberOfThread=1;
int AllClientIDS[100];
ClientPackage AllClientPackages[100];
pthread_mutex_t AllMutexes[100];
pthread_t AllThreads[100];

static void SignalHandler(int signo) {

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	doneflag=1;
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	
	int i;
	char k='k';
	for(i=0;i<100;++i)
	{	
			
		if(AllClientIDS[i]!=-1)
		{
			pthread_mutex_lock(&AllMutexes[i]);

			write(newSocketNumber[i],&k,sizeof(char));			

			pthread_mutex_unlock(&AllMutexes[i]);

			pthread_mutex_destroy(&AllMutexes[i]);

		}
	}

	for(i=0;i<numberOfThread;++i)
	{	
			

			pthread_join(AllThreads[i],NULL);

		
	}

	close(mainSocketNumber1);
	exit(0);
}

/*Dersin slaytlarindan aldigim establish fonksiyonu verilen port numarasinda socket olusturur*/
int establish (unsigned short portnum);

/*Baglanan her client icin olusturulan thread fonksiyonudur*/
void* miniServer(void* param);

/*Clientlar ile server arasinda dosya transferini saglayan fonksiyonlar*/
void* receiveFile(void* param);
void* sendFile(void* param);

int main(int argc,char *argv[]){

	int mainSocketNumber;


	
	struct  timeval TEMPTIME;
	
	struct sigaction act;
	act.sa_handler = SignalHandler;
	act.sa_flags = 0;
	double timedif;

	if(argc!=2)
	{
		fprintf(stderr,"Usage:<PortNumber>\n");
		fprintf(stderr,"SampleUsage: ./fileShareServer 8080\n");		
		exit(1);
	}
	if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1)) 
	{
		perror("Failed to set CTRL+C handler");
		exit(1);
	}

	/*Program basladiginda baslangic zamani global degiskene kopyalanir*/
	gettimeofday(&STARTTIME,NULL);

	/*Girilen port numarasiyla socket olusturulur eger bir hata varsa program biter*/
	mainSocketNumber=establish(atoi(argv[1]));

	if(mainSocketNumber==-1)
	{
		fprintf(stderr,"Server kurulamadi muhtemelen bu port kullaniliyor!\n");
		exit(1);	
	}
	
	
	mainSocketNumber1=mainSocketNumber;
	
	int i;
	for(i=0;i<100;++i)
	{
		AllClientPackages[i].clientId=-1;
		AllClientIDS[i]=-1;
	}

	int numberOfClient=1;

	while(1){	
	
		newSocketNumber[numberOfClient]= accept(mainSocketNumber,NULL,NULL);
		AllClientIDS[numberOfThread]=numberOfThread;
		AllClientPackages[numberOfThread].clientId=numberOfThread;
		AllClientPackages[numberOfThread].clientSocket=newSocketNumber[numberOfClient];

		pthread_mutex_init(&AllMutexes[numberOfThread],NULL);

		write(newSocketNumber[numberOfClient],&numberOfClient,sizeof(int));

		/*Baglanan her clint icin cagrilan thread fonksiyonu */

		pthread_create(&AllThreads[numberOfClient],NULL,miniServer,&AllClientPackages[numberOfClient]);
		
		gettimeofday(&TEMPTIME,NULL);
		timedif=((TEMPTIME.tv_sec-STARTTIME.tv_sec)*1000.0)+((TEMPTIME.tv_usec-STARTTIME.tv_usec)/1000.0);
		
		fprintf(stderr,"Client %d Connected! currentTime:%0.2f\n",numberOfClient,timedif);
	
		++numberOfThread;
		++numberOfClient;
	}
	


	return 0;
}

void* miniServer(void* param)
{

		struct  timeval TEMPTIME;
		sigset_t set;
		sigemptyset(&set);
		sigaddset(&set, SIGINT);
		
		pthread_sigmask(SIG_BLOCK, &set, NULL);

		Package AllPackages[100];
		int numberOfPackages=0;
		int command=-1;

		int clientSocket=(*(ClientPackage*)param).clientSocket;
		int clientId=(*(ClientPackage*)param).clientId;

		char* filename;
		char t_filename[255];
		int sendClientID;
		int i=0;
		int k=0;
		char l='l';
		char s='s';
		int minus1=-1;
		int fileNameSize;
		char newPath[255];
		char path[255];
		char null='\0';
		char enter='\n';
		DIR *pathDir;
		double timedif;
		struct dirent *pathDirent;

		getcwd(path,255);

		
		/*Thread fonksiyonu surekli client tarafindan komut okur*/
		while(1)
		{
				/*karsi taraftan soketin kapatilmasi durumunda bu fonksiyon 0 return edecektir*/
			if(0==read(clientSocket,&command,sizeof(int)))
			{
				gettimeofday(&TEMPTIME,NULL);

				timedif=((TEMPTIME.tv_sec-STARTTIME.tv_sec)*1000.0)+((TEMPTIME.tv_usec-STARTTIME.tv_usec)/1000.0);

				fprintf(stderr,"Client %d disconnected! currentTime:%0.2f\n",clientId,timedif);

				AllClientIDS[clientId]=-1;

				close(clientSocket);

				pthread_mutex_destroy(&AllMutexes[clientId]);

				return NULL;
			}			

			/*Eger socketten 0 komutu okunmussa bu file transferi simgeler ve sonrasinda transfer edilecek clientin id si okunur
				eger okunan id -999 ise bu dosya servera yollanmistir */
			if(command==0)
			{	
					gettimeofday(&TEMPTIME,NULL);
					timedif=((TEMPTIME.tv_sec-STARTTIME.tv_sec)*1000.0)+((TEMPTIME.tv_usec-STARTTIME.tv_usec)/1000.0);
					
					read(clientSocket,&sendClientID,sizeof(int));
					
					if(sendClientID==-999)
					fprintf(stderr,"Client %d request sendfile to Server currentTime:%0.2f\n",clientId,timedif);
					else
					fprintf(stderr,"Client %d request sendfile to Client %d currentTime:%0.2f\n",clientId,sendClientID,timedif);

					AllPackages[numberOfPackages].sendClientSocket=newSocketNumber[sendClientID];
	
					AllPackages[numberOfPackages].receiveClientSocket=clientSocket;
					AllPackages[numberOfPackages].clientId=sendClientID;

					if(sendClientID!=-999)
					pthread_mutex_lock(&AllMutexes[sendClientID]);

					receiveFile(&AllPackages[numberOfPackages]);

					if(sendClientID!=-999)
					pthread_mutex_unlock(&AllMutexes[sendClientID]);
					gettimeofday(&TEMPTIME,NULL);
					timedif=((TEMPTIME.tv_sec-STARTTIME.tv_sec)*1000.0)+((TEMPTIME.tv_usec-STARTTIME.tv_usec)/1000.0);										

					if(sendClientID!=-999)	
					fprintf(stderr,"Client %d to %d file transfer completed currentTime:%0.2f\n",clientId,sendClientID,timedif);
					else
					fprintf(stderr,"Client %d file transfer completed to Server currentTime:%0.2f\n",clientId,timedif);
							
			}

			/*Eger komut 1 ise client lsClients istegi yollamistir*/
			else if(command==1)
			{	
					
				gettimeofday(&TEMPTIME,NULL);
				
				timedif=((TEMPTIME.tv_sec-STARTTIME.tv_sec)*1000.0)+((TEMPTIME.tv_usec-STARTTIME.tv_usec)/1000.0);
					
				
				fprintf(stderr,"Client %d request lsClients  currentTime:%0.2f\n",clientId,timedif);

				pthread_mutex_lock(&AllMutexes[clientId]);

				write(clientSocket,&l,sizeof(char));

				for(i=0;i<numberOfThread;++i)
				{
					if(AllClientIDS[i]!=-1)
						write(clientSocket,&AllClientIDS[i],sizeof(int));
				}
				write(clientSocket,&minus1,sizeof(int));

				pthread_mutex_unlock(&AllMutexes[clientId]);			
			}

			/*Eger komut 2 ise client listServer istegi yollamistir*/
			else if(command==2)
			{			
					gettimeofday(&TEMPTIME,NULL);
				
					timedif=((TEMPTIME.tv_sec-STARTTIME.tv_sec)*1000.0)+((TEMPTIME.tv_usec-STARTTIME.tv_usec)/1000.0);
					
				
					fprintf(stderr,"Client %d request listServer  currentTime:%0.2f\n",clientId,timedif);



					pthread_mutex_lock(&AllMutexes[clientId]);

					write(clientSocket,&s,sizeof(char));
					/*lssonucunu yaz burada*/	
					pathDir = opendir (path);
					
					if (pathDir == NULL) 
					{
            			fprintf (stderr,"Cannot open directory '%s'\n", path);
            			exit(0);
    				}

					while ((pathDirent = readdir(pathDir)) != NULL  )
					{
			
						
			
						fileNameSize=strlen(pathDirent->d_name);
			

						if(pathDirent->d_name[0]!='.' && pathDirent->d_name[fileNameSize-1]!='~')
						{	k=0;
				
							while(pathDirent->d_name[k]!='\0'){

								
								write(clientSocket,&(pathDirent->d_name[k]),sizeof(char));
								++k;
							}
								write(clientSocket,&enter,sizeof(char));						
						}

					}

					closedir (pathDir);					
					write(clientSocket,&null,sizeof(char));
					pthread_mutex_unlock(&AllMutexes[clientId]);
			}

			/*Bu komut ise clientin servera beni oldur mesaji yollamasini ifade eder*/
			/*Client tarafinda 1 threadin tek gorevi sonsuz defa okuma yapmaktir read blogunda kalmamasi icin server o bloga sentinel deger yollar */
			else if(command==5)
			{
				char ky='y';
				pthread_mutex_lock(&AllMutexes[clientId]);
				write(clientSocket,&ky,sizeof(char));
				pthread_mutex_unlock(&AllMutexes[clientId]);
				
			}	
			else
			{
			
				break;
			}
			
		}


	
}



void* receiveFile(void* param)
{

	int clientSocket=(*(Package*)param).receiveClientSocket;	
	int sendClientSocket=(*(Package*)param).sendClientSocket;
	int sendCClientID=(*(Package*)param).clientId;

	struct stat file_stat;
	int fd;
	int i=0;
 	char buffer='x';
	char filename[255];
	char path[255];
	getcwd(path,255);
	filename[0]='\0';
	char f='f';


	if(sendCClientID==-999)
	{
		while(buffer!='\0')
		{
			read(clientSocket,&buffer,sizeof(char));
			filename[i]=buffer;
			++i;
		}

		fd=open(filename, O_CREAT | O_WRONLY | O_TRUNC,0777);

		read(clientSocket,&file_stat, sizeof(file_stat));

		for(i=0;i<file_stat.st_size;++i)
		{		
			
				read(clientSocket,&buffer,1);
				write(fd,&buffer,1);	
		}
			
		close(fd);
	}

	else{

		write(sendClientSocket,&f,sizeof(char));

		while(buffer!='\0'){
			read(clientSocket,&buffer,sizeof(char));
			write(sendClientSocket,&buffer,sizeof(char));		
			filename[i]=buffer;
			++i;
		}
	
		fprintf(stderr,"filename:%s\n",filename);
		strcat(path,"/");
		strcat(path,filename);

		fprintf(stderr,"%s\n\n\n",path);

	
		read(clientSocket,&file_stat, sizeof(file_stat));
		write(sendClientSocket,&file_stat,sizeof(file_stat));

	
		perror("d");
		for(i=0;i<file_stat.st_size;++i){
		
			
			read(clientSocket,&buffer,1);
			write(sendClientSocket,&buffer,1);	
		}


	}

	fprintf(stderr,"Dosya transferi basarili!\n");

	
}

int establish (unsigned short portnum)
{
	char myname[ HOST_NAME_MAX];

	int s;

	struct sockaddr_in serverAdress;

	struct hostent *hp;

	memset(&serverAdress, 0, sizeof(struct sockaddr_in));

	gethostname(myname,HOST_NAME_MAX);

	hp= gethostbyname(myname);

	if (hp == NULL) 
		return -1;

	serverAdress.sin_family= AF_INET;
	
	serverAdress.sin_port= htons(portnum);
	serverAdress.sin_addr.s_addr=htonl(INADDR_ANY);
 	
	if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	if (bind(s,(struct sockaddr *)&serverAdress,sizeof(struct sockaddr_in)) < 0)
	{
		close(s);
		return -1;	
	}
	
	listen(s, 10);
	
	return s;
}

