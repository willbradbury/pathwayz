#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
const int NUM_SIMULATIONS = 100000;

//Global Variables

//Function Prototypes
void initializeGameState(char* gamestate);
int parseFileIntoGameState(char* gamestate, char *filename);
void printGameState(char* gamestate, FILE* output);
int doesPathTraverse(char* gamestate, int row, int col, int player);
int getWinner(char* gamestate); //returns -1 if game is not yet over
char** getLegalNextGameStates(char* gamestate,int* numNextGameStates); //returns null if no available transitions
float* unweightedWinStatistics(char* gamestate);
float* weightedWinStatistics(char* gamestate, int recurDepth, int maxRecurDepth);

//Functions
void initializeGameState(char* gamestate){
    memset(gamestate,0,GAMESTATE_SIZE*sizeof(char));
    gamestate[GAMESTATE_SIZE-1] = 1;
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
    fprintf(output,"PLAYER TO MOVE: %c\n",gamestate[GAMESTATE_SIZE-1]==1?'O':'X');
}

int getWinner(char* gamestate){
    int i,j,p,q,whiteInCol,blackInCol,whiteWins,blackWins,changed;
    char c;
    char* connectivity;
    connectivity = (char*)malloc(GAMESTATE_SIZE*sizeof(char));
    memset(connectivity,0,GAMESTATE_SIZE*sizeof(char));
    whiteWins = 0;
    blackWins = 0;
    whiteInCol = 0;
    blackInCol = 0;
    for(i=0;i<BOARD_ROWS;i++){
        c = gamestate[i*BOARD_COLS];
        if(c==1 || c==3){
            connectivity[i*BOARD_COLS] = 1;
            whiteInCol = 1;
        }else if(c==2 || c==4){
            connectivity[i*BOARD_COLS] = 2;
            blackInCol = 1;
        }
    }
    if(!whiteInCol && !blackInCol){
        free(connectivity);
        return -1;
    }
    changed = 1;
    while(changed){
        changed = 0;
        for(j=1;j<BOARD_COLS;j++){
            for(i=0;i<BOARD_ROWS;i++){
                if(connectivity[i*BOARD_COLS+j]) continue;
                c = gamestate[i*BOARD_COLS+j];
                if(c==1 || c==3){
                    for(p=-1;p<2;p++){
                        for(q=-1;q<2;q++){
                            if( (i+p)<0 || (i+p)>=BOARD_ROWS || (j+q)<0 || (j+q)>=BOARD_COLS ) continue;
                            if(connectivity[(i+p)*BOARD_COLS+(j+q)]==1){
                                changed = 1;
                                connectivity[i*BOARD_COLS+j] = 1;
                            }
                        }
                    }
                }else if(c==2 || c==4){
                    for(p=-1;p<2;p++){
                        for(q=-1;q<2;q++){
                            if( (i+p)<0 || (i+p)>=BOARD_ROWS || (j+q)<0 || (j+q)>=BOARD_COLS ) continue;
                            if(connectivity[(i+p)*BOARD_COLS+(j+q)]==2){
                                changed = 1;
                                connectivity[i*BOARD_COLS+j] = 2;
                            }
                        }
                    }
                }
            }
        }
    }
    j = BOARD_COLS-1;
    for(i=0;i<BOARD_ROWS;i++){
        if(connectivity[i*BOARD_COLS+j]==1){
            whiteWins = 1;
        }else if(connectivity[i*BOARD_COLS+j]==2){
            blackWins = 1;
        }
    }
    free(connectivity);
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

char** getLegalNextGameStates(char* gamestate,int* numNextGameStates){
    int i,currPlayer,nextPlayer,row,col;
    char** nextGameStates;
    int switches[6] = {0,2,1,3,4,5};

    *numNextGameStates = 0;
    for(i=0;i<BOARD_ROWS*BOARD_COLS;i++){
        if(gamestate[i]==0){
            *numNextGameStates+=2;
        }
    }

    if(*numNextGameStates==0){
        return (char**)0;
    }

    nextGameStates = (char**)malloc(*numNextGameStates*sizeof(char*));

    for(i=0;i<BOARD_ROWS*BOARD_COLS;i++){
        if(gamestate[i]==0){
            nextGameStates[2*i] = (char*)malloc(GAMESTATE_SIZE*sizeof(char));
            nextGameStates[2*i+1] = (char*)malloc(GAMESTATE_SIZE*sizeof(char));
            memcpy(nextGameStates[2*i],gamestate,GAMESTATE_SIZE*sizeof(char));
            memcpy(nextGameStates[2*i+1],gamestate,GAMESTATE_SIZE*sizeof(char));
            currPlayer = gamestate[GAMESTATE_SIZE-1];
            nextPlayer = currPlayer==1?2:1;
            nextGameStates[2*i][GAMESTATE_SIZE-1] = nextPlayer;
            nextGameStates[2*i+1][GAMESTATE_SIZE-1] = nextPlayer;
            nextGameStates[2*i][i] = currPlayer;
            nextGameStates[2*i+1][i] = nextPlayer+2;
            row = i/BOARD_COLS;
            col = i%BOARD_COLS;
            if(row>0){
                nextGameStates[2*i+1][i-BOARD_COLS  ] = switches[(int)nextGameStates[2*i+1][i-BOARD_COLS  ]];
                if(col>0){
                    nextGameStates[2*i+1][i-BOARD_COLS-1] = switches[(int)nextGameStates[2*i+1][i-BOARD_COLS-1]];
                }
                if(col<BOARD_COLS-1){
                    nextGameStates[2*i+1][i-BOARD_COLS+1] = switches[(int)nextGameStates[2*i+1][i-BOARD_COLS+1]];
                }
            }
            if(row<BOARD_ROWS-1){
                nextGameStates[2*i+1][i+BOARD_COLS  ] = switches[(int)nextGameStates[2*i+1][i+BOARD_COLS  ]];
                if(col>0){
                    nextGameStates[2*i+1][i+BOARD_COLS-1] = switches[(int)nextGameStates[2*i+1][i+BOARD_COLS-1]];
                }
                if(col<BOARD_COLS-1){
                    nextGameStates[2*i+1][i+BOARD_COLS+1] = switches[(int)nextGameStates[2*i+1][i+BOARD_COLS+1]];
                }
            }
            if(col>0){
                nextGameStates[2*i+1][i           -1] = switches[(int)nextGameStates[2*i+1][i           -1]];
            }
            if(col<BOARD_COLS-1){
                nextGameStates[2*i+1][i           +1] = switches[(int)nextGameStates[2*i+1][i           +1]];
            }
        }
    }
    return nextGameStates;
}

float* unweightedWinStatistics(char* gamestate){
    int k,i,j,whiteWins,blackWins,winner,numNextGameStates,currPlayer,nextPlayer,row,col,simulations;
    int switches[6] = {0,2,1,3,4,5};
    char* playingGame;
    float* winStats;

    srand(time(NULL));

    winStats = (float*)malloc(PLAYERS*sizeof(float));
    playingGame = (char*)malloc(GAMESTATE_SIZE*sizeof(char));

    simulations = 0;
    whiteWins = 0;
    blackWins = 0;

    for(k=0;k<NUM_SIMULATIONS;k++){
        memcpy(playingGame,gamestate,GAMESTATE_SIZE*sizeof(char));
        while(-1 == (winner=getWinner(playingGame))){
            numNextGameStates = 0;
            for(i=0;i<BOARD_ROWS*BOARD_COLS;i++){
                if(gamestate[i]==0){
                    numNextGameStates+=2;
                }
            }
            if(numNextGameStates==0){
                printf("Found a draw...\n");
                break;
            }
            i = rand()%numNextGameStates;
            j = i/2;
            currPlayer = playingGame[GAMESTATE_SIZE-1];
            nextPlayer = currPlayer==1?2:1;
            if(i%2==0){
                playingGame[j] = currPlayer;
            }else{
                row = j/BOARD_COLS;
                col = j%BOARD_COLS;
                playingGame[j] = nextPlayer+2;
                if(row>0){
                    playingGame[j-BOARD_COLS  ] = switches[(int)playingGame[j-BOARD_COLS  ]];
                    if(col>0){
                        playingGame[j-BOARD_COLS-1] = switches[(int)playingGame[j-BOARD_COLS-1]];
                    }
                    if(col<BOARD_COLS-1){
                        playingGame[j-BOARD_COLS+1] = switches[(int)playingGame[j-BOARD_COLS+1]];
                    }
                }
                if(row<BOARD_ROWS-1){
                    playingGame[j+BOARD_COLS  ] = switches[(int)playingGame[j+BOARD_COLS  ]];
                    if(col>0){
                        playingGame[j+BOARD_COLS-1] = switches[(int)playingGame[j+BOARD_COLS-1]];
                    }
                    if(col<BOARD_COLS-1){
                        playingGame[j+BOARD_COLS+1] = switches[(int)playingGame[j+BOARD_COLS+1]];
                    }
                }
                if(col>0){
                    playingGame[j           -1] = switches[(int)playingGame[j           -1]];
                }
                if(col<BOARD_COLS-1){
                    playingGame[j           +1] = switches[(int)playingGame[j           +1]];
                }
            }
            playingGame[GAMESTATE_SIZE-1] = nextPlayer;
        }
        switch(winner){
            case 1:
                whiteWins++;
                simulations++;
                break;
            case 2:
                blackWins++;
                simulations++;
                break;
        }
    }
    free(playingGame);
    winStats[0] = (float)whiteWins/(float)simulations;
    winStats[1] = (float)blackWins/(float)simulations;
    return winStats;
}

float* weightedWinStatistics(char* gamestate, int recurDepth, int maxRecurDepth){
    //THIS METHOD DOES NOTHING RIGHT NOW...
    char** nextGameStates;
    return (float*)0;
}

//Main
int main(int argc, char **argv){
    char* gamestate = (char*)malloc(GAMESTATE_SIZE*sizeof(char));
    initializeGameState(gamestate);
    if(argc > 1){
        if(parseFileIntoGameState(gamestate, argv[1]) == 0){
            printf("File %s loaded successfully.\n",argv[1]);
        }else{
            printf("ERROR: File %s not loaded.\n",argv[1]);
            return 1;
        }
    }
    int i,j,numNextGameStates;
    FILE* outfile = fopen("output.txt","w");
    char** nextStates = getLegalNextGameStates(gamestate,&numNextGameStates);
    float* stats;
    float whiteWinsBoard[BOARD_COLS*BOARD_ROWS];
    for(i=0;i<numNextGameStates;i+=2){
        stats = unweightedWinStatistics(nextStates[i]);
        printGameState(nextStates[i],stdout);
        printf("White Wins: %f\nBlack Wins: %f\n",stats[0],stats[1]);
        whiteWinsBoard[i/2] = stats[0];
    }
    for(i=0;i<BOARD_ROWS;i++){
        for(j=0;j<BOARD_COLS;j++){
            fprintf(outfile,"%7f,",whiteWinsBoard[i*BOARD_COLS+j]);
        }
        fprintf(outfile,"\n");
    }
    return 0;
}
