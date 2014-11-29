#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

/* -- Pathwayz Reference --
* Cell States:
*   0 -> empty
*   1 -> solid white
*   2 -> solid black
*   3 -> white with black dot
*   4 -> black with white dot
* gamestate[96] = player (0 = white, 1 = black)
*/

//Global Constants
const int PLAYERS = 2;
const int GAMESTATE_SIZE = 97; //squares on a pathwayz board plus current player
const int MAX_RECUR_DEPTH = 2;
const int BOARD_ROWS = 8;
const int BOARD_COLS = 12;

//Global Variables

//Function Prototypes
void initializeGameState(char* gamestate);
int parseFileIntoGameState(char* gamestate, char *filename);
void printGameState(char* gamestate, FILE* output);
int doesPathTraverse(char* gamestate, int row, int col, int player);
int getWinner(char* gamestate); //returns -1 if game is not yet over
char** getLegalNextGameStates(char* gamestate); //returns null if no available transitions
float* unweightedWinStatistics(char* gamestate);
float* weightedWinStatistics(char* gamestate, int recurDepth, int maxRecurDepth);

//Functions
void initializeGameState(char* gamestate){
    memset(gamestate,0,GAMESTATE_SIZE*sizeof(char));
}

int parseFileIntoGameState(char* gamestate, char *filename){
    FILE* infile;
    char c;
    int i;

    infile = fopen(filename,"r");
    i = 0;

    if(infile == NULL) return 1;
    while(1){
        c = fgetc(infile);
        if(feof(infile)) break;
        if(c<='9' && c>='0'){
            gamestate[i] = (char)(c - '0');
            i++;
        }
    }
    fclose(infile);

    if(gamestate[GAMESTATE_SIZE-1] > PLAYERS){
        return 1;
    }

    return 0;
}

void printGameState(char* gamestate, FILE* output){
    int i,j;
    for(i=0; i<BOARD_ROWS; i++){
        for(j=0; j<BOARD_COLS; j++){
            switch(gamestate[i*BOARD_COLS+j]){
                case 0:
                    fprintf(output,"   ");
                    break;
                case 1:
                    fprintf(output," O ");
                    break;
                case 2:
                    fprintf(output," X ");
                    break;
                case 3:
                    fprintf(output,"<O>");
                    break;
                case 4:
                    fprintf(output,"<X>");
                    break;
                default:
                    fprintf(output,"###");
            }
        }
        fprintf(output,"\n");
    }
    fprintf(output,"PLAYER TO MOVE: %c\n",gamestate[GAMESTATE_SIZE-1]==0?'O':'X');
}

int doesPathTraverse(char* gamestate, int row, int col, int player){
    char c;
    int isPath;
    if(row<0 || row>=BOARD_ROWS || col<0 || col>=BOARD_COLS) return 0;
    c = gamestate[row*BOARD_COLS+col];
    if(c != player && c!= player+2) return 0;
    if(col == BOARD_COLS-1) return 1;
    gamestate[row*BOARD_COLS+col] = '*';
    isPath = doesPathTraverse(gamestate,row-1,col-1,player) ||
             doesPathTraverse(gamestate,row-1,col  ,player) ||
             doesPathTraverse(gamestate,row-1,col+1,player) ||
             doesPathTraverse(gamestate,row  ,col-1,player) ||
             doesPathTraverse(gamestate,row  ,col+1,player) ||
             doesPathTraverse(gamestate,row+1,col-1,player) ||
             doesPathTraverse(gamestate,row+1,col  ,player) ||
             doesPathTraverse(gamestate,row+1,col+1,player) ;
    gamestate[row*BOARD_COLS+col] = c;
    return isPath;
}

int getWinner(char* gamestate){
    int i,j;
    char elem;
    char whiteCouldWin = 1;
    char blackCouldWin = 1;
    char whiteInCol = 0;
    char blackInCol = 0;
    char whiteWins = 0;
    char blackWins = 0;

    for(j=0; j<BOARD_COLS; j++){
        for(i=0; i<BOARD_ROWS; i++){
            elem = gamestate[i*BOARD_COLS+j];
            if(elem==1 || elem==3){
                whiteInCol = 1;
            }else if(elem==2 || elem==4){
                blackInCol = 1;
            }
        }

        whiteCouldWin &= whiteInCol;
        blackCouldWin &= blackInCol;

        if(!(whiteCouldWin || blackCouldWin)){
            return -1;
        }
    }

    whiteWins = 0;
    blackWins = 0;
    for(i=0; i<BOARD_ROWS; i++){
        elem = gamestate[i*BOARD_COLS];
        if(!whiteWins && whiteCouldWin && (elem==1 || elem==3)){
            whiteWins = doesPathTraverse(gamestate,i,0,1);
        }else if(!blackWins && blackCouldWin && (elem==2 || elem==4)){
            blackWins = doesPathTraverse(gamestate,i,0,2);
        }
    }

    if(!whiteWins && !blackWins){
        return -1;
    }else if(whiteWins){
        return 1;
    }else if(blackWins){
        return 2;
    }else if(whiteWins && blackWins){
        return gamestate[GAMESTATE_SIZE-1]==1?2:1;
    }else{
        return -1;
    }
}

char** getLegalNextGameStates(char* gamestate){

    return (char**)0;
}

float* unweightedWinStatistics(char* gamestate){

    return (float*)0;
}

float* weightedWinStatistics(char* gamestate, int recurDepth, int maxRecurDepth){

    return (float*)0;
}

//Main
int main(int argc, char **argv){
    char* gamestate = malloc(GAMESTATE_SIZE*sizeof(char));
    initializeGameState(gamestate);
    if(argc > 0){
        if(parseFileIntoGameState(gamestate, argv[0]) == 0){
            printf("File %s loaded successfully.\n",argv[0]);
        }else{
            printf("ERROR: File %s not loaded.\n",argv[0]);
            return 1;
        }
    }

    return 0;
}
