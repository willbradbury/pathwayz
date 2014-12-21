#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "khash.h"

#define TRUE 1
#define FALSE 0
typedef struct{
    int wins;
    int visits;
} gamestate_stats;

KHASH_MAP_INIT_STR(str, gamestate_stats)

/* -- Pathwayz Reference --
* Cell States:
*   1 -> empty
*   2 -> solid white
*   3 -> solid black
*   4 -> white with black dot
*   5 -> black with white dot
* gamestate[96] = player (1 = white, 2 = black)
* gamestate[97] = 0
*/

//Global Constants
const int PLAYERS = 2;
const int GAMESTATE_SIZE = 97; //squares on a pathwayz board plus current player
const int BOARD_ROWS = 8;
const int BOARD_COLS = 12;
const int NUM_SIMULATIONS = 10000;

//Global Variables
khash_t(str) *simulationData;

//Function Prototypes
void initializeGameState(char* gamestate);
int parseFileIntoGameState(char* gamestate, char *filename);
void printGameState(char* gamestate, FILE* output);
int doesPathTraverse(char* gamestate, int row, int col, int player);
int getWinner(char* gamestate); //returns -1 if game is not yet over
char** getLegalNextGameStates(char* gamestate,int* numNextGameStates); //returns null if no available transitions
float* unweightedWinStatistics(char* gamestate);
int playRandomSimulation(char* gamestate); //returns winner
int simulateGame(char* gamestate); //returns winner
char* getBestMove(char* gamestate);

//Functions
void initializeGameState(char* gamestate){
    memset(gamestate,1,GAMESTATE_SIZE*sizeof(char)+1);
    gamestate[GAMESTATE_SIZE-1] = 1;
    gamestate[GAMESTATE_SIZE] = 0;
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
                case 1:
                    fprintf(output,"   ");
                    break;
                case 2:
                    fprintf(output," O ");
                    break;
                case 3:
                    fprintf(output," X ");
                    break;
                case 4:
                    fprintf(output,"<O>");
                    break;
                case 5:
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
        if(c==2 || c==4){
            connectivity[i*BOARD_COLS] = 1;
            whiteInCol = 1;
        }else if(c==3 || c==5){
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
                if(c==2 || c==4){
                    for(p=-1;p<2;p++){
                        for(q=-1;q<2;q++){
                            if( (i+p)<0 || (i+p)>=BOARD_ROWS || (j+q)<0 || (j+q)>=BOARD_COLS ) continue;
                            if(connectivity[(i+p)*BOARD_COLS+(j+q)]==1){
                                changed = 1;
                                connectivity[i*BOARD_COLS+j] = 1;
                            }
                        }
                    }
                }else if(c==3 || c==5){
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
    int i,k,currPlayer,nextPlayer,row,col;
    char** nextGameStates;
    int switches[7] = {0,1,3,2,4,5,6};

    *numNextGameStates = 0;
    for(i=0;i<BOARD_ROWS*BOARD_COLS;i++){
        if(gamestate[i]==1){
            *numNextGameStates+=2;
        }
    }

    if(*numNextGameStates==0){
        return NULL;
    }

    nextGameStates = (char**)malloc((*numNextGameStates)*sizeof(char*));
    i=0;
    for(k=0;k<BOARD_ROWS*BOARD_COLS;k++){
        if(gamestate[k]==1){
            nextGameStates[2*i] = (char*)malloc(GAMESTATE_SIZE*sizeof(char)+1);
            nextGameStates[2*i+1] = (char*)malloc(GAMESTATE_SIZE*sizeof(char)+1);
            memcpy(nextGameStates[2*i],gamestate,GAMESTATE_SIZE*sizeof(char)+1);
            memcpy(nextGameStates[2*i+1],gamestate,GAMESTATE_SIZE*sizeof(char)+1);
            currPlayer = gamestate[GAMESTATE_SIZE-1];
            nextPlayer = currPlayer==1?2:1;
            nextGameStates[2*i][GAMESTATE_SIZE-1] = nextPlayer;
            nextGameStates[2*i+1][GAMESTATE_SIZE-1] = nextPlayer;
            nextGameStates[2*i][k] = currPlayer+1;
            nextGameStates[2*i+1][k] = nextPlayer+3;
            row = k/BOARD_COLS;
            col = k%BOARD_COLS;
            if(row>0){
                nextGameStates[2*i+1][k-BOARD_COLS  ] = switches[(int)nextGameStates[2*i+1][k-BOARD_COLS  ]];
                if(col>0){
                    nextGameStates[2*i+1][k-BOARD_COLS-1] = switches[(int)nextGameStates[2*i+1][k-BOARD_COLS-1]];
                }
                if(col<BOARD_COLS-1){
                    nextGameStates[2*i+1][k-BOARD_COLS+1] = switches[(int)nextGameStates[2*i+1][k-BOARD_COLS+1]];
                }
            }
            if(row<BOARD_ROWS-1){
                nextGameStates[2*i+1][k+BOARD_COLS  ] = switches[(int)nextGameStates[2*i+1][k+BOARD_COLS  ]];
                if(col>0){
                    nextGameStates[2*i+1][k+BOARD_COLS-1] = switches[(int)nextGameStates[2*i+1][k+BOARD_COLS-1]];
                }
                if(col<BOARD_COLS-1){
                    nextGameStates[2*i+1][k+BOARD_COLS+1] = switches[(int)nextGameStates[2*i+1][k+BOARD_COLS+1]];
                }
            }
            if(col>0){
                nextGameStates[2*i+1][k           -1] = switches[(int)nextGameStates[2*i+1][k           -1]];
            }
            if(col<BOARD_COLS-1){
                nextGameStates[2*i+1][k           +1] = switches[(int)nextGameStates[2*i+1][k           +1]];
            }
            i++;
        }
    }
    //printf("nextGameStates: %p\n",nextGameStates);
    return nextGameStates;
}

float* unweightedWinStatistics(char* gamestate){
    int k,i,j,q,whiteWins,blackWins,winner,numEmptyLocs,currPlayer,nextPlayer,row,col,simulations;
    int switches[7] = {0,1,3,2,4,5,6};
    int emptyLocs[96];
    char* playingGame;
    float* winStats;

    winStats = (float*)malloc(PLAYERS*sizeof(float));
    playingGame = (char*)malloc(GAMESTATE_SIZE*sizeof(char)+1);

    simulations = 0;
    whiteWins = 0;
    blackWins = 0;

    for(k=0;k<NUM_SIMULATIONS;k++){
        memcpy(playingGame,gamestate,GAMESTATE_SIZE*sizeof(char)+1);
        while(-1 == (winner=getWinner(playingGame))){
            numEmptyLocs = 0;
            q = 0;
            for(i=0;i<BOARD_ROWS*BOARD_COLS;i++){
                if(playingGame[i]==1){
                    numEmptyLocs++;
                    emptyLocs[q] = i;
                    q++;
                }
            }
            if(numEmptyLocs==0){
                winner = playingGame[GAMESTATE_SIZE-1];
                break;
            }
            i = rand()%numEmptyLocs;
            j = emptyLocs[i];
            currPlayer = playingGame[GAMESTATE_SIZE-1];
            nextPlayer = currPlayer==1?2:1;
            if(rand()%2==0){
                playingGame[j] = currPlayer+1;
            }else{
                row = j/BOARD_COLS;
                col = j%BOARD_COLS;
                playingGame[j] = nextPlayer+3;
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

int playRandomSimulation(char* gamestate){
    int i,j,k,winner,numEmptyLocs,currPlayer,nextPlayer,row,col;
    int switches[7] = {0,1,3,2,4,5,6};
    int emptyLocs[96];
    char* playingGame;

    playingGame = (char*)malloc(GAMESTATE_SIZE*sizeof(char)+1);
    memcpy(playingGame,gamestate,GAMESTATE_SIZE*sizeof(char)+1);

    while(-1 == (winner=getWinner(playingGame))){
        numEmptyLocs = 0;
        k = 0;
        for(i=0;i<BOARD_ROWS*BOARD_COLS;i++){
            if(playingGame[i]==1){
                numEmptyLocs++;
                emptyLocs[k] = i;
                k++;
            }
        }
        if(numEmptyLocs==0){
            winner = playingGame[GAMESTATE_SIZE-1];
            break;
        }
        i = rand()%numEmptyLocs;
        j = emptyLocs[i];
        currPlayer = playingGame[GAMESTATE_SIZE-1];
        nextPlayer = currPlayer==1?2:1;
        if(rand()%2==0){
            playingGame[j] = currPlayer+1;
        }else{
            row = j/BOARD_COLS;
            col = j%BOARD_COLS;
            playingGame[j] = nextPlayer+3;
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
    free(playingGame);
    return winner;
}

int simulateGame(char* gamestate){
    int i,j,winner,numNextGameStates,pickRandom,team,totalVisits,ret;
    double score,bestScore,lnTotalVisits;
    char* bestNextState;
    char** nextStates;
    khiter_t ptr;
    team = gamestate[GAMESTATE_SIZE-1];
    //printf("DEBUG (2.1.1)\n");
    if(-1 != (winner=getWinner(gamestate))){
        ptr = kh_get(str,simulationData,gamestate);
        if(ptr == kh_end(simulationData)){
            ptr = kh_put(str,simulationData,gamestate,&ret);
        }
        kh_value(simulationData,ptr).visits = 1;
        if(winner==3-team){
            kh_value(simulationData,ptr).wins = 1;
        }else{
            kh_value(simulationData,ptr).wins = 0;
        }
        return winner;
    }
    //printf("DEBUG (2.1.2)\n");
    nextStates = getLegalNextGameStates(gamestate,&numNextGameStates);
    //printf("DEBUG (2.1.3)\n");
    pickRandom = 0;
    totalVisits = 0;
    for(i=0;i<numNextGameStates;i++){
        //printf("simulationData=%p\n",simulationData);
        //printf("n_buckets=%i, keys=%p\n",simulationData->n_buckets,simulationData->keys);
        //printf("nextStates[i]=%s\n",nextStates[i]);
        ptr = kh_get(str,simulationData,nextStates[i]);
        //printf("DEBUG 2.1.3.1\n");
        if(ptr == kh_end(simulationData)){
            pickRandom = 1;
            break;
        }
        totalVisits += kh_value(simulationData,ptr).visits;
    }
    //printf("DEBUG (2.1.4)\n");
    bestNextState = NULL;
    if(!pickRandom){
        bestScore = 0.0;
        lnTotalVisits = log((double)totalVisits);
        for(i=0;i<numNextGameStates;i++){
            ptr = kh_get(str,simulationData,nextStates[i]);
            score = (double)kh_value(simulationData,ptr).wins/(double)kh_value(simulationData,ptr).visits +
                    1.41421356 * sqrt(lnTotalVisits/(double)kh_value(simulationData,ptr).visits);
            if(score>bestScore){
                bestScore = score;
                if(bestNextState!=NULL){
                    free(bestNextState);
                }
                bestNextState = nextStates[i];
            }else{
                free(nextStates[i]);
            }
        }
    }else{
        i = rand()%numNextGameStates;
        bestNextState = nextStates[i];
        for(j=0;j<numNextGameStates;j++){
            if(j!=i){
                free(nextStates[j]);
            }
        }
    }
    free(nextStates);
    if(bestNextState==NULL){
        winner = gamestate[GAMESTATE_SIZE-1];
        ptr = kh_get(str,simulationData,gamestate);
        if(ptr == kh_end(simulationData)){
            ptr = kh_put(str,simulationData,gamestate,&ret);
        }
        kh_value(simulationData,ptr).visits = 1;
        if(winner==team){
            kh_value(simulationData,ptr).wins = 1;
        }else{
            kh_value(simulationData,ptr).wins = 0;
        }
        return winner;
    }
    if(pickRandom){
        winner = playRandomSimulation(bestNextState);
        ptr = kh_get(str,simulationData,bestNextState);
        if(ptr == kh_end(simulationData)){
            ptr = kh_put(str,simulationData,bestNextState,&ret);
            kh_value(simulationData,ptr).visits = 1;
            if(winner==team){
                kh_value(simulationData,ptr).wins = 1;
            }else{
                kh_value(simulationData,ptr).wins = 0;
            }
        }else{
            kh_value(simulationData,ptr).visits++;
            if(winner==team){
                kh_value(simulationData,ptr).wins++;
            }
        }
    }else{
        winner = simulateGame(bestNextState);
        ptr = kh_get(str,simulationData,bestNextState);
        if(ptr == kh_end(simulationData)){
            printf("WE HAVE A PROBLEM (2).\n");
        }
        kh_value(simulationData,ptr).visits++;
        if(winner==team){
            kh_value(simulationData,ptr).wins++;
        }
    }
    //printf("DEBUG (2.1.7)\n");
    //free(bestNextState);
    //printf("DEBUG (2.1.8)\n");
    return winner;
}

char* getBestMove(char* gamestate){
    int i,sim,numNextGameStates,totalVisits;
    double score,bestScore,lnTotalVisits;
    char** nextStates;
    char* bestNextState;
    khiter_t ptr;
    //printf("DEBUG (2.1)\n");
    for(sim=0;sim<NUM_SIMULATIONS;sim++){
        simulateGame(gamestate);
    }
    //printf("DEBUG (2.2)\n");
    bestScore = 0;
    totalVisits = 0;
    bestNextState = NULL;
    nextStates = getLegalNextGameStates(gamestate,&numNextGameStates);
    //printf("DEBUG (2.3)\n");
    for(i=0;i<numNextGameStates;i++){
        ptr = kh_get(str,simulationData,nextStates[i]);
        if(ptr == kh_end(simulationData)) continue;
        totalVisits += kh_value(simulationData,ptr).visits;
    }
    //printf("DEBUG (2.4)\n");
    lnTotalVisits = log((double)totalVisits);
    for(i=0;i<numNextGameStates;i++){
        ptr = kh_get(str,simulationData,nextStates[i]);
        if(ptr == kh_end(simulationData)){
            free(nextStates[i]);
            continue;
        }
//        score = (double)kh_value(simulationData,ptr).wins/(double)kh_value(simulationData,ptr).visits +
//                1.41421356 * sqrt(lnTotalVisits/(double)kh_value(simulationData,ptr).visits);
        score = (double)kh_value(simulationData,ptr).visits;
        if(score>bestScore){
            bestScore = score;
            if(bestNextState!=NULL){
                free(bestNextState);
            }
            bestNextState = nextStates[i];
        }else{
            free(nextStates[i]);
        }
    }
    //printf("DEBUG (2.5)\n");
    free(nextStates);
    if(bestNextState==NULL){
        printf("WE HAVE A PROBLEM (1).\n");
    }
    //printf("DEBUG (2.6)\n");
    return bestNextState;
}

//Main
int main(int argc, char **argv){
    int winner;
    char* gamestate = (char*)malloc(GAMESTATE_SIZE*sizeof(char)+1);
    srand(time(NULL));
    simulationData = kh_init(str); //Setup the global map
    initializeGameState(gamestate);
    if(argc > 1){
        if(parseFileIntoGameState(gamestate, argv[1]) == 0){
            printf("File %s loaded successfully.\n",argv[1]);
        }else{
            printf("ERROR: File %s not loaded.\n",argv[1]);
            return 1;
        }
    }
    /*int i,j,numNextGameStates;
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
    }*/
    //printf("DEBUG (1)\n");
    while(-1 == (winner=getWinner(gamestate))){
        //printf("DEBUG (2)\n");
        gamestate = getBestMove(gamestate);
        //printf("DEBUG (3)\n");
        printGameState(gamestate,stdout);
        //printf("DEBUG (4)\n");
    }
    fprintf(stdout,"Winner: %i\n",winner);
    kh_destroy(str,simulationData);
    return 0;
}
