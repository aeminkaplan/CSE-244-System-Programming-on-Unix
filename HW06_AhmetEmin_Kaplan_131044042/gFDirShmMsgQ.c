/*
*
*			CSE 244 HW 06
*			grepFromDirShmMsgq.c
*		Ahmet Emin Kaplan 131044042
*
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
#include<pthread.h>
#include <sys/resource.h>
#include<semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#define SEMAPHORENAME "/mysem"
#define SECONDSEMOPHORE "/secondsem"

int NumberOfMsgQ=0;
int *allMsqPtr;
sem_t* mainSemaphore;
sem_t* secondSemaphore;

char* SharedMemory;
int numberOfMatch=0;
int GlobalShMem;


typedef struct{
	long mtype;
	int first;
	int second;

}Location;

typedef struct{

	char filenamePtr[255];
	char target[100];
	int msqid;

}Allparams;

#define OUTPUT_FILE_NAME "gfDirShmMsgq.log"


/*Signal Handler olusan shared memory ve semaphore u silecek*/

static void SignalHandler(int signo) {

	int z;
	int wpid,status;
	
	//unlink(GENERALFIFO);
	
	while ((wpid = wait(&status)) > 0);

	for(z=0;z<NumberOfMsgQ;++z)	
	msgctl(allMsqPtr[z],IPC_RMID, NULL);

	sem_close(mainSemaphore);
	sem_close(secondSemaphore);

	sem_unlink(SEMAPHORENAME);
	sem_unlink(SECONDSEMOPHORE);
	
	shmdt(SharedMemory);
	shmctl(GlobalShMem, IPC_RMID , 0);
	exit(1);
}

/*FONKSIYONLAR*/

/*Acilacak ve icinde arama yapilacak dosyalar icin gerekli buffer buyuklugunu hesaplayan fonksiyondur*/
int calculateBufferSize(char* filenamePtr);

/*Recursive olarak butuk alt klasorleri gezen fonksiyondur*/
void FindAndExecuteSubDirectories(const char* path,const char* target,int sq);

/*Girilen path in directory olup olmadigini return eden fonksiyondur*/
int isDirectory(const char *path);

/*Girilen dosyada arama yapan fonksiyondur*/
void* grep(void* param);

/*Alt directory sayisini return eden fonksiyondur*/
int CountAllSubDirectories(const char* path);



int main (int argc, char *argv[]) {

/*Recursive cagri sayisi fazla oldugu durumlarda stacksize yeterli olmamakta bunun icin stacksize limitini yukseltiyorum*/

	int temp;
	struct rlimit limit;
	limit.rlim_cur = 1000000000;
  	limit.rlim_max = 1000000000;

	setrlimit(RLIMIT_STACK,&limit);

	if (argc!=3) {
    	
		fprintf (stderr,"Usage: testprog <dirname_or_path>   <target>\n");
        return 1;
   	}
	
/*Onceden bu isimde semaphore olma ihtimaline karsi unlink ediyorum*/

	sem_unlink(SEMAPHORENAME);
	sem_unlink(SECONDSEMOPHORE);


	mainSemaphore = sem_open(SEMAPHORENAME, O_CREAT|O_EXCL, 0644, 1);
	secondSemaphore	=sem_open(SECONDSEMOPHORE, O_CREAT|O_EXCL, 0644, 0);
	
	temp=shmget(9876541,sizeof(char), IPC_CREAT | 0666);

	GlobalShMem=temp;

	SharedMemory=(char*)shmat(temp,NULL,0);

	

	FindAndExecuteSubDirectories(argv[1],argv[2],0);

	fprintf(stderr,"Pipe ve Fifolardan alinan verilerle olusturulan Dosya--> gfDirShmMsgq_result.log\n");

	fprintf(stderr,"Her bir threadin bulduklari eslesmeleri yazdiklari Dosya-->gfDirShmMsgq.log\n");
	

	fprintf(stderr,"Eslesme sayisi:%d\n",numberOfMatch);
	sem_close(mainSemaphore);
	sem_close(secondSemaphore);

	sem_unlink(SEMAPHORENAME);
	sem_unlink(SECONDSEMOPHORE);

	shmdt(SharedMemory);
	shmctl(temp, IPC_RMID , 0);
     return 0;
}


void FindAndExecuteSubDirectories(const char* path,const char* target,int sq)
{	
				
	pthread_t AllThreads[1000];
	int numberOfThread=0;
	Allparams AllDatas[1000];




	struct sigaction act;
	act.sa_handler = SignalHandler;
	act.sa_flags = 0;
	
	if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1)) 
	{
		perror("Failed to set CTRL+C handler");
		exit(1);
	}
	
	
	struct dirent *pathDirent;
	Location l1;
	l1.mtype=999;
	char newPath[255];
	DIR *pathDir;
	FILE *resultFptr;
	int fileNameSize;
	int temp,t,k;
	int result,status;
	pid_t childpid,wpid;
	int index,z,f=0,n=0,flag;
	int counter=0;
	key_t messageQueueKey;
	/*Dosyalar icin kullanacagim message queue id lerini tuttugum int arrayi*/

	int allMsqids[50];

	allMsqPtr=allMsqids;
	int numOfSub=0,numOfDir=0;
	/*log dosyasini olusturmak icin pipelardan okudugum degerleri yazacagim chracter arrayi*/
	char allLogs[50][10000];
	char temps[1000];
	char buffer[50][10000];
	/*Alt dosyalarin disimlerini tutacagim character arrayi*/
	char fileNames[50][255];
	int filenameCounter=0;
	
	for(z=0;z<50;++z)
	{
		fileNames[z][0]='\0';
		buffer[z][0]='\0';
	}

	
		
	for(z=0;z<50;++z)
		allLogs[z][0]='\0';
	
	pathDir = opendir (path);
	
    if (pathDir == NULL) 
	{
            fprintf (stderr,"Cannot open directory '%s'\n", path);
            exit(0);
    }
		
	
	/*  Path icindeki butun dosyalari ve klasorleri sayiyorum  */
    


	while ((pathDirent = readdir(pathDir)) != NULL  )
	{
			
			strcpy(newPath,path);	
			
			fileNameSize=strlen(pathDirent->d_name);
			

			if(pathDirent->d_name[0]!='.' && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				strcat(newPath,"/");	
				strcat(newPath,pathDirent->d_name);
			}

			/*Eger yeni olusturulan path directory ise directory sayisini artiriyorum*/
			if(isDirectory(newPath) && pathDirent->d_name[0]!='.' && pathDirent->d_name[fileNameSize-1]!='~')
			{
				++numOfDir;
			}
		  
			/*Eger yeni olusturulan path file altdosya sayisini artiriyorum*/
			else if (pathDirent->d_name[0]!='.' && !isDirectory(newPath) && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				++numOfSub;
			}
				
	}

	/*rewind islemi*/
	rewinddir(pathDir);

	/*Bu dongu ile path icerinde bulunan directoryler icin yeni child process ler olusturulur ve bu processler her bir file icin thread olusturacaktir process olusturan donguyu thread olusturan donguden once yapmamin sebebi child processler olusturulurken o anki threadlerde kopyalanacaktir bundan dolayi problemler meydana gelmesini onluyorum*/

	while ((pathDirent = readdir(pathDir)) != NULL)
	{
				
			strcpy(newPath,path);	
			
			fileNameSize=strlen(pathDirent->d_name);
			
			/*Eger alt klasorun ismi . veya .. veya  uzantisi ~ ise  isleme alinmayacak*/
			/*Aksi durumda yeni path olusturulacak*/

			if(pathDirent->d_name[0]!='.' && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				strcat(newPath,"/");	
				strcat(newPath,pathDirent->d_name);
			}

			/*Eger yeni olusturulan path directory ise child olusturulduktan sonra child tarafindan recursive cagri yapilacak*/
			if(isDirectory(newPath) && pathDirent->d_name[0]!='.' && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				
				childpid=fork();
				if(childpid==-1)
				{
					fprintf(stderr,"Fork Faillure!\n");
					exit(0);				
				}
				
				else if(childpid==0)
				{
			
				#ifdef DEBUG
					fprintf(stderr,"pid:%d parentpid:%d\n",getpid(),getppid());
				#endif

					/*Recursive cagri*/
					FindAndExecuteSubDirectories(newPath,target,sq+20);				
					exit(0);
					
				}
						
			}
		  

	}

	/*rewind islemi*/
	rewinddir(pathDir);

	/*Bu dongu ile path icersinde bulunan file lar icin thread ler olusturulur*/
	while ((pathDirent = readdir(pathDir)) != NULL)
	{
				
			strcpy(newPath,path);	
			
			fileNameSize=strlen(pathDirent->d_name);
			
			/*Eger alt dosyanin ismi . veya .. veya  uzantisi ~ ise  isleme alinmayacak*/
			/*Aksi durumda yeni path olusturulacak*/

			if(pathDirent->d_name[0]!='.' && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				strcat(newPath,"/");	
				strcat(newPath,pathDirent->d_name);
			}


		  
			/*Eger yeni olusturulan path file ise child olusturulduktan sonra dosya iceriginde arama thread tarafindan yapilacak*/
		   if (pathDirent->d_name[0]!='.' && !isDirectory(newPath) && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				strcpy(fileNames[filenameCounter],newPath);
				++filenameCounter;

					
				strcpy(AllDatas[counter].filenamePtr,newPath);
				strcpy(AllDatas[counter].target,target);

		
				messageQueueKey=ftok(newPath,1);

				allMsqids[counter]=msgget(messageQueueKey,0666 | IPC_CREAT);
					
				AllDatas[counter].msqid=allMsqids[counter];
		
				/*Thread Fonksiyonu*/
				pthread_create(&AllThreads[counter],NULL,grep,&AllDatas[counter]);													
								
					++NumberOfMsgQ;				
				++counter;
							
			}
	}
				
	

				/*PIPE lardan okuma yaptigim kisim parent file ile ilgilenen childlardan konum verilerini okur*/
	
		
		for(k=0;k<counter;++k)
		{		
				flag=0;
			do{
				/*Message queue dan okuma*/
				
				msgrcv(allMsqids[k],&l1,sizeof(Location),0,0);

				if(l1.first!=-1){
				
				if(flag==0){
		
					sprintf(temps,"%s\n%d %d*\n",fileNames[k],l1.first,l1.second);
					flag=1;
				}				
				else{
					sprintf(temps,"%d %d*\n",l1.first,l1.second);
					
				
				}
				
				/*Olusturulan konum bilgilerinden log dosyasinin icerigi olusturulur*/	 
				strcat(allLogs[k],temps);
				}

			}while(l1.first!=-1);	
	
			

				

		}

	
	/*Butun threadler join edilir ve message queue lar silinir*/

	int m=0;
	for(m=0;m<counter;++m)
	{
		
		pthread_join(AllThreads[m],NULL);
		msgctl(allMsqids[m],IPC_RMID, NULL);
	}

	

	/*sq degeri 0 olmasi icin en tepedeki process olmali bu blogu en tepedeki process yapar*/
	if(sq==0)
	{	

		char junk='a';
		int y,t=0;

		int numOfAllSubDirs=CountAllSubDirectories(path)-1;	
		resultFptr=fopen("gfDirShmMsgq_result.log","a");
		/*Alt directory sayisi kadar shared memoryden  okuma yapar ve dosyaya yazar*/

		/*OKUMA yapilacak kisim*/

		for(y=0;y<numOfAllSubDirs;++y)
		{
				
				for(;;)
				{
					sem_wait(secondSemaphore);
				
					if(*SharedMemory!='\0')
					{
						if(*SharedMemory!='*')
						fprintf(resultFptr,"%c",*SharedMemory);
						else
						++numberOfMatch;

						*SharedMemory='\a';
					}		
					
					else{
					
						*SharedMemory='\a';
						break;

					}
				}		
		
		
		}
			/*Pipelardan okudugu bilgileri ise bu kisimda dosyaya yazar*/

		for(t=0;t<numOfSub;++t)
		{	int y=0;
			while(allLogs[t][y]!='\0')
			{	if(allLogs[t][y]=='*')
				++numberOfMatch;
				++y;
			}
			fprintf(resultFptr,"%s\n",allLogs[t]);
		}

		fclose(resultFptr);

	}	

	/*En tepedeki process haricindeki  processler bu blogu yapar*/
	else if(sq!=0){
				
		char junkt='a';
		char nl='\n';
		char nu='\0';
		int h=0,t=0,c=0;
		int fd4,flag;
		int t1=0;	
			



		/*CRITICAL SECTION*/	

		sem_wait(mainSemaphore);

		for(h=0;h<numOfSub;++h)
		{
			t=0;
			do{
				if(allLogs[h][t]=='\0' &&  h+1!=numOfSub)
					*SharedMemory='\n';
				else

					*SharedMemory=allLogs[h][t];

				junkt=allLogs[h][t];

				sem_post(secondSemaphore);
				++t;

				while(*SharedMemory!='\a');

			}while(junkt!='\0');
			junkt='a';		
		
		}
		if(numOfSub==0){
		

		*SharedMemory='\0';
		sem_post(secondSemaphore);		
		
		while(*SharedMemory!='\a');
		
		}
		
		sem_post(mainSemaphore);	


		/*END OF CRITICAL SECTION*/

	}


	/*BUTUN COCUKLARIN OLMESI BEKLENIR*/
	while ((wpid = wait(&status)) > 0);

		
	closedir (pathDir);


}


/*Verilen path in directory olup olmadigini dondurecek olan fonksiyon*/
int isDirectory(const char *path) 
{
   struct stat statbuf;

   if (stat(path, &statbuf) == -1)
      return 0;

   else
      return S_ISDIR(statbuf.st_mode);
}


/*Girilen dosya ismiyle dosyayi acarak karakter sayisini hesaplar ve olusturulacak buffer buyuklugunu belirler*/

int calculateBufferSize(char* filenamePtr)
{
	int counter=0;

	char junk;

	FILE* filePtr;

	filePtr=fopen(filenamePtr,"r");

	if(filePtr==NULL)
	{
		perror("File Could'nt be opened!\n");
		exit(1);
	}

	while(fscanf(filePtr,"%c",&junk)!=EOF)	
		++counter;

	
	fclose(filePtr);

	return counter;
}


void* grep(void* param)
{
	
	Allparams p=*(Allparams*)param;
	char* filenamePtr=p.filenamePtr;
	const char* target=p.target;
	



	Location l2;
	l2.mtype=999;
	int flag;
	int i,counter=0,currentLineNumber=1,currentColumnNumber=1,widthOfTarget=-1;	

	int bufferSize;

	FILE* 	outputFilePtr;

	FILE*   inputFilePtr;

	char* buffer;

	#ifdef DEBUG
		fprintf(stderr,"pid:%d parentpid:%d\n",getpid(),getppid());
	#endif

	widthOfTarget=strlen(target);

	bufferSize=calculateBufferSize(filenamePtr);

	buffer=(char*)malloc(sizeof(char)*(bufferSize+3));

	outputFilePtr=fopen(OUTPUT_FILE_NAME,"a");

	inputFilePtr=fopen(filenamePtr,"r");

	if(inputFilePtr==NULL)
	{
		fprintf(stderr,"Dosya acilamadi:%s\n",filenamePtr);
		exit(0);
	}	

	for(i=0;i<bufferSize;++i)
	{
		fscanf(inputFilePtr,"%c",&buffer[i]);
	}
	
	
	fprintf(outputFilePtr,"\nFilePath:%s Target:%s\n\n",filenamePtr,target);
	
	for(i=0;i<bufferSize;++i)
	{
		/*Her eslesme icin konum verileri pipe ile yukariya yollanir*/		
		if(strncmp(target,&buffer[i],widthOfTarget)==0)
		{		

			l2.first=currentLineNumber;
			l2.second=currentColumnNumber;

			/*Eslesme konumlari struct icinde message queue ya yollanir*/

			msgsnd(p.msqid,&l2,sizeof(Location),0);

			fprintf(outputFilePtr,"%d.Occurance line: %d          column: %d\n",counter+1,currentLineNumber,currentColumnNumber);
			++counter;		
		}

		if(buffer[i]=='\n')
		{
	
			  ++currentLineNumber;
	   		  currentColumnNumber=1;

		}
			
		else
			++currentColumnNumber;

	}


	l2.first=-1;
	l2.second=-1;

			
	msgsnd(p.msqid,&l2,sizeof(Location),0);
		
	if(counter==0)
	{
		
		fprintf(outputFilePtr,"No content found!\n");
			
	}
	
	fprintf(outputFilePtr,"_____________________________________________\n");
	

	free(buffer);
	fclose(inputFilePtr);

	
	fclose(outputFilePtr);
	

}


int CountAllSubDirectories(const char* path)
{
	int counter=1;

	int fileNameSize;

	char newPath[255];

	DIR *pathDir;

	struct dirent *pathDirent;

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
				strcat(newPath,"/");	
				strcat(newPath,pathDirent->d_name);
			}

			
			if(isDirectory(newPath) && pathDirent->d_name[0]!='.' && pathDirent->d_name[fileNameSize-1]!='~')
			{
				counter+=CountAllSubDirectories(newPath);
			}
		  				
	}

	closedir (pathDir);
	return counter;
}
