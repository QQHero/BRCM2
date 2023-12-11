#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

// web资源文件链接替换
// 2016年12月10日10:15

char *webroot = "login";
char *webroot_copy = "login_copy";

int length_diff;

int prefix;
int n=0;
int i=0;
char **p;
char **p_copy;

void show_files_number(char *path_name)
{
    struct stat file_stat;
    DIR *pdir_name = NULL;
    struct dirent *pdir_subname = NULL;
    char filename_buf[1024] = { 0 };
	
	// 排除 .svn|*.db
	if( NULL!=strstr(path_name,".svn") || NULL!=strstr(path_name,".db") )
	{
		return ;
	}

    if(0 == lstat(path_name,&file_stat))
    {
        if(S_ISREG(file_stat.st_mode))
        {
			n++;
        }
        else if(S_ISDIR(file_stat.st_mode))
        {
            if(NULL == (pdir_name = opendir(path_name)))
            {
                exit(0);
            }
            while((pdir_subname = readdir(pdir_name)) != NULL)
            {
                if(pdir_subname->d_name[0] == '.')
                {
                    continue;
                }
                snprintf(filename_buf,sizeof(filename_buf),"%s/%s",path_name,pdir_subname->d_name);
                show_files_number(filename_buf);
            }
        }
    }
	return ;
}

void show_files(char *path_name)
{
    struct stat file_stat;
    DIR *pdir_name = NULL;
    struct dirent *pdir_subname = NULL;
    char filename_buf[1024] = { 0 };
	
	// 排除 .svn|*.db
	if( NULL!=strstr(path_name,".svn") || NULL!=strstr(path_name,".db") )
	{
		return ;
	}
    if(0 == lstat(path_name,&file_stat))
    {
        if(S_ISREG(file_stat.st_mode))
        {
			int length = strlen(path_name)+1;
			char *tmp = malloc(length);
			strncpy(tmp,path_name,length);
			p[i] = tmp;
			char *tmp2 = malloc(length + length_diff);
			sprintf(tmp2, "%s%s", webroot_copy,path_name+prefix);
			p_copy[i] = tmp2;
			i++;
        }
        else if(S_ISDIR(file_stat.st_mode))
        {
            if(NULL == (pdir_name = opendir(path_name)))
            {
                exit(0);
            }
            while((pdir_subname = readdir(pdir_name)) != NULL)
            {
                if(pdir_subname->d_name[0] == '.')
                {
                    continue;
                }
                snprintf(filename_buf,sizeof(filename_buf),"%s/%s",path_name,pdir_subname->d_name);
                show_files(filename_buf);
            }
        }
    }
    
    return;
}


void insert(char *dest, char *src, int start)
{
	int length = strlen(dest);
	char *tmp = malloc(length-start+1);
	
	int j=0, k;
	for(k=start;k<length;k++,j++)
	{
		tmp[j] = dest[k];
	}
	tmp[j] = '\0';
	
	k=start;
	while( *(src) !='\0' )
	{
		dest[k] = *src;
		k++;
		src++;
	}
	while( *(tmp) !='\0' )
	{
		dest[k] = *tmp;
		k++;
		tmp++;
	}
	dest[k] = '\0';
}

char * random_string(char *t)
{
	int k;
	int seconds = time((time_t*)NULL);
	t[0] = '?';
	for( k=1; k<11; k++)
	{
		t[k] = seconds%10 + '0';
		seconds = seconds/10;
	}
	return t;
}

int main(int argc, char**argv)
{
	if(3 == argc)
	{
		webroot = argv[1];
		webroot_copy = argv[2];
	}

	prefix = strlen(webroot);
	length_diff = strlen(webroot_copy) - prefix;
	show_files_number(webroot);
	
	p = malloc( n *sizeof(char *));
	p_copy = malloc( n *sizeof(char *));
	
	show_files(webroot);
	
	FILE * file = NULL;
	FILE * file2 = NULL;
	char line_buf[1024] = { 0 };
	char *tmp = NULL;
	int j=0,k=0;

	const char *noneed[5]={".js", ".css", ".png", ".jpg", ".gif"};
	for(i=0;i<n;i++)
	{
		// 排除 *.js|*.css|*.png|*.jpg|*.gif
		for(j=0; j<5; j++)
		{
			if( NULL!=strstr(p[i], noneed[j]) )
			{
				break;
			}
		}
		if(j<5)
			continue;
		
		file = fopen(p[i],"r");
		file2 = fopen(p_copy[i],"w+");

		while (fgets(line_buf, sizeof (line_buf), file) != NULL)
		{
			
			for(k=0;k<n;k++)
			{
				char * find_str = p[k]+prefix;
				tmp = strstr(line_buf, find_str);
				if(tmp != NULL)
				{
					int length = strlen(line_buf) - strlen(tmp) + strlen(find_str);
					//insert(line_buf, "?xxxxxxxx", length);
					char str[12] = {0};
					random_string(str);
					insert(line_buf, str, length);
				}
			}
			
			fputs(line_buf, file2);

		}
		fclose(file);
		fclose(file2);
	}
	
	
	for(i=0;i<n;i++)
	{
		free(p[i]);
	}
	free(p);
	
/*
	printf("%s\n",__FILE__);
	
	struct stat buf;
    struct stat buf2;
  
    int result;  
  
    result = stat ("./nocache.c", &buf);
	stat (webroot, &buf2); 
  
    if (result != 0)  
      {  
          perror ("Failed ^_^");  
      }  
    else  
      {
		  //! 文件的大小，字节为单位
			printf("%d\n",buf.st_size);
			
			//! 文件创建的时间  
			printf("%s\n",ctime (&buf.st_ctime));
			
			//! 最近一次修改的时间 
			printf("%s\n",ctime (&buf.st_mtime));
			
			//! 最近一次访问的时间 
			printf("%s\n",ctime (&buf.st_atime));
            
			int x = S_ISDIR(buf.st_mode); // 是否是一个目录
			printf("%d\n",x);
			
			x = S_ISDIR(buf2.st_mode); // 是否是一个目录
			printf("%d\n",x);
         
      } 
*/
	return 0;
}