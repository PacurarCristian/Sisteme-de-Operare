#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define pipe1 "RESP_PIPE_33846"
#define pipe2 "REQ_PIPE_33846"
#define SHM_KEY 17500

int main(int argc, char **argv){

	int fd, fd2;
	char c;
	
	if(mkfifo(pipe1, 0600) != 0){
		printf("ERROR\n");
		printf("cannot create the response pipe\n");
		return 1;
	}

	fd = open(pipe2, O_RDONLY);
	
	if(fd == -1) {
		printf("ERROR\n");
		printf("cannot open the request pipe\n");
		return 1;
	}
	
	fd2 = open(pipe1, O_WRONLY);
	
	if(fd2 == -1) {
		printf("ERROR\n");
		printf("cannot open the response pipe\n");
		return 1;
	}
	
	c = 7;
	write(fd2, &c, 1);
	write(fd2, "CONNECT", 7);
	printf("SUCCESS\n");

	int size = 0, i;
	char request[30];
	unsigned int val = 0, offset = 0, val2 = 0;
	
	int shmId;
	int *sharedInt = NULL;
	
	while(read(fd, &size, 1)){
		for(i=0; i<size; i++){
			read(fd, &request[i], 1);
		}
		
		request[i] = '\0';
		
		if(strcmp("PING\0", request) == 0){
			c = 4;
			write(fd2, &c, 1);
			write(fd2, "PING", 4);
			
			write(fd2, &c, 1);
			write(fd2, "PONG", 4);
			
			val = 33846;
			write(fd2, &val, sizeof(unsigned int));
		}
		else if(strcmp("CREATE_SHM\0", request) == 0){
			read(fd, &val, 4);

			shmId = shmget(SHM_KEY, val, IPC_CREAT | 0664);
			if(shmId < 0) {
				c = 10;
				write(fd2, &c, 1);
				write(fd2, "CREATE_SHM", 10);
				
				c = 5;
				write(fd2, &c, 1);
				write(fd2, "ERROR", 5);
				
				return 1;
			}
			
			sharedInt = (int*)shmat(shmId, NULL, 0);
			if(sharedInt == (void*)-1){
				perror("Could not attach to shm");
				return 1;
			}
			
			c = 10;
			write(fd2, &c, 1);
			write(fd2, "CREATE_SHM", 10);
				
			c = 7;
			write(fd2, &c, 1);
			write(fd2, "SUCCESS", 7);
		}
		else if(strcmp("WRITE_TO_SHM\0", request) == 0){
			read(fd, &offset, 4);
			read(fd, &val2, 4);
			
			if(offset >= 0 && offset <= val && (offset + sizeof(unsigned int)) <= val){
				sharedInt[offset/4] = val2;
				
				c = 12;
				write(fd2, &c, 1);
				write(fd2, "WRITE_TO_SHM", 12);
				
				c = 7;
				write(fd2, &c, 1);
				write(fd2, "SUCCESS", 7);
			}
			else{
				c = 12;
				write(fd2, &c, 1);
				write(fd2, "WRITE_TO_SHM", 12);
				
				c = 5;
				write(fd2, &c, 1);
				write(fd2, "ERROR", 5);
			}
		}
		else if(strcmp("MAP_FILE\0", request) == 0){
			int fd3;
			off_t sizef;
			char *data = NULL;
			char fisier[30];
			
			size = 0;
			read(fd, &size, 1);
			read(fd, &fisier, size);
			fisier[size] = '\0';

			fd3 = open(fisier, O_RDONLY);
			if(fd3 == -1) {
				//perror("Could not open input file");
				c = 8;
				write(fd2, &c, 1);
				write(fd2, "MAP_FILE", 8);
				
				c = 5;
				write(fd2, &c, 1);
				write(fd2, "ERROR", 5);
				return 1;
			}
			
			sizef = lseek(fd3, 0, SEEK_END);
			lseek(fd3, 0, SEEK_SET);
			data = (char*)mmap(NULL, sizef, PROT_READ, MAP_PRIVATE, fd3, 0);
			
			if(data == (void*)-1) {
				c = 8;
				write(fd2, &c, 1);
				write(fd2, "MAP_FILE", 8);
				
				c = 5;
				write(fd2, &c, 1);
				write(fd2, "ERROR", 5);
				
				close(fd3);
				return 1;
			}
			
			c = 8;
			write(fd2, &c, 1);
			write(fd2, "MAP_FILE", 8);
				
			c = 7;
			write(fd2, &c, 1);
			write(fd2, "SUCCESS", 7);
			
			close(fd3);
		}
		else { //if(strcmp("EXIT\0", request) == 0){
			close(fd);
			close(fd2);
			unlink(pipe1);
			break;
		}
	}
	
	shmdt(sharedInt);
	shmctl(shmId, IPC_RMID, 0);
	
	return 0;
}
