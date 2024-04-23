#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "sdata.h"

int
main(int argc, char *argv[])
{
	if(argc<1) {
		printf("Error!\n");
		exit();
	}

	#pragma region Prikupljanje deljenih objekata

	char *fpath;
	int sencntr;
	char *lsw;
	char *lw;
	int lw_sz;
	char *ssw;
	char *sw;
	int sw_sz;
	int cmd;

	int *addr1;
	int *addr2;
	int *addr3;
	int *addr4;

	// takes shared data
	int rez = 0;
	while(rez>=0){
		rez = get_data("fpath", &fpath);	
		rez = get_data("sencntr", &addr1);
		sencntr = *addr1;
		rez = get_data("lsw", &lsw);
		rez = get_data("lw", &lw);
		rez = get_data("lw_sz", &addr2);
		lw_sz = *addr2;
		rez = get_data("ssw", &ssw);
		rez = get_data("sw", &sw);
		rez = get_data("sw_sz", &addr3);
		sw_sz = *addr3;
		rez = get_data("cmd", &addr4);
		cmd = *addr4;
		break;
	}

	// Error handling
	if(rez<0){
		switch (rez)
		{
		case -1:
			printf("Wrong arguments!\n");
			break;
		case -2:
			printf("There is no such shared structure!\n");
			break;
		default:
			printf("Unknown problem!\n");
			break;
		}
		exit();
	}
	else printf("Structures successfully extracted.\n");

	#pragma endregion

	#pragma region Meni
	//Stampanje menija
	printf("\n~MENU~\n");
	printf("	-latest\n");
	printf("	-gloabal extrema\n");
	printf("	-pause\n");
	printf("	-resume\n");
	printf("	-end\n\n");
	#pragma endregion

	while(1){
	
		// Odabir opcije
		char o[128];
		int n = read(0, o, sizeof(o));
		o[n-1] = '\0';

		// Latest
		if(strcmp(o, "latest")==0){
			printf("We are currently processing %d. sentence\n", *addr1);
			printf("The longest word in it is: %s\n", lsw);
			printf("The shortest word in it is: %s\n", ssw);
		}

		// Global extrema
		else if(strcmp(o, "global extrema")==0){
			printf("The longest word: %s [%d]\n", lw, *addr2);
			printf("The shortest word: %s [%d]\n", sw, *addr3);
		}

		// Pause
		else if(strcmp(o, "pause")==0){
			printf("Paused...\n");
			*addr4 = 1;
		}

		// Resume
		else if(strcmp(o, "resume")==0){
			printf("Resumed...\n");
			*addr4 = 2;
		}

		// End
		else if(strcmp(o, "end")==0){
			*addr4 = 3;
			exit();
		}

		// Incorrect input
		else{
			printf("Incorrect input!\n");
		}
	
	}

	
	
	exit();
}
