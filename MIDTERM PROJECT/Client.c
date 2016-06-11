/*

	Ahmet Emin Kaplan 131044042
	CSE 244 Midterm	
	Client.c
*/


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
#include<time.h>

#define SERVERNAME "Serverfifo"

/*Global Degiskenler*/

int serverfd=0;
int clientfd=0;
FILE* logFilePtr;


void readFunctions(char *firstFunction,char *secondFunction,char* firstTxtFileName,char* secondTxtFileName);
void usage();

/*Signal handler*/
static void Signal_Handler(int signo) {

	int temp;
	temp=getpid();
	temp=temp*(-1);
		close(serverfd);

	char fname[10];
	
	sprintf(fname,"%d",getpid());
	if(signo==SIGINT || signo==SIGHUP){

	serverfd=open(SERVERNAME,O_WRONLY);
	write(serverfd,&temp,sizeof(int));
	if(signo==SIGINT){
	fprintf(logFilePtr,"Client Terminated!(CTRL+C)\n");
	fprintf(stderr,"Client Terminated!(CTRL+C)\n");
	}
	else{
	fprintf(logFilePtr,"Client Terminated!(SIGHUP)\n");
	fprintf(stderr,"Client Terminated!(SIGHUP)\n");
	}
	}
	else if(signo==SIGUSR2)
	{
	fprintf(stderr,"Server Full ! Please try again later !\n");
			fprintf(logFilePtr,"Server Full ! Please try again later !\n");
	}

	else if(signo==SIGALRM)
	{
		serverfd=open(SERVERNAME,O_WRONLY);
		write(serverfd,&temp,sizeof(int));
		fprintf(stderr,"Server yanit vermiyor!\n");
		fprintf(logFilePtr,"Server yanit vermiyor!\n");
	}

	else if(signo==SIGUSR1)
	{

		fprintf(stderr,"Server closed!\n");
		fprintf(logFilePtr,"Server closed!\n");
	}

	fclose(logFilePtr);
	close(serverfd);

	close(clientfd);

	unlink(fname);

	exit(0);
}

int main (int argc, char *argv[]) {


	int fdserver,fdclient,i=0;
	char clientFifoname[50];
	char clientLogFileName[50];
	char FirstFunctionBuffer[1000];
	char SecondFunctionBuffer[1000];
	double time_interval;
	char operation;	
	char TIME[BUFSIZ];
	int pid;
	
	double result;
	
	if(argc!=5)
	{
			usage();
	}

	else if(strncmp(argv[2],"f1.txt",2)!=0 && strncmp(argv[2],"f2.txt",2)!=0 && strncmp(argv[2],"f3.txt",2)!=0
	&& strncmp(argv[2],"f4.txt",2)!=0 && strncmp(argv[2],"f5.txt",2)!=0 && strncmp(argv[2],"f6.txt",2)!=0)
	{
		fprintf(stderr,"Yanlis Dosya ismi girildi!\n");
		usage();
	}

	else if(strncmp(argv[1],"f1.txt",2)!=0 && strncmp(argv[1],"f2.txt",2)!=0 && strncmp(argv[1],"f3.txt",2)!=0
	&& strncmp(argv[1],"f4.txt",2)!=0 && strncmp(argv[1],"f5.txt",2)!=0 && strncmp(argv[1],"f6.txt",2)!=0)
	{
		fprintf(stderr,"Yanlis Dosya ismi girildi!\n");
		usage();
	}
	
	else if(argv[4][0]!='/' && argv[4][0]!='-' && argv[4][0]!='+' && argv[4][0]!='x' )
	{
		fprintf(stderr,"Yanlis Operation girildi!\n");
		usage();
	}

	strcpy(FirstFunctionBuffer,argv[1]);
	strcpy(SecondFunctionBuffer,argv[2]);
	sscanf(argv[3],"%lf",&time_interval);
	
	if(argv[4][0]=='x')
		operation='*';
	else	
		operation=argv[4][0];


	fdserver=open(SERVERNAME,O_WRONLY);

	if(fdserver==-1)
	{
		fprintf(stderr,"Server bulunamadi\n");
		exit(0);
	}

	serverfd=fdserver;
	struct sigaction act;
	act.sa_handler = Signal_Handler;
	act.sa_flags = 0;

	if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGALRM, &act, NULL) == -1)|| (sigaction(SIGINT, &act, NULL) == -1) || (sigaction(SIGUSR1, &act, NULL) == -1) || (sigaction(SIGHUP, &act, NULL) == -1) || (sigaction(SIGUSR2, &act, NULL) == -1)) 
	{
		perror("Failed to set Signal handler");
		exit(1);
	}


	pid=getpid();

	sprintf(clientFifoname,"%d",pid);
	sprintf(clientLogFileName,"%d.log",pid);
	if(mkfifo(clientFifoname,0600)==-1){
		
		perror("Failed to Create Fifo\n");
		exit(0);
	
	}
	
	logFilePtr=fopen(clientLogFileName,"w");

	if(logFilePtr==NULL)
	{
		fprintf(stderr,"Log file olusturulamadi!\n");
		exit(1);	
	}
	

	write(fdserver,&pid,sizeof(int));

	close(fdserver);

	time_t ClientTimer;

    struct tm* time_info;

    time(&ClientTimer);

    time_info = localtime(&ClientTimer);
	
    strftime(TIME, 26, "%d.%m.%Y, %H:%M:%S", time_info);

	fprintf(logFilePtr, "Client Pid: %s\n", clientFifoname);

	

	fprintf(logFilePtr, "Aritmetical operation and Function file names: %s %s %c\n", FirstFunctionBuffer, SecondFunctionBuffer, operation);
	
	fprintf(logFilePtr, "CONNECTION TIME: %s\n", TIME);

	fdclient=open(clientFifoname,O_WRONLY);
	if(fdclient==-1)
	{
		perror("ClientFifo couldnt be opened!\n");
		exit(0);
	}	

	clientfd=fdclient;
	i=0;
	while(FirstFunctionBuffer[i]!='\0')
	{
		write(fdclient,&FirstFunctionBuffer[i],sizeof(char));
		++i;
	}
		write(fdclient,&FirstFunctionBuffer[i],sizeof(char));
	i=0;

	while(SecondFunctionBuffer[i]!='\0')
	{
		write(fdclient,&SecondFunctionBuffer[i],sizeof(char));
		++i;
	}
	write(fdclient,&SecondFunctionBuffer[i],sizeof(char));

	write(fdclient,&operation,sizeof(char));
	//perror("yazdim op");
	write(fdclient,&time_interval,sizeof(double));
	//perror("yazdim timeinterval");

	close(fdclient);
	fdclient=open(clientFifoname,O_RDONLY);
	
	if(fdclient==-1)
	{
		perror("ClientFifo couldnt be opened!\n");
		exit(0);
	}	
	

	while(1){
	alarm(30);
	read(fdclient,&result,sizeof(double));
	fprintf(stderr,"SONUC:%f\n",result);
	fprintf(logFilePtr,"Result:%f\n",result);	
	}
	
	close(fdclient);
	unlink(clientFifoname);
    perror("unlink");
     return 0;
}


void usage()
{

		fprintf(stderr,"Usage: Client -<fi> -<fj> -<time interval> -<operation>\n");
		fprintf(stderr,"Sample Usage: f1 f2 2.5 x\n");
		fprintf(stderr,"Sample Usage: f3 f6 4.0 +\n");
		fprintf(stderr,"Sample Usage: f5 f4 6.0 -\n");
		fprintf(stderr,"Sample Usage: f4 f2 10.0 /\n");
		exit(0);
}
