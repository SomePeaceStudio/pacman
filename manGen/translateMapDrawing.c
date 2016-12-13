#include "stdio.h"
#include <stdlib.h>
#include <string.h>

#define MAXBUFF 255

char empty 	= '~';	int emptyVal 	= 0;
char wall 	= '=';	int wallVal 	= 1;
char point 	= '.';	int pointVal 	= 3;

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
    // while((read = fread(buffer, 1, sizeof(buffer), mapDrawFile)) > 0){
    debug_print("%s\n", "Writting to temp file..");
    while(fscanf(mapDrawFile, "%s", buffer) > 0){
    	buffer[MAXBUFF-1] = '\0';
    	debug_print("Just read: %s count %d\n", buffer, (int)strlen(buffer));
    	if(!mapWidth) mapWidth = strlen(buffer);
	    // buffer[read] = '\0';
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