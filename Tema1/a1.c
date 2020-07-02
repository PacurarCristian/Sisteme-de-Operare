#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#define MAX 512

struct section_header{
	char name[18];
	int type;
	int offset;
	int size;
};

typedef struct section_header section_header;

void octal_to_char(int nr, char *char_perm, int index){
	switch(nr){
		case 0: 
			char_perm[index] = '-';
			char_perm[index + 1] = '-';
			char_perm[index + 2] = '-';
			break;
		case 1: 
			char_perm[index] = '-';
			char_perm[index + 1] = '-';
			char_perm[index + 2] = 'x';
			break;
		case 2: 
			char_perm[index] = '-';
			char_perm[index + 1] = 'w';
			char_perm[index + 2] = '-';
			break;
		case 3: 
			char_perm[index] = '-';
			char_perm[index + 1] = 'w';
			char_perm[index + 2] = 'x';
			break;
		case 4: 
			char_perm[index] = 'r';
			char_perm[index + 1] = '-';
			char_perm[index + 2] = '-';
			break;
		case 5: 
			char_perm[index] = 'r';
			char_perm[index + 1] = '-';
			char_perm[index + 2] = 'x';
			break;
		case 6: 
			char_perm[index] = 'r';
			char_perm[index + 1] = 'w';
			char_perm[index + 2] = '-';
			break;
		case 7: 
			char_perm[index] = 'r';
			char_perm[index + 1] = 'w';
			char_perm[index + 2] = 'x';
			break;
	}
}

off_t list(const char* dirPath, int size_greater, char *permissions, int recursiv){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;
    dir = opendir(dirPath);

	int octal_perm = 0;
	char char_perm[10] = {'\0'};

    if(dir == NULL) {
		printf("ERROR\n");
		printf("invalid directory path\n");
		exit(-1);
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {

            snprintf(fullPath, 512, "%s/%s", dirPath, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0) {

				octal_perm = statbuf.st_mode & 0777;
				for(int i=0; i<9; i=i+3){
					if(i == 0){
						octal_to_char(octal_perm/8/8, char_perm, i);
					}
					else if(i == 3){
						octal_to_char(octal_perm/8%8, char_perm, i);
					}
					else if(i == 6){
						octal_to_char(octal_perm%8, char_perm, i);
					}
					
				}
				char_perm[9] = '\0';

				if(strcmp(permissions, "\0") == 0){
					if(S_ISREG(statbuf.st_mode) && statbuf.st_size > size_greater){
						printf("%s\n", fullPath);
					}
					else if(size_greater == -1){
						printf("%s\n", fullPath);

						if(S_ISDIR(statbuf.st_mode) && recursiv == 1){
							list(fullPath, size_greater, permissions, recursiv);
						}
					}
				}
				else if(strcmp(permissions, "\0") != 0){
					if(S_ISREG(statbuf.st_mode) && statbuf.st_size > size_greater && strcmp(char_perm, permissions) == 0){
						printf("%s\n", fullPath);
					}
					else if(strcmp(char_perm, permissions) == 0 && size_greater == -1){
						printf("%s\n", fullPath);

						if(S_ISDIR(statbuf.st_mode) && recursiv == 1){
							list(fullPath, size_greater, permissions, recursiv);
						}
					}
				}
            }
        }
    }

    closedir(dir);
	return 0;
}

const char * get_string(char *argv){
	int egal = 0, index = 0;
	static char value[512];

	for(int i=0; i<strlen(argv); i++){
		if(egal == 1){
			value[index++] = argv[i];
		}
		else if(argv[i] == '='){
			egal = 1;
		}
	}

	value[index] = '\0';
	
	return value;
}

section_header * parse(const char* path, int *nr_sections, int *corect, int afisare){
	int header_size = 0, version = 0;
    int fd = 0, i, j;
    char c, magic[3];
    struct section_header *sh;
    
    *corect = 0;
    fd = open(path, O_RDONLY);

    if(fd == -1) {
		printf("ERROR\n");
		printf("invalid file path\n");
		exit(-1);
    }
    
    lseek(fd, 0, SEEK_END);
    lseek(fd, -4, SEEK_CUR);
    read(fd, &header_size, 2);
    
    read(fd, &magic[0], 1);
    read(fd, &magic[1], 1);
    magic[2] = '\0';
    
    if(strcmp(magic, "wF") != 0){
    	if(afisare == 1){
    		printf("ERROR\n");
    		printf("wrong magic\n");
    	}
    	
    	return NULL;
    }
    
    lseek(fd, -header_size, SEEK_END);
    read(fd, &version, 1);
    
    if(version < 98 || version > 142){
    	if(afisare == 1){
    		printf("ERROR\n");
    		printf("wrong version\n");
    	}
    	
    	return NULL;
    }
    
    read(fd, nr_sections, 1);
    
    if(*nr_sections < 5 || *nr_sections > 14){
    	if(afisare == 1){
    		printf("ERROR\n");
    		printf("wrong sect_nr\n");
    	}
    	
    	return NULL;
    }
    
    sh = (struct section_header*)malloc(*nr_sections*sizeof(struct section_header));
    
    for(i=0; i<*nr_sections; i++){
    	for(j=0; j<17; j++){
    		read(fd, &c, 1);
    		sh[i].name[j] = c;
    	}
    	
    	sh[i].name[j] = '\0';
    	read(fd, &sh[i].type, 1);
    	read(fd, &sh[i].offset, 4);
    	read(fd, &sh[i].size, 4);
    	
    	if(sh[i].type != 67 && sh[i].type != 47){
    		if(afisare == 1){
				printf("ERROR\n");
				printf("wrong sect_types\n");
    		}
    		
			free(sh);
			return NULL;
    	}
    }
    
    *corect = 1;
    if(afisare == 1){
		printf("SUCCESS\n");
		printf("version=%d\n", version);
		printf("nr_sections=%d\n", *nr_sections);
		
		for(i=0; i<*nr_sections; i++){
			printf("section%d: %s %d %d\n", i+1, sh[i].name, sh[i].type, sh[i].size);
		}
		
		free(sh);
	}
    
    close(fd);
    return sh;
}

int extract(const char* path, int offset, int size, int nr_linie){
	int fd = 0, linie_noua = 0, index_size = 0, nr_linii = 1;
	char c, c2, line[MAX] = {'\0'};
	int i = 0, j = 0;
    
    fd = open(path, O_RDONLY);

    if(fd == -1) {
		printf("ERROR\n");
		printf("invalid file\n");
		exit(-1);
    }
    
    lseek(fd, offset, SEEK_SET);
    
    while(index_size <= size){
    	while(linie_noua != 1 && index_size <= size){
    		read(fd, &c, 1);
    		index_size++;
    		
    		if((int)c == 13){
    			read(fd, &c2, 1);
    			index_size++;
    			
    			if((int)c2 == 10){
    				line[i] = '\0';
    				linie_noua = 1;
    			}
    			else{
    				line[i++] = c;
    				line[i++] = c2;
    			}
    		}
    		else{
    			line[i++] = c;
    		}
    		
    		if(i > MAX-1){
    			//memorie depasita
    			return -1;
    		}
    	}
    	
    	line[i] = '\0';
    	i = 0;
    	linie_noua = 0;
    	
    	if(nr_linii == nr_linie){
    		printf("SUCCESS\n");
    		
    		for(j=strlen(line)-1; j>=0; j--){
    			printf("%c", line[j]);
    		}
    		
    		printf("\n");
    		break;
    	}
    	else{
    		nr_linii++;
    	}
    }
    
    if(nr_linii < nr_linie){
    	printf("ERROR\n");
    	printf("invalid line\n");
    }
    
    close(fd);
    return 0;
}

off_t findall(const char* path){
	DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;
    int nr_sections = 0, corect = 0;
    section_header *sh;
    
    dir = opendir(path);

    if(dir == NULL) {
		printf("ERROR\n");
		printf("invalid directory path\n");
		exit(-1);
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {

            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0) {

				if(S_ISREG(statbuf.st_mode)){
					sh = parse(fullPath, &nr_sections, &corect, 0);
					
					if(corect == 1){
						for(int i=0; i<nr_sections; i++){
							if(sh[i].size > 1190){
								corect  = 0;
								break;
							}
						}
					}
					
					if(corect == 1){
						printf("%s\n", fullPath);
					}
					
					free(sh);
				}
				else if(S_ISDIR(statbuf.st_mode)){
					findall(fullPath);
				}
            }
        }
    }

    closedir(dir);
    return 0;
}

int main(int argc, char **argv){
	int size_greater = -1, recursiv = 0, nr_sections = 0, corect = 0;
	char permissions[10] = {'\0'};
	char path[512] = {'\0'};

    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("33846\n");
        }
		else if(strcmp(argv[1], "list") == 0){
			for(int i=2; i<argc; i++){
				if(strstr(argv[i], "size_greater=") != NULL){
					sscanf(get_string(argv[i]), "%d", &size_greater);
				}
				else if(strstr(argv[i], "permissions=") != NULL){
					strncpy(permissions, get_string(argv[i]), 10);
				}
				else if(strstr(argv[i], "path=") != NULL){
					strncpy(path, get_string(argv[i]), 512);
				}
				else if(strstr(argv[i], "recursive") != NULL){
					recursiv = 1;
				}
			}
			
			printf("SUCCESS\n");
			list(path, size_greater, permissions, recursiv);
		}
		else if(strcmp(argv[1], "parse") == 0){
			strncpy(path, get_string(argv[2]), 512);
			parse(path, &nr_sections, &corect, 1);
		}
		else if(strcmp(argv[1], "extract") == 0){
			int section = 0, line = 0;
			section_header *sh;
			
			for(int i=2; i<argc; i++){
				if(strstr(argv[i], "section=") != NULL){
					sscanf(get_string(argv[i]), "%d", &section);
				}
				else if(strstr(argv[i], "line=") != NULL){
					sscanf(get_string(argv[i]), "%d", &line);
				}
				else if(strstr(argv[i], "path=") != NULL){
					strncpy(path, get_string(argv[i]), 512);
				}
			}
			
			sh = parse(path, &nr_sections, &corect, 0);
			if(section < 1 || section > nr_sections){
				printf("ERROR\n");
				printf("invalid section\n");
			}
			else{
				extract(path, sh[section-1].offset, sh[section-1].size, line);
			}
			
			free(sh);
		}
		else if(strcmp(argv[1], "findall") == 0){
			strncpy(path, get_string(argv[2]), 512);
			printf("SUCCESS\n");
			findall(path);
		}
    }
	
    return 0;
}
