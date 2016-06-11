/*

Ahmet Emin Kaplan 131044042
	CSE 244 Midterm
	IntegralGen.c

*/

/*KUTUPHANELER*/

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
#include<matheval.h>
#include<sys/time.h>

#define SERVERNAME "Serverfifo"

/*Global Degiskenler*/

int AllClientspids[1000];
int AllminiServersPid[1000];
int numOfAllminiServers=0;
int numberOfClient=0;
int ServerFd;
char AllFunctions[6][1000];
struct  timeval STARTTIME;
void miniServer(int clientPid,struct  timeval connectionTime);
void createNewCFile(char* function1,char* function2,char operation);
void readFunctions();
void killMiniServer(int pid);
void killClient(int pid);
int isMain=1;
void *f1;
void *f2;
int clientFifofd_gl;
int clientPid_gl;

double Resolution;
int MaximumClient;

/*Signal Handler*/
static void Signal_Handler(int signo) {
	
	int status,i,k;	
	pid_t wpid;
	int pid=getpid();

	if(isMain){

		if(signo==SIGINT)
		fprintf(stderr,"CTRL+C Basildi Server Kapaniyor!\n");
		else if(signo==SIGHUP)
		{
			fprintf(stderr,"Hang-up signal yakalandi!\n");
		}
	
		while ((wpid = wait(&status)) > 0);
	
		for(i=0;i<numberOfClient;++i)
		{		
				kill(AllClientspids[i],SIGUSR1);
		}
	
		    
		close(ServerFd);
		unlink(SERVERNAME);
	
	}

	
	else if(isMain==0){
		
		//	kill(clientPid_gl,SIGUSR1);
			evaluator_destroy(f1);
		
			evaluator_destroy(f2);
			
			close(clientFifofd_gl);
			
	}

	exit(0);

}



int main (int argc, char *argv[]) {



	int GeneralServerFd;
	int childpid,i,status;
	int tempPID;
	struct  timeval connectionTime;


	if(argc!=3)
	{
		fprintf(stderr,"Usage: <Resolution(Double)> <Max # of Clients(Int)>\n");
		exit(1);
	}
	
	sscanf(argv[1],"%lf",&Resolution);
	sscanf(argv[2],"%d",&MaximumClient);

	gettimeofday(&STARTTIME,NULL);
	readFunctions();
	

	if(mkfifo(SERVERNAME,0600)==-1)
	{
		perror("Failed to Create Fifo\n");
		exit(0);
	}


	/*Clientlerin baglanti kuracagi fifo Okuma ve yazma moduyla server tarafindan acilir*/
	/*Boylece Server gereksiz yere Birilerinin baglanmasini beklemez*/

	GeneralServerFd=open(SERVERNAME,O_RDWR);
	if(GeneralServerFd==-1)
	{
		perror("Server Fifo couldnt be opened!\n");
		exit(0);
	}

	ServerFd=GeneralServerFd;

	struct sigaction act;
	act.sa_handler = Signal_Handler;
	act.sa_flags = 0;

	if ((sigemptyset(&act.sa_mask) == -1) ||  (sigaction(SIGHUP, &act, NULL) == -1) || (sigaction(SIGINT, &act, NULL) == -1) || (sigaction(SIGUSR2, &act, NULL) == -1)) 
	{
		perror("Failed to set Signal handler");
		exit(1);
	}
		
	fprintf(stderr,"Main Server PID : %d \n",getpid());


	/*Program herhangi bir signal tarafindan sonlandirilmadigi surece yasayacaktir*/
	/*Eger bir client servera baglanirsa client kendi pid sini yollar*/
	/*Servergelen pid yi okuduktan sonra gelen clienti fork ile olusturdugu miniserver a devreder*/
	while(1){
		
		read(GeneralServerFd,&tempPID,sizeof(int));

		/*Clientlerin baglanti zamani kaydedilir*/

		gettimeofday(&connectionTime,NULL);

		if(tempPID>0 && numOfAllminiServers<MaximumClient){
			
			
			childpid=fork();
			if(childpid==-1)
			{
				fprintf(stderr,"Failed to create fork!\n");
				exit(1);
			}
			if(childpid==0){
				isMain=0;
				miniServer(tempPID,connectionTime);	
				exit(0);		
			}	
			else{
				fprintf(stderr,"Client %d Connected to Miniserver %d\n",tempPID,childpid);
				AllClientspids[numberOfClient]=tempPID;
				AllminiServersPid[numberOfClient]=childpid;
				++numberOfClient;
				++numOfAllminiServers;	
				
			}
	

		}
		else if(tempPID<0){
			--numOfAllminiServers;
		 	killMiniServer(tempPID);

			
		}

		else if(numOfAllminiServers==MaximumClient)
		{
			killClient(tempPID);	

		}


	}

    	 return 0;
}


/*Eger clientlerden biri sonlandigi zaman sonlanan client servera kendi pidsinin negatifini yollayarak sonlandigini bildirir ve bu pid den
Sonlanan client ile ilgilenen mini serverin pidsi elde edilir.Ve Bu mini server sonlandirilir*/
void killMiniServer(int pid)
{
	int temp=pid*(-1);
	int i=0;
	for(i=0;i<numberOfClient;++i)
	{
		if(AllClientspids[i]==temp)
		{
			fprintf(stderr,"Miniserver %d and Client %d Disconnected\n",AllminiServersPid[i],temp);
			kill(AllminiServersPid[i],SIGUSR2);
			break;
		}
	
	}

}

/*Eger server full durumdaysa client e SIGUSR2 signali yollanir*/
void killClient(int pid)
{
	kill(pid,SIGUSR2);
}

/*Client ile ilgilenen miniserverin kodudur*/
void miniServer(int clientPid,struct  timeval connectionTime)
{
	void * function1;
	void * function2;
	int i=0;
	double one=1;
	char junk='a';
	double result=0.0;
	double result2=0.0;
	double lastResult=0.0;
	char FunctionFile1[1000];
	char FunctionFile2[1000];
	char Function1[1000];
	char Function2[1000];

	char clientFifoName[50];
	double time_interval=0;
	double k;
	char operation;
	double t0;
	int clientFifoFd;
	int miniServerPid;
	int childpid,childpid2;
	sprintf(clientFifoName,"%d",clientPid);
	
	miniServerPid=getpid();
	clientPid_gl=clientPid;

	/*Clientin pid si isminde acilmis olan fifofan clientin yolladigi veriler okunur ve bu verilere gore sonuc elde edilip cliente yollanir*/

	clientFifoFd=open(clientFifoName,O_RDONLY);
	if(clientFifoFd==-1)
	{
		perror("ClientFifo couldnt be opened!\n");
		exit(0);
	}
	i=0;

	do
	{
		read(clientFifoFd,&junk,sizeof(char));
		FunctionFile1[i]=junk;
		++i;
	}
	while(junk!='\0');

	junk='a';
	i=0;
	do
	{
		read(clientFifoFd,&junk,sizeof(char));
		FunctionFile2[i]=junk;
		++i;
	}
	while(junk!='\0');

	read(clientFifoFd,&operation,sizeof(char));

	read(clientFifoFd,&time_interval,sizeof(double));

	close(clientFifoFd);



	if(strncmp(FunctionFile1,"f1.txt",2)==0)
		strcpy(Function1,AllFunctions[0]);
	else if(strncmp(FunctionFile1,"f2.txt",2)==0)
		strcpy(Function1,AllFunctions[1]);
	else if(strncmp(FunctionFile1,"f3.txt",2)==0)
		strcpy(Function1,AllFunctions[2]);
	else if(strncmp(FunctionFile1,"f4.txt",2)==0)
		strcpy(Function1,AllFunctions[3]);
	else if(strncmp(FunctionFile1,"f5.txt",2)==0)
		strcpy(Function1,AllFunctions[4]);
	else if(strncmp(FunctionFile1,"f6.txt",2)==0)
		strcpy(Function1,AllFunctions[5]);


	if(strncmp(FunctionFile2,"f1.txt",2)==0)
		strcpy(Function2,AllFunctions[0]);
	else if(strncmp(FunctionFile2,"f2.txt",2)==0)
		strcpy(Function2,AllFunctions[1]);
	else if(strncmp(FunctionFile2,"f3.txt",2)==0)
		strcpy(Function2,AllFunctions[2]);
	else if(strncmp(FunctionFile2,"f4.txt",2)==0)
		strcpy(Function2,AllFunctions[3]);
	else if(strncmp(FunctionFile2,"f5.txt",2)==0)
		strcpy(Function2,AllFunctions[4]);
	else if(strncmp(FunctionFile2,"f6.txt",2)==0)
		strcpy(Function2,AllFunctions[5]);


	clientFifoFd=open(clientFifoName,O_WRONLY);

	if(clientFifoFd==-1)
	{
		perror("ClientFifo couldnt be opened!\n");
		exit(0);
	}	

	/*libmatheval.h kutuphanesinin icinde expresiondan fonksiyonun return degerini veren evaluater yapilari olusturulur*/
	/*Miniserver sirayla t0 <-->(t0+ti)    t0<-->(t0+ti+ti)   seklinde her seferinde araligi buyulterek integral hesaplar */
	function1=evaluator_create (Function1);
	f1=function1;
	function2=evaluator_create(Function2);	
	f2=function2;	
	result=0.0;
	result2=0.0;
	double o=time_interval;

	lastResult=0.0;
	k=0.0;
	t0=(connectionTime.tv_sec-STARTTIME.tv_sec)+(connectionTime.tv_usec-STARTTIME.tv_usec)/1000000.0;
	time_interval+=t0;
	k=t0;
	clientFifofd_gl=clientFifoFd;
	/*Mini server sonlanmadigi surece devam edecek olan integral hesaplari cliente yollanir*/
	while(1){

		
		for(;k<time_interval;k+=Resolution){
			result=evaluator_evaluate_x(function1,k);
			result2=evaluator_evaluate_x(function2,k);

			if(operation=='+')
				lastResult+=(result+result2)*Resolution;
			else if(operation=='-')
				lastResult+=(result-result2)*Resolution;
			else if(operation=='/')
				lastResult+=(result/result2)*Resolution;
			else if(operation=='*')
				lastResult+=(result*result2)*Resolution;
			}
		
	
	write(clientFifoFd,&lastResult,sizeof(double));

	time_interval+=o;
	}
	close(clientFifoFd);

}


/*Server acildigi zaman butun fonksiyon dosyalarini okuyan fonksiyondur*/
void readFunctions()
{	

	char Functions[6][100]={"f1.txt","f2.txt","f3.txt","f4.txt","f5.txt","f6.txt"};

	int i=0;
	int k=0,t=0;
	char junk;

	FILE* functionFilePtr;	
	for(k=0;k<6;++k){

		functionFilePtr=fopen(Functions[k],"r");
	
		if(functionFilePtr==NULL)
		{
			fprintf(stderr,"DOSYA ACMA HATASI %s!\n",Functions[k]);
			exit(0);
		}
		i=0;
		while(fscanf(functionFilePtr,"%c",&junk)!=EOF)
		{
			AllFunctions[k][i]=junk;
			++i;		
		}

		AllFunctions[k][i-1]='\0';

		fclose(functionFilePtr);
		for(t=0;t<i;++t)
		{
			if(AllFunctions[k][t]=='t' &&
		 (
(t==0 || AllFunctions[k][t-1]=='+' || AllFunctions[k][t-1]=='-' || AllFunctions[k][t-1]=='(' || AllFunctions[k][t-1]=='*' || AllFunctions[k][t-1]=='/')
 && (AllFunctions[k][t+1]=='+'|| AllFunctions[k][t+1]==')' || AllFunctions[k][t+1]=='-' || AllFunctions[k][t+1]=='*' || AllFunctions[k][t+1]=='/' || AllFunctions[k][t+1]=='\0')))	
				AllFunctions[k][t]='x';

		}

	}	
}


