/*
*
*			CSE 244 HW 02
*			grepfromDirectory.c
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

#define DEBUG
#define OUTPUT_FILE_NAME "gfD.log"

/*FONKSİYONLAR*/

/*Acilacak ve icinde arama yapilacak dosyalar icin gerekli buffer buyuklugunu hesaplayan fonksiyondur*/
int calculateBufferSize(char* filenamePtr);

/*Recursive olarak butuk alt klasorleri gezen fonksiyondur*/
void FindAndExecuteSubDirectories(const char* path,const char* target);

/*Girilen path in directory olup olmadigini return eden fonksiyondur*/
int isDirectory(const char *path);

/*Girilen dosyada arama yapan fonksiyondur*/
int grep(char* filenamePtr,const char* target);



int main (int argc, char *argv[]) {

    int numberOfContent=0;

	FILE* resultFilePtr;    
	

	if (argc!=3) {
    	
		fprintf (stderr,"Usage: testprog <dirname_or_path>   <target>\n");
        return 1;
   	}

	
	FindAndExecuteSubDirectories(argv[1],argv[2]);
	
	/*Toplam bulunan eslesme sayisini okuyorum ve ekrana yazdiriyorum*/
  	resultFilePtr=fopen("131044042_result.txt","r");
	
	fscanf(resultFilePtr,"%d",&numberOfContent);

	fprintf(stderr,"Number of content:%d\n",numberOfContent);
	
	fclose(resultFilePtr);

	/*Butun child larin bulduklari eslesmeleri yazdigi dosyayi siliyorum*/
	remove("131044042_result.txt");

	remove("131044042_result.txt~");

     return 0;
}


void FindAndExecuteSubDirectories(const char* path,const char* target)
{

	struct dirent *pathDirent;

	char newPath[255];

	DIR *pathDir;
	FILE *resultFptr;
	int fileNameSize;
	int temp;
	int result;
	pid_t childpid;

	pathDir = opendir (path);

	strcpy(newPath,path);
	
    if (pathDir == NULL) 
	{
            fprintf (stderr,"Cannot open directory '%s'\n", path);
            exit(0);
    }

	/*  Path icindeki butun dosyalarin tek tek isleme alinma kismi  */
    
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
				
				else if(childpid==0){
			
				#ifdef DEBUG
					fprintf(stderr,"pid:%d parentpid:%d\n",getpid(),getppid());
				#endif

					/*Recursive cagri*/
					FindAndExecuteSubDirectories(newPath,target);				
					exit(0);
					
				}
				else
					wait(NULL);	
			}
		  
			/*Eger yeni olusturulan path file ise child olusturulduktan sonra dosya iceriginde arama child tarafindan yapilacak*/
			else if (pathDirent->d_name[0]!='.' && !isDirectory(newPath) && pathDirent->d_name[fileNameSize-1]!='~')
			{	
				childpid=fork();

				if(childpid==-1)
				{
					fprintf(stderr,"Fork Faillure!\n");
					exit(0);				
				}

				else if(childpid==0){

					temp=grep(newPath,target);
					
					resultFptr=fopen("131044042_result.txt","r");
				
					/*	Bulunan eslesme miktari 131044042_result.txt dosyasina yazilacak 
							daha dogrusu her process buldugu miktari ekleyecek
					*/

					
				/*Eger dosya "r" modunda acilabilmisse icerigi okunarak eslesme miktari icerige eklenerek yazilir*/
					if(resultFptr!=NULL)
					{
						fscanf(resultFptr,"%d",&result);
						result+=temp;
						fclose(resultFptr);
						resultFptr=fopen("131044042_result.txt","w");
						fprintf(resultFptr,"%d",result);
						fclose(resultFptr);					
					}
					

					/*Eger 131044042_result.txt dosyasi "r" modunda acilamamissi dosya yoktur bu durumda "w"  modunda acilir
						bulunan eslesme miktari yazilir		*/
					
				else{
						resultFptr=fopen("131044042_result.txt","w");
						fprintf(resultFptr,"%d",temp);
						fclose(resultFptr);
					}
					exit(0);					
				}
				else
					wait(NULL);
			}
			
	}
   
	
  /*Parent process directory yi kapatacak*/
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


int grep(char* filenamePtr,const char* target)
{


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
	
	fprintf(outputFilePtr,"FilePath:%s Target:%s\n\n",filenamePtr,target);

	for(i=0;i<bufferSize;++i)
	{

		if(strncmp(target,&buffer[i],widthOfTarget)==0)
		{
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

	if(counter==0)
	{
		fprintf(outputFilePtr,"No content found!\n");
			
	}

	fprintf(outputFilePtr,"_____________________________________________\n");
	free(buffer);
	fclose(inputFilePtr);
	fclose(outputFilePtr);
	return counter;
}
