#include "stdio.h"
#include <stdlib.h>
#include <string.h>

#define MAXBUFF 255

//  None	= 0
//  Dot		= 1
//  Wall	= 2
//  PowerPellet		= 3
//  Invincibility	= 4
//  Score	= 5

char empty 			= '~';	int emptyVal 			= 0;
char point 			= '.';	int pointVal 			= 1;
char wall 			= '=';	int wallVal 			= 2;
char powerPellet 	= '*';	int powerPelletVal 		= 3;
char invincibility 	= '#';	int invincibilityVal 	= 4;
char score 			= '$';	int scoreVal 			= 5;

void Die(char *mess) { perror(mess); exit(1); }
#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)


int main(int argc, char const *argv[])
{
	char buffer[MAXBUFF];
	int mapWidth = 0;
	int mapHeight = 0;
	if(argc != 3){
		printf("%s\n", "Usage: translateMapDrawing <mapDrawing> <mapOutput>");
		return 1;
	}
	FILE *mapDrawFile;
    if((mapDrawFile = fopen(argv[1], "r")) == NULL){
        Die("Could not open mapDrawFile");
        return 1;
    }

    FILE *tempFile;
    if((tempFile = fopen("temp", "w+")) == NULL){
        Die("Could not open tempFile");
        return 1;
    }

    FILE *mapOut;
    if((mapOut = fopen(argv[2], "w")) == NULL){
        Die("Could not open mapOut");
        return 1;
    }
    debug_print("%s\n", "Opened both files!");   
    debug_print("%s\n", "Writting to temp file..");
    while(fscanf(mapDrawFile, "%s", buffer) > 0){
    	buffer[MAXBUFF-1] = '\0';
    	debug_print("Just read: %s count %d\n", buffer, (int)strlen(buffer));
    	if(!mapWidth) mapWidth = strlen(buffer);
	    for (int i = 0; i < strlen(buffer); ++i){
	    	if(buffer[i] == empty){
	    		fprintf(tempFile, "%d %d %d\n", emptyVal, mapHeight, i);
	    		continue;
	    	}
	    	if(buffer[i] == wall){
	    		fprintf(tempFile, "%d %d %d\n", wallVal, mapHeight, i);
	    		continue;	
	    	}
	    	if(buffer[i] == point){
	    		fprintf(tempFile, "%d %d %d\n", pointVal, mapHeight, i);
	    		continue;
	    	}
	    	if(buffer[i] == powerPellet){
	    		fprintf(tempFile, "%d %d %d\n", powerPelletVal, mapHeight, i);
	    		continue;
	    	}
	    	if(buffer[i] == invincibility){
	    		fprintf(tempFile, "%d %d %d\n", invincibilityVal, mapHeight, i);
	    		continue;
	    	}
	    	if(buffer[i] == score){
	    		fprintf(tempFile, "%d %d %d\n", scoreVal, mapHeight, i);
	    		continue;
	    	}
	    }
	    mapHeight++;
	}
	
	debug_print("%s\n", "Writting map sizes..");
	fprintf(mapOut, "%d %d\n", mapWidth, mapHeight);
	debug_print("%s\n", "Rewinding temp file..");
	rewind(tempFile);

	debug_print("%s\n", "Coppying results to mapOut..");
	size_t lineLen;
	char* line;
	while (getline(&line, &lineLen, tempFile) != -1) {
		fprintf(mapOut, "%s", line);
    }
    
    if (line){
        free(line);
    }

	debug_print("%s\n", "Closing files..");
    if(fclose(mapDrawFile) != 0){
        printf("%s\n", "Could not close mapDrawFile");
    }
    if(fclose(mapOut) != 0){
        printf("%s\n", "Could not close mapOut");
    }
    if(fclose(tempFile) != 0){
        printf("%s\n", "Could not close mapOut");
    }

	return 0;
}