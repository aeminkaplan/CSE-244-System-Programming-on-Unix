

/*
*
*			CSE 244 HW 01
*			grepfromFile.c
*		Ahmet Emin Kaplan 131044042
*
*/
										 
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define OUTPUT_FILE_NAME "gfF.log"

/*
Parametre olarak alinan dosyanin icindeki karakter sayisini return eder
*/
int calculateBufferSize(char* filenamePtr);

/*
Programin asil islevini yapan fonksiyondur
*/
void grep(char* filenamePtr,char* buffer,int bufferSize,char* target);


int main(int argc , char* argv[])
{


	char* buffer;

	int buffersize=0;

	if(3!=argc)
	{
		perror("Usage: -fileName -target\n");
		exit(1);
	}




	buffersize=calculateBufferSize(argv[1]);

	buffer=(char*)malloc(sizeof(char)*buffersize);

	grep(argv[1],buffer,buffersize,argv[2]);


	free(buffer);

	return 0;
}



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


void grep(char* filenamePtr,char* buffer,int bufferSize,char* target)
{

	int i,counter=0,currentLineNumber=1,currentColumnNumber=1,widthOfTarget=-1;	
	
	FILE* 	outputFilePtr;
	FILE*   inputFilePtr;

	widthOfTarget=strlen(target);

	outputFilePtr=fopen(OUTPUT_FILE_NAME,"a");
	inputFilePtr=fopen(filenamePtr,"r");

	for(i=0;i<bufferSize;++i)
	{
		fscanf(inputFilePtr,"%c",&buffer[i]);
	}
	
	fprintf(outputFilePtr,"\nFileName:%s Target:%s\n\n",filenamePtr,target);

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

	fclose(inputFilePtr);
	fclose(outputFilePtr);

}
