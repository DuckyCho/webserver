#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#define BUF_SIZE 1024
#define SMALL_BUF 100

void * request_handler(void * arg);
void send_data(FILE * fp, char * ct, char * file_name);
char * content_type(char * file);
void send_error(FILE * fp);
void send_error_400(FILE * fp);
void error_handling(char * message);


int main(int argc, char * argv[]){

	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_size;
	char buf[BUF_SIZE];
	pthread_t t_id;
	
	if(argc != 2){
		perror("No argument!");
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if(listen(serv_sock, 20) == -1)
		error_handling("listen() error");


	while(1){
		clnt_adr_size = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_size);
		printf("Connection Request : %s:%d\n",inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
		pthread_create(&t_id, NULL, request_handler, &clnt_sock);
		pthread_detach(t_id);
	}

	close(serv_sock);
	return 0;
}


void * request_handler(void * arg){
	int clnt_sock = *((int *)arg);
	printf("clntsock : %d\n",clnt_sock);
	char req_line[SMALL_BUF];
	FILE * clnt_read;
	FILE * clnt_write;

	char method[10];
	char ct[15];
	char file_name[30];
	memset(file_name,0,30);
	clnt_read = fdopen(clnt_sock, "r");
	clnt_write = fdopen(dup(clnt_sock), "w");
	fgets(req_line, SMALL_BUF, clnt_read);
	
	printf("req_line : %s\n",req_line);
	if(strstr(req_line, "HTTP/") == NULL){
		send_error_400(clnt_write);
		fclose(clnt_read);
		return;
	}

	strcpy(method, strtok(req_line, " /"));
	strcpy(file_name, strtok(NULL, " /"));
	strcpy(ct, content_type(file_name));
	if(strcmp(file_name,"HTTP")==0){
		strcpy(file_name,"index.html");
	}
	printf("method : %s!\n",method);
	printf("fileName : %s!\n",file_name);
	printf("contentType : %s!\n",ct);
	if(strcmp(method, "GET")!=0){
		printf("method user requested was not GET! error!");
		send_error(clnt_write);
		fclose(clnt_read);
		fclose(clnt_write);
		return;
	}
	

	fclose(clnt_read);
	send_data(clnt_write, ct, file_name);
}


void send_data(FILE * fp, char * ct, char * file_name){
	char protocol[]="HTTP/1.0 200 OK\r\n";
	char server[]="Server:Linux Web Server \r\n";
	char cnt_len[40];
	int file_size;
	char cnt_type[SMALL_BUF];
	unsigned char buf[BUF_SIZE];
	FILE * send_file;
	struct stat st;

	sprintf(cnt_type, "Content-type:%s\r\n\r\n",ct);
	send_file = fopen(file_name, "r");
	if(send_file == NULL){
		if(strcmp(file_name,"favicon.ico") == 0){
			printf("favicon request!! send 400 error!\n");
			send_error_400(fp);
			return;
		}
		else{
			printf("%s file open fail! send 404 page!\n",file_name);
			send_error(fp);
			return;
		}
		
	}

	fputs(protocol, fp);
	fputs(server, fp);
	stat(file_name, &st);
	sprintf(cnt_len,"Content-length:%d\r\n",st.st_size);
	fputs(cnt_len, fp);
	fputs(cnt_type, fp);
	
	if(strcmp(ct,"image/jpeg") == 0 || strcmp(ct,"image/png")==0){
		while( ( file_size = fread( buf, 1, BUF_SIZE, send_file) )> 0){
			fwrite(buf,1,BUF_SIZE,fp);
			fflush(fp);
		}
	}
	else{
		while( ( file_size = fread( buf, 1, BUF_SIZE-1, send_file) )> 0){
			sprintf(cnt_len,"Content-length:%d\r\n",file_size);
			fputs(buf, fp);
			fflush(fp);
		}
	}
	fflush(fp);
	fclose(fp);
}

char * content_type(char * file){
	char extension[SMALL_BUF];
	char file_name[SMALL_BUF];
	if(strcmp(file,"HTTP") == 0){
		strcpy(file_name,"index.html\0");
	}
	else{
		strcpy(file_name, file);
	}
	strtok(file_name, ".");
	strcpy(extension, strtok(NULL, "."));
	if(!strcmp(extension, "html") || !strcmp(extension, "htm"))
		return "text/html";
	else if(!strcmp(extension, "jpg") )
		return "image/jpeg";
	else if (!strcmp(extension, "png")) 
		return "image/png";
	else
		return "text/plain";
}

void send_error_400(FILE * fp){
	char protocol[]="HTTP/1.0 400 Bad Request\r\n";
	char server[]="Server:Linux Web Server\r\n";
	char cnt_len[] = "Content-length:2048\r\n";
	char cnt_type[]="Content-type:text/html\r\n\r\n";
	
	fputs(protocol, fp);
	fputs(server, fp);
	fputs(cnt_len, fp);
	fputs(cnt_type, fp);
	
	fflush(fp);
}

void send_error(FILE * fp){
	char protocol[]="HTTP/1.0 404 Not Found\r\n";
	char server[]="Server:Linux Web Server\r\n";
	char cnt_len[] = "Content-length:109\r\n";
	char cnt_type[]="Content-type:text/html\r\n\r\n";
	char buf[BUF_SIZE];
	FILE * errorPage = fopen("404.html","r");

	fputs(protocol, fp);
	fputs(server, fp);
	fputs(cnt_len, fp);
	fputs(cnt_type, fp);
	while(fgets(buf, BUF_SIZE, errorPage) != NULL){
		fputs(buf, fp);
		fflush(fp);
	}
	fflush(fp);
}

void error_handling(char * message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
