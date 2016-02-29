#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <err.h>

enum{MAX_FILES=3};
char *progName;

int notnumber(char *str){
	int i;

	for(i=0;i<strlen(str);i++){
		if(!isdigit(str[i]))
			return 1;
	}
	return 0;
}

int fileprocessor(char *fileName,int offset){
	int err_count=0,file_src,file_dst,nr,nw;
	char buffer[1024]="", new_name[512];

	snprintf(new_name,sizeof(new_name),"%s.out",fileName);
	file_src=open(fileName,O_RDONLY);	//fichero de origen.
	if(file_src<0){
		printf("\n");
		warn("No se puede abrir el fichero %s ",fileName);
		return ++err_count;
	}
	file_dst=creat(new_name,0755);	//fichero de destino.
	if(file_dst<0){
		printf("\n");
		warn("No se puede abrir el fichero %s ",new_name);
		return ++err_count;
	}

	if((lseek(file_src,-offset,SEEK_END)) < 0 || offset == 0)
		lseek(file_src,0,SEEK_SET);

	while((nr=read(file_src,buffer,sizeof(buffer)))!=0){
		if(nr<0){
			printf("\n");
			warn("Error de lectura en el fichero %s ",fileName);
			close(file_src);
			return ++err_count;
		}
		nw=write(file_dst,buffer,sizeof(buffer));
		if(nw<0){
			printf("\n");
			warn("Error de escritura en el fichero %s ",new_name);
			close(file_dst);
			return ++err_count;
		}
	}
	close(file_src);
	close(file_dst);
	return err_count;
}

int takefiles(DIR *dir,char *files[]){
	struct  dirent *dr;
	struct stat st;
	char *str;
	int filescounter=0;

	while((dr=readdir(dir))!=NULL){
		stat(dr->d_name,&st);
		if ((st.st_mode & S_IFMT) == S_IFREG){
			str=strrchr(dr->d_name,'.');
			if(str && strcmp(str,".txt")==0){
				if(filescounter<MAX_FILES){
					files[filescounter] = dr->d_name;
					++filescounter;
				}else{
					fprintf(stderr, "%s: Se ha excedido el numero máximo de ficheros\n",progName);
					exit(1);
					closedir(dir);
				}
			}
		}
	}
	return filescounter;
}

int dirprocessor(char *dirPath,int offset){
	DIR *dir;
	char *files[MAX_FILES];
	int err_count=0,filescounter=0,i,status;

	dir=opendir(dirPath);
	if(dir==NULL)
		err(0,"No se puede abrir el directorio");

	filescounter=takefiles(dir,files);

	for(i=0;i<filescounter;i++){
		switch(fork()){
		case -1:
			fprintf(stderr, "%s: fork failed\n",progName);
			exit(1);
		case 0:	//child
			err_count += fileprocessor(files[i],offset);
			if(err_count){
				exit(EXIT_FAILURE);
			}else{
				exit(EXIT_SUCCESS);
			}
		}
	}
	for(i=0;i<filescounter;i++){
		wait(&status);
		err_count += status;
	}
	closedir(dir);
	return err_count;
}

int main(int argc,char* argv[]){

	int offset=0,err_count=0;
	progName=argv[0];

	if(argc > 2){
		fprintf(stderr, "%s: Usage error:[number of bytes to read from the tail]\n",progName);
		exit(1);
	}else if(argc == 2){
		if(notnumber(argv[1])){
			fprintf(stderr, "%s: Usage error:[number of bytes to read from the tail]\n",progName);
			exit(1);
		}
		offset=atoi(argv[1]);
	}
	
	err_count += dirprocessor(".",offset);

	if(err_count){
		exit(EXIT_FAILURE);
	}else{
		exit(EXIT_SUCCESS);
	}
}