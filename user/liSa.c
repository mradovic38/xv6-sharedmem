#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "sdata.h"
#include "kernel/fcntl.h"

char *
strcat (char *dest, const char *src)
{
  strcpy (dest + strlen (dest), src);
  return dest;
}

int cmp(char *s1, char *s2){
	int i=-1;
	while(s1[++i]!='\0'){
		if(s2[i]=='\0') return 1;
	}
	return -1;
}

int
main(int argc, char *argv[])
{
	if(argc<1) {
		printf("Greska!\n");
		exit();
	}

	#pragma region Gathering shared data

	char *fpath;
	int sencntr;
	char *lsw;
	char *lw;
	int lw_sz;
	char *ssw;
	char *sw;
	int sw_sz;
	int cmd;

	int *sencntrp;
	int *lwszp;
	int *swszp;
	int *cmdp;

	// takes shared data
	int rez = 0;
	while(rez>=0){
		rez = get_data("fpath", &fpath);	
		rez = get_data("sencntr", &sencntrp);
		sencntr = *sencntrp;
		rez = get_data("lsw", &lsw);
		rez = get_data("lw", &lw);
		rez = get_data("lw_sz", &lwszp);
		lw_sz = *lwszp;
		rez = get_data("ssw", &ssw);
		rez = get_data("sw", &sw);
		rez = get_data("sw_sz", &swszp);
		sw_sz = *swszp;
		rez = get_data("cmd", &cmdp);
		cmd = *cmdp;
		break;
	}

	// Handling errors
	if(rez<0){
		switch (rez)
		{
		case -1:
			printf("Invalid arguments!\n");
			break;
		case -2:
			printf("There is no such shared structure!\n");
			break;
		default:
			printf("Unknown error!\n");
			break;
		}
		exit();
	}
	else printf("Successfully gathered shared data.\n");

	#pragma endregion

	char *strncat(char *dest, const char *src, int n)
	{
		char *p = dest + strlen(dest);
		int src_len = strlen(src);

		int i;
		for (i = 0; i < n && src[i] != '\0'; i++)
			p[i] = src[i];

		p[i] = '\0';

		return dest;
	}

	int n;
	char c;
	int fd,cur = 0;
	int wstart = -1;
	int wend = 0;
	char curWrd[512];
	const char* path;
	int isSleeping =0;

	strcpy(path,fpath);
	if((fd = open(path, O_RDONLY))<0){
		printf("Error opening file %s\n", fpath);
		*cmdp = 3;
		exit();
	}
	printf("File processing [%s]...\n",fpath);
	
	while(*cmdp!=3){
			while(n = (read(fd,&c,sizeof(char)))){
					if(*cmdp==3){
							break;
					}
					int i=0;
					while(i<n){
						if(*cmdp==1){
						sleep(1);
						continue;
						}
						if(*cmdp==3){
							break;
						}
						// word end
						if(c == '.' || c=='!' || c=='?' || c=='\0'){
							(*sencntrp)++;
							wstart = -1;
							wend = 0;
							curWrd[0]= '\0';
							strcpy(lsw, "no");
							strcpy(ssw,"no");
							if(!isSleeping){
								sleep(150);
								isSleeping = 1;
							}
							i++;
							continue;
						}
						//nije kraj recenice

						//nije kraj reci
						if((c>='a' && c<='z') || (c>='A' && c<='Z') || c=='-' || c=='\'' || (c>='0' && c<='9')){
							if(wstart==-1) wstart = 0;
							strncat(curWrd, &c,1);
							wend++;
							isSleeping = 0;
							i++;
							continue;
						}
						else{
						//kraj reci
							if(wstart!=-1) {
								if(cmp(curWrd, ssw)<0 || strcmp(ssw, "There is no shortest word in the sentence")==0 || strcmp(ssw, "no")==0) {
									strcpy(ssw, curWrd);
								}
								if(cmp(curWrd, lsw)>0 || *lwszp==0 || strcmp(ssw, "There is no longest word in the sentence")==0 || strcmp(lsw, "no")==0) {
									strcpy(lsw, curWrd);
								}
								if(cmp(curWrd,sw)<0 || *lwszp==0){
									strcpy(sw, curWrd);
									*swszp = wend - wstart;
								}
								if(cmp(curWrd,lw)>0 || *lwszp==0){
									strcpy(lw, curWrd);
									*lwszp = wend - wstart;
								}
								if(!isSleeping){
									sleep(150);
									isSleeping = 1;
								}
							}
							
							wstart = -1;
							wend = 0;
							curWrd[0]='\0';
							i++;
						}
							
					}
				
			}
			printf("End of file processing.\n");
			break;
			
			
		
	}
	exit();
	
}
