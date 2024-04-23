#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"
#include "sdata.h"
#include "kernel/syscall.h"

#define KERNBASE 0x80000000  
#define V2P(a) (((uint) (a)) - KERNBASE)


int
main(int argc, char *argv[])
{
	char *fpath = (argc > 1) ? argv[1] : "../home/README";

	int sencntr = 1;
	char *lsw = "There is no longest word in the sentence";
	char *lw = "There is no longest word";
	int lw_sz = 0;
	char *ssw = "There is no shortest word in the sentence";
	char *sw = "There is no shortest word";
	int sw_sz =0;
	int cmd = 0;

	int *addr1 = &sencntr;
	int *addr2 = &lw_sz;
	int *addr3 = &sw_sz;
	int *addr4 = &cmd;


	int rez = 0;
	while(rez>=0){
		rez = share_data("fpath", fpath, sizeof(fpath));	
		rez = share_data("sencntr", addr1, sizeof(addr1));
		rez = share_data("lsw", lsw, sizeof(lsw));
		rez = share_data("lw", lw, sizeof(lw));
		rez = share_data("lw_sz", addr2, sizeof(addr2));
		rez = share_data("ssw", ssw, sizeof(ssw));
		rez = share_data("sw", sw, sizeof(sw));
		rez = share_data("sw_sz", addr3, sizeof(addr3));
		rez = share_data("cmd", addr4, sizeof(addr4));
		break;
	}

	//Handlovanje errora
	if(rez<0){
		switch (rez)
		{
		case -1:
			printf("Invalid arguments!\n");
			break;
		case -2:
			printf("The name is already taken!\n");
			break;
		case -3:
			printf("There is no free space!\n");
			break;
		default:
			printf("Unknown error!\n");
			break;
		}
		exit();
	}
	else printf("Successfully shared structures.\n");

	int pid1 = fork();
	if(pid1==0){
		char *commaArgs[2] = {"/bin/coMMa", 0};
		exec(commaArgs[0], commaArgs);
	}

	int pid2 = fork();
	if(pid2==0){
		char *lisaArgs[2] = {"/bin/liSa", 0};
		exec(lisaArgs[0],lisaArgs);
	}

	if(pid1<0 || pid2<0){
		printf("Unsuccessful fork()!\n");
		exit();
	}

	wait();
	wait();
	exit();
}
