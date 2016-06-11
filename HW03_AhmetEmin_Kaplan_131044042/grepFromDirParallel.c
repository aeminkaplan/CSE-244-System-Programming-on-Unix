/*
*
*			CSE 244 HW 03
*			grepfromDirParallel.c
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



#define OUTPUT_FILE_NAME "gfDirParallel.log"
#define FIFO_PERM (S_IRUSR | S_IWUSR)
/*FONKSİYONLAR*/

static volatile sig_atomic_t doneflag = 0;
/* ARGSUSED */
static void setdoneflag(int signo) {
doneflag = 1;
}

/*Acilacak ve icinde arama yapilacak dosyalar icin gerekli buffer buyuklugunu hesaplayan fonksiyondur*/
int calculateBufferSize(char* filenamePtr);

/*Recursive olarak butuk alt klasorleri gezen fonksiyondur*/
void FindAndExecuteSubDirectories(const char* path,const char* target,const char* infifo,int sq);

/*Girilen path in directory olup olmadigini return eden fonksiyondur*/
int isDirectory(const char *path);

/*Girilen dosyada arama yapan fonksiyondur*/
int grep(char* filenamePtr,const char* target,int fd[],int fd2[]);



int main (int argc, char *argv[]) {





	if (argc!=3) {
    	
		fprintf (stderr,"Usage: testprog <dirname_or_path>   <target>\n");
        return 1;
   	}
	


	
	FindAndExecuteSubDirectories(argv[1],argv[2],NULL,0);
	


     return 0;
}


void FindAndExecuteSubDirectories(const char* path,const char* target,const char* outfifo,int sq)
{	

	struct sigaction act;
	act.sa_handler = setdoneflag;
	act.sa_flags = 0;
	
	if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1)) 
	{
		perror("Failed to set CTRL+C handler");
		exit(1);
	}
	
	/*Olusturulacak fifolar icin character arrayi*/
	char tempfifos2name[20][30];
	struct dirent *pathDirent;
	int konum[2];
	char newPath[255];
	DIR *pathDir;
	FILE *resultFptr;
	int fileNameSize;
	int temp,t,k;
	int result,status;
	pid_t childpid,wpid;
	int index,z,f=0,n=0,flag;
	int counter=0;

	/*Dosyalar icin kullanacagim pipe arrayi*/
	int allfds[100][2];
	int allfds2[100][2];

	int numOfSub=0,numOfDir=0,dirc=0;
	/*log dosyasini olusturmak icin pipelardan okudugum degerleri yazacagim chracter arrayi*/
	char allLogs[50][10000];
	char temps[300];
	char buffer[50][10000];
	/*Alt dosyalarin isimlerini tutacagim character arrayi*/
	char fileNames[50][255];
	int filenameCounter=0;
	for(z=0;z<50;++z)
	{
		fileNames[z][0]='\0';
		buffer[z][0]='\0';
	}
	/*fifolar isimlendirilir*/
	for(z=sq,f=0;z<sq+20;++z)
	{
		sprintf(tempfifos2name[f],"2fifo%d",z);
		++f;
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


	while ((pathDirent = readdir(pathDir)) != NULL && !doneflag )
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
				/*Alt directory icin fifo olusturuyorum ve olusturdugum fifonun ismi alt directorynin output fifosu olacak*/
				mkfifo(tempfifos2name[dirc],0600);

				childpid=fork();
				if(childpid==-1)
				{
					fprintf(stderr,"Fork Faillure!\n");
					exit(0);				
				}
				
				else if(childpid==0){
			
				#ifdef DEBUG
					fprintf(stderr,"pid:%d parentpid:%d\n",getpid(),getppid());
				#endif

					/*Recursive cagri*/
					FindAndExecuteSubDirectories(newPath,target,tempfifos2name[dirc],sq+20);				
					exit(0);
					
				}

				++dirc;			
			}
		  
			/*Eger yeni olusturulan path file ise child olusturulduktan sonra dosya iceriginde arama child tarafindan yapilacak*/
			else if (pathDirent->d_name[0]!='.' && !isDirectory(newPath) && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				strcpy(fileNames[filenameCounter],newPath);
				++filenameCounter;
				pipe(allfds[counter]);
				pipe(allfds2[counter]);	
				childpid=fork();

				if(childpid==-1)
				{
					fprintf(stderr,"Fork Faillure!\n");
					exit(0);				
				}

				else if(childpid==0){
					temp=grep(newPath,target,allfds[counter],allfds2[counter]);
					
					
					/*child isini yapar ve ölür */	
								
					exit(0);					
				}
				++counter;
			}
				
	}
	if(doneflag){
		for(k=0;k<20;++k)
			unlink(tempfifos2name[k]);		
			fprintf(stderr,"Signal yakalandi!\n");
			exit(1);
		}
				/*PIPE lardan okuma yaptigim kisim parent file ile ilgilenen childlardan konum verilerini okur*/
		if(counter!=0 && childpid!=0){
		for(k=0;k<counter;++k)
		{
				flag=0;
			do{
							
				read(allfds[k][0],konum,sizeof(int)*2);
				write(allfds2[k][1],konum,sizeof(int));
				if(konum[0]!=-1){
				if(flag==0){
					sprintf(temps,"%s\n%d %d\n",fileNames[k],konum[0],konum[1]);
					flag=1;
				}				
				else
					sprintf(temps,"%d %d\n",konum[0],konum[1]);
								/*Olusturulan konum bilgilerinden log dosyasinin icerigi olusturulur*/	 
				strcat(allLogs[k],temps);
				}
			}while(konum[0]!=-1);	
		}
		
			for(k=0;k<counter;++k)
			{
				close(allfds[k][0]);
				close(allfds[k][1]);
				close(allfds2[k][0]);
				close(allfds2[k][1]);
			}	
	}
	/*sq degeri 0 olmasi icin en tepedeki process olmali bu blogu en tepedeki process yapar*/
	if(numOfDir!=0 && sq==0)
	{	

		char junk='a';
		int y,t=0;
		int fd1;	
		resultFptr=fopen("gfDirParallel_result.log","a");
		/*Alt directory sayisi kadar fifo olacagi icin fifolardan okuma yapar ve dosyaya yazar*/
		for(y=0;y<numOfDir;++y)
		{
		
					fd1=open(tempfifos2name[y],O_RDONLY);

				do{
						if(read(fd1,&junk,sizeof(char)>0)){
						if(junk!='\0'){
						fprintf(resultFptr,"%c",junk);
						//fprintf(stderr,"%c",junk);
						}
					}				
				}
				while(junk!='\0');


				junk='a';
				close(fd1);
				unlink(tempfifos2name[y]);
		
		}
			/*Pipelardan okudugu bilgileri ise bu kisimda dosyaya yazar*/
				for(t=0;t<numOfSub;++t)
				{
					fprintf(resultFptr,"%s\n",allLogs[t]);
				}

		fclose(resultFptr);
	}	
	/*En tepedeki process haricindeki parent processler bu blogu yapar*/
	else if(numOfDir!=0 && sq!=0){
				
		char junkt='a';
		char nl='\n';
		char nu='\0';
		int h=0,t=0,c=0;
		int fd3,fd4,flag;
		int t1=0;	
			
		/*Oncelikle alt directorylerin fifoya yazdiklarini buffera kaydeder*/
		for(h=0;h<numOfDir;++h)
		{			
			fd4=open(tempfifos2name[h],O_RDONLY);

		
			t=0;
			do{

				if(read(fd4,&buffer[h][t],sizeof(char))>0){
				
				junkt=buffer[h][t];
								
				++t;
				}
				else
					junkt='a';

			}while(junkt!='\0');
			junkt='a';		
			close(fd4);
			unlink(tempfifos2name[h]);		
		}


		
		/*Pipedan okudugu verilerle bufferi birlestirir*/
		for(t1=0;t1<numOfSub;++t1)
		{
			strcpy(buffer[numOfDir+t1],allLogs[t1]);

		}
		fd3=open(outfifo,O_WRONLY);
			
		/*Butun verileri ustteki processin okumasi icin output fifosuna yazar*/
		for(h=0;h<numOfDir+numOfSub;++h)
		{	t=0;
			do{
				if(buffer[h][t]=='\0' &&  h+1!=(numOfSub+numOfDir))
					write(fd3,&nl,sizeof(char));
				else
					write(fd3,&buffer[h][t],sizeof(char));

				junkt=buffer[h][t];

				
				++t;
			
			}while(junkt!='\0');
			junkt='a';		
		
		}
		
		write(fd3,&nu,sizeof(char));
		close(fd3);
	}
	/*Bu blogu parant olupta directory child a sahip olmayan processler yapar*/
	else if(numOfDir==0)
	{
		/*pipedan okuyup alllogs bufferina kaydettigi verileri output fifosuna yazar*/
		char junkt='a';
		int h,t,fd8;
		char nl='\n';
		fd8=open(outfifo,O_WRONLY);
		for(h=0;h<numOfSub;++h)
		{
			t=0;
			do{
				if(allLogs[h][t]=='\0' && h<numOfSub && h+1!=numOfSub){

					write(fd8,&nl,sizeof(char));
									
				}
				else
					write(fd8,&allLogs[h][t],sizeof(char));
				
				junkt=allLogs[h][t];
				++t;
			
			}while(junkt!='\0');
			junkt='a';		
		}
	

		write(fd8,&nl,sizeof(char));
		close(fd8);

	}

	/*BUTUN COCUKLARIN OLMESI BEKLENIR*/
	while ((wpid = wait(&status)) > 0);

	if(childpid!=0)	
	{		
			closedir (pathDir);
	}       

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


int grep(char* filenamePtr,const char* target,int fd[],int fd2[])
{

	int konum[2]={-1,-1};
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

	buffer=(char*)malloc(sizeof(char)*bufferSize);

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
	
	if(!doneflag)
		fprintf(outputFilePtr,"FilePath:%s Target:%s\n\n",filenamePtr,target);
	
	for(i=0;i<bufferSize && !doneflag;++i)
	{
		
		if(strncmp(target,&buffer[i],widthOfTarget)==0 && !doneflag )
		{		
			
			konum[0]=currentLineNumber;
			konum[1]=currentColumnNumber;
			write(fd[1],konum,sizeof(int)*2);
			read(fd2[0],&flag,sizeof(int));
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

	konum[0]=-1;
	konum[1]=-1;
	write(fd[1],konum,sizeof(int)*2);
	if(counter==0 && !doneflag)
	{
		fprintf(outputFilePtr,"No content found!\n");
			
	}
	if(!doneflag)
	fprintf(outputFilePtr,"_____________________________________________\n");
	

	free(buffer);
	fclose(inputFilePtr);

	close(fd[0]);
	close(fd[1]);
	close(fd2[0]);
	close(fd2[1]);
	if(doneflag)
	{	
		fprintf(outputFilePtr,"CTRL+C Signal Yakalandi!\n");
			fclose(outputFilePtr);
	
		return -1;
	}
	return counter;
}
