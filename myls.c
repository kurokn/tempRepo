#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <glob.h>
#define N 512

typedef struct cmd_param{
    int i;
    int R;
    int l;
}Param;

Param global_param;

char** read_all_filesnames(const char* path){
    DIR* dir;
    struct dirent* dp;
    struct stat buf;
    char base_path[N]={0};
    char** file_paths = NULL;
    int len = 0, i;

    if ((dir=opendir(path)) == NULL){
        perror(path);
        return NULL;
    }

    while((dp=readdir(dir))!=NULL){
        len++;
    }
    file_paths = (char**)malloc(sizeof(char*) * (len+1));
    memset(file_paths, 0, len+1);
    i = 0;
    rewinddir(dir);
    
    while((dp=readdir(dir))!=NULL){
        memset(base_path, 0, N);
        
        strcat(base_path, dp->d_name);
        file_paths[i] = strdup(base_path);
        i++;
    }
    closedir(dir);
    file_paths[len] = 0;
    return file_paths;
}

void free_array(char** arr){
    int i=0;
    while(arr[i]){
        free(arr[i]);
        i++;
    }
    free(arr);
}


void sort_files(char** files){
    int i, j;
    int len = 0;
    char * t;
    while(files[len])len++;
    for(i=0;i<len;i++){
        for(j=0;j<len-i-1;j++){
            if(strcmp(files[j], files[j+1])>0){
                t = files[j];
                files[j] = files[j+1];
                files[j+1] = t;
            }
        }
    }
}

char* filename(const char* path){
    int i,j;
    i = j = 0;
    for(j=0;path[j];j++){
        if(path[j]=='/'){
            i=j+1;
        }
    }
    char* name = (char*)malloc(sizeof(char) * (j-i));
    strcpy(name, path+i);
    name[j-i] = 0;
    return name;
}

void getmod(mode_t mode,char *line){
    memset(line,0,sizeof(char)*11);
    strcat(line,S_ISDIR(mode)?"d":"-");
    strcat(line,(mode&S_IRWXU)&S_IRUSR?"r":"-");
    strcat(line,(mode&S_IRWXU)&S_IWUSR?"w":"-");
    strcat(line,(mode&S_IRWXU)&S_IXUSR?"x":"-");
    strcat(line,(mode&S_IRWXG)&S_IRGRP?"r":"-");
    strcat(line,(mode&S_IRWXG)&S_IWGRP?"w":"-");
    strcat(line,(mode&S_IRWXG)&S_IXGRP?"x":"-");
    strcat(line,(mode&S_IRWXO)&S_IROTH?"r":"-");
    strcat(line,(mode&S_IRWXO)&S_IWOTH?"w":"-");
    strcat(line,(mode&S_IRWXO)&S_IXOTH?"x":"-");
}

void printBuf(struct stat* buf,char* user_name, char* group_name,char* szBuf,char* name, char* modline){
    if(global_param.l){
        if(global_param.i){
            printf("%8u %s %u %4s %4s %5ld %s %s\n", buf->st_ino,modline, buf->st_nlink,
            user_name,group_name,buf->st_size,szBuf, name);
        }else{
            printf("%s %u %4s %4s %5ld %s %s\n", modline, buf->st_nlink,user_name,group_name,buf->st_size,szBuf, name);
        }
    }else{
        if(global_param.i){
            printf("%8u %s\n", buf->st_ino, name);
        }else{
            printf("%s\n", name);
        }
    }
}

void update_file_info(struct stat* buf, char* szBuf, char* user_name, char* group_name, char* modline){
    struct group *group;
    struct passwd *passwd;
    strftime(szBuf, N, "%h %d %Y %H:%M", localtime(&buf->st_mtime));
    if(szBuf[4]=='0'){
        szBuf[4]=' ';
    }
    passwd = getpwuid(buf->st_uid);
    if(passwd){
        strcpy(user_name, passwd->pw_name);
    }else{
        sprintf(user_name, "%s", buf->st_uid);
    }
    
    group = getgrgid(buf->st_gid);
    if(group){
        strcpy (group_name, group->gr_name);
    }else{
        sprintf (group_name, "%d", buf->st_gid);
    }
    getmod(buf->st_mode, modline);
}

int file_ignored(char* fname){
    if(fname[0]=='.'){
        return 1;
    }
    return 0;
}

int endwith(const char* p, char c){
    int i;
    i=0;
    char k;
    while(p[i]){
        k = p[i++];
    }
    return c==k;
}

void R_file(char* path){
    DIR *dp;
	struct dirent *entry;
	struct stat buf;
    char szBuf[N]={0};
    char user_name[64]={0};
    char group_name[64]={0};
    char modline[11]={0};
    char *name;
    char base[N]={0};
    char from_dir[N]={0};
    int i;
    int n,index;
    struct dirent **namelist = NULL;
    if(stat(path, &buf) == -1){
        perror(path);
        exit(1);
    }
    if(S_ISREG(buf.st_mode)){
        update_file_info(&buf, szBuf, user_name, group_name, modline);
        printBuf(&buf, user_name, group_name, szBuf, path, modline);
        return;
    }
    getcwd(from_dir,N);
	if ((dp = opendir(path)) == NULL) {
        perror(path);
		return ;
	}
    if(global_param.R){
        n = scandir(path, &namelist,0,alphasort);
        if(n < 0){ 
            perror(path);
        }
    }
    if(global_param.R){
        printf("%s:\n", path);
        index = 0;
        while(index < n){
            entry = namelist[index];
            if(file_ignored(entry->d_name)){
                free(namelist[index]);
                index++;
                continue;
            }
            strcpy(base, path);
            if(!endwith(path, '/')){
                strcat(base, "/");
            }else{
                i=0;
                while(path[i]){
                    i++;
                }
                i--;
                while(i-1>=0 && path[i-1]=='/'){
                    path[i]=0;
                    i--;
                }
            }
            strcat(base, entry->d_name);
            if(stat(base, &buf) == -1){
                perror(base);
                exit(1);
            }
            update_file_info(&buf, szBuf, user_name, group_name, modline);
            printBuf(&buf, user_name, group_name, szBuf, entry->d_name, modline);
            free(namelist[index]);
            index++;
        }
        rewinddir(dp);
        while ((entry = readdir(dp)) != NULL) {
            if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")){
                continue;
            }
            strcpy(base, path);
            if(!endwith(path, '/')){
                strcat(base, "/");
            }else{
                i=0;
                while(path[i]){
                    i++;
                }
                i--;
                while(i-1>=0 && path[i-1]=='/'){
                    path[i]=0;
                    i--;
                }
            }
            strcat(base, entry->d_name);
            stat(base, &buf);
            if(S_ISDIR(buf.st_mode) && !file_ignored(entry->d_name)){
                R_file(base);
            }
        }
    }else{
        while ((entry = readdir(dp)) != NULL) {
            if(file_ignored(entry->d_name)){
                continue;
            }
            strcpy(base, path);
            if(!endwith(path, '/')){
                strcat(base, "/");
            }else{
                i=0;
                while(path[i]){
                    i++;
                }
                i--;
                while(i-1>=0 && path[i-1]=='/'){
                    path[i]=0;
                    i--;
                }
            }
            strcat(base, entry->d_name);
            if(stat(base, &buf) == -1){
                perror(base);
                exit(1);
            }
            update_file_info(&buf, szBuf, user_name, group_name, modline);
            printBuf(&buf, user_name, group_name, szBuf, entry->d_name, modline);
        }
    }
	closedir(dp);
}

int deal_args(int argc, char* args[]){
    int i = 0, j;
    int start = 0;
    char p;
    memset(&global_param, 0, sizeof(Param));
    for(i=1;i<argc;i++){
        j = 0;
        if(args[i][0] == '-'){
            if(start){
                puts("All options (if any) must be specified before any files/directories (if any).");
                exit(1);
            }
            j++;
            while((p=args[i][j])){
                if(p=='i'){
                    global_param.i = 1;
                }else if(p=='R'){
                    global_param.R = 1;
                }else if(p=='l'){
                    global_param.l = 1;
                }else{
                    puts("unknown arguments");
                    exit(1);
                }
                j++;
            }
        }else{
            if(!start){
                start = i;
            }
        }
    }
    return start;
}

char **fileNameExpand(char **tokens)
{
   glob_t gl;
   int i, j, k;

   char** expanded_tokens = NULL;
   int len = 0;
   for(i = 0; tokens[i]; i++) {
      gl.gl_offs = 0;
      glob(tokens[i], GLOB_TILDE, 0, &gl);
      if(gl.gl_pathc){
         len += gl.gl_pathc;
      }else{
         len++;
      }
      globfree(&gl);
   }
   expanded_tokens = (char**)malloc(sizeof(char*) * (len+1));
   k = 0;
   for(i = 0; tokens[i]; i++) {
      gl.gl_offs = 0;
      glob(tokens[i], GLOB_NOCHECK|GLOB_TILDE, 0, &gl);
      for(j = 0; j < gl.gl_pathc; j++){
         expanded_tokens[k++] = strdup(gl.gl_pathv[j]);
      }
      if(!gl.gl_pathc){
         expanded_tokens[k++] = strdup(tokens[i]);
      }
      globfree(&gl);
   }
   expanded_tokens[k] = NULL;
   return expanded_tokens;
}

int main(int argc, char* args[]){
    char path[N]={0};
    char** file_names;
    char **expanded_tokens;
    int start;
    int i = 0;
    if((start=deal_args(argc, args))!=0){
        for(i=start;i<argc;i++){
            if(access(args[i], F_OK)==-1){
                printf("%s not exist!\n", args[i]);
                exit(1);
            }
        }
        expanded_tokens = fileNameExpand(args);
        for(i=start;expanded_tokens[i];i++){
            R_file(expanded_tokens[i]);
        }
        free_array(expanded_tokens);
    }else{
        getcwd(path, N);
        file_names = read_all_filesnames(path);
        sort_files(file_names);
        R_file(".");
        //read_file_list(file_names);
        free_array(file_names);
    }
    return 0;
}