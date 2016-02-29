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

char *PROGNAME;

int filter (char *filename,char *pattern,char *command[]){
	int file, fd[2],status;
	char path[512];

	switch(fork()){
	case -1:
		fprintf(stderr, "%s: fork failed\n",PROGNAME);
		exit(1);
	case 0:		//hijo 1 se encarga de filtrar la salida estandar
		pipe(fd);
		switch(fork()){
		case -1:
			fprintf(stderr, "%s: fork failed\n",PROGNAME);
			exit(1);
		case 0:		//hijo 2 se encarga de ejecutar el comando
			close(fd[0]);	//cierro el lado de lectura
			snprintf(path,sizeof(path),"/usr/bin/%s",command[0]);	//creo el path del comando
			file=open(filename,O_RDONLY);
			if(file<0)
				err(1,"No se puede abrir el fichero %s",filename);

			dup2(file,0);
			dup2(fd[1],1);
			execv(path,command);
			err(1,"exec %s failed",command[0]);
		default:
			wait(&status);
			if(status!=0)
				exit(EXIT_FAILURE);

			close(fd[1]);	//cierro el lado de escritura
			dup2(fd[0],0);
			execl("/bin/grep","grep",pattern,NULL);
			err(1,"exec grep failed");
		}
	default:
		wait(&status);
		return status;
	}
}

int dirprocessor(char *dirPath,char *pattern,char *command[]){
	DIR *dir;
	struct  dirent *dr;
	struct stat st;
	int err_count=0;

	dir=opendir(dirPath);
	if(dir==NULL)
		err(1,"No se puede abrir el directorio");

	while((dr=readdir(dir))!=NULL){
		stat(dr->d_name,&st);
		if ((st.st_mode & S_IFMT) == S_IFREG){
			err_count += filter(dr->d_name,pattern,command);
		}
	}
	closedir(dir);
	return err_count;
}

int
main(int argc,char *argv[]){
	char *pattern=argv[1];
	char *command[10];
	int i,err_count=0;

	PROGNAME=argv[0];
	if(argc<3){
		fprintf(stderr, "%s: Usage error:[regular expresion] [command] [options]\n",PROGNAME);
		exit(EXIT_FAILURE);
	}

	for(i=2;i<argc;i++){
		command[i-2]=argv[i];
	}
	command[i-2]=NULL;
	err_count=dirprocessor(".",pattern,command);

	if(err_count){
		exit(EXIT_FAILURE);
	}else{
		exit(EXIT_SUCCESS);
	}
}
