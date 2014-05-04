//
//  gameoflife.c
//  Tutorial
//
//  Created by Xiaoping Li on 4/25/14.
//  Copyright (c) 2014 edu.jhu.cs.xiaoping. All rights reserved.
//

/*
 * gameoflife: an mpi progam that mimic the
 * game of life
 * author: Xiaoping Li
 * version: 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "gameoflife.h"



int main (int argc, char** argv){
	
	/* simulation variables */
	int iterations = 64;
    
	/* MPI Standard variables */
	int num_procs;
	int ID;
    
	/* Messaging variables */
	MPI_Status stat;
	int prev;
	int next;
	int* inputData;
	int inputDatalen;
    
	/* MPI Setup */
	if( MPI_Init( &argc, &argv ) != MPI_SUCCESS ){
		printf ( "MPI_INit error\n" );
	}
	
	MPI_Comm_size( MPI_COMM_WORLD, &num_procs );
	MPI_Comm_rank( MPI_COMM_WORLD, &ID );
    
	/* Initialize process specific state  */
	next = ( ID + 1 ) % num_procs;  //relative next for multi-core
	prev = ID == 0 ? num_procs -1 : ID-1; //relative prev for multi-core
	inputDatalen = dimension * dimension / num_procs;  //data size 16*16
	inputData = malloc( inputDatalen * sizeof(int) );
	int i;
	for (i = 0; i < inputDatalen; i++){
		inputData[ i ] = global_grid[ ID * inputDatalen + i ];
	}
	/*multiprocessor : 2,4,8,16*/
    if((num_procs % 2 ==0) && (num_procs <= 16)){
		int iter;
		for(iter = 0; iter < iterations; iter++){
            
			/* messages from prev/next */
			int msgFromPrev[dimension];
			int msgFromNext[dimension];
			/* messages to prev/next */
			int msgToPrev[dimension];
			int msgToNext[dimension];
			for(i = 0; i < dimension; i++){
				msgToPrev[i] = inputData[i];
				msgToNext[i] = inputData[inputDatalen - dimension + i];
			}
			
            
			if( ID % 2 == 0){
				MPI_Send (&msgToNext, dimension, MPI_INT, next, 1, MPI_COMM_WORLD);
				MPI_Recv (&msgFromPrev,dimension,MPI_INT,prev, 1, MPI_COMM_WORLD, &stat);
                
				MPI_Send (&msgToPrev, dimension, MPI_INT, prev, 2, MPI_COMM_WORLD);
				MPI_Recv (&msgFromNext, dimension, MPI_INT, next, 2, MPI_COMM_WORLD, &stat);
			}
			else{
				MPI_Recv (&msgFromPrev,dimension,MPI_INT,prev, 1, MPI_COMM_WORLD, &stat);
				MPI_Send (&msgToNext, dimension, MPI_INT, next, 1, MPI_COMM_WORLD);
                
				MPI_Recv (&msgFromNext, dimension, MPI_INT, next, 2, MPI_COMM_WORLD, &stat);
				MPI_Send (&msgToPrev, dimension, MPI_INT, prev, 2, MPI_COMM_WORLD);
			}
			
			/* a message sandwiches: msgFromPrev,inputData,msgFromNext */
			int msgswch[inputDatalen+2*dimension];
			for( i = 0; i < (inputDatalen + 2 * dimension); i++){
				if(i < dimension){
					msgswch[ i ] = msgFromPrev[ i ];
				}else if (i < inputDatalen + dimension){
					msgswch[ i ] = inputData[i - dimension];
				}else{
					msgswch[ i ] = msgFromNext[ i - dimension - inputDatalen ];
				}
			}
			
			/* update inputData */
			int row,col,prevr,prevc,nextr,nextc,numberOfOne;
			for (i = 0; i < inputDatalen; i++){
				row = (i / dimension) + 1;
				col = ( i % dimension );
				prevr = row - 1;
				nextr = row + 1;
				prevc = (col==0) ? (dimension -1) : (col -1) ;
				nextc = (col==(dimension-1)) ? (0) : (col + 1);
                
				numberOfOne = 0;
				numberOfOne += msgswch[prevr*dimension + prevc];
				numberOfOne += msgswch[prevr*dimension + col];
				numberOfOne += msgswch[prevr*dimension + nextc];
				numberOfOne += msgswch[row*dimension + prevc];
				numberOfOne += msgswch[row*dimension + nextc];
				numberOfOne += msgswch[nextr*dimension + prevc];
				numberOfOne += msgswch[nextr*dimension + col];
				numberOfOne += msgswch[nextr*dimension + nextc];
                
				if((inputData[i]==0)&&(numberOfOne==3)){
					inputData[i] = 1;
				}else if((inputData[i]==1)&&((numberOfOne==2)||(numberOfOne==3))){
					inputData[i] = 1;
				}else{
					inputData[i] = 0;
				}
			}
            
			/* pass new inputData to master proc */
			if (ID == 0){
				int slavemsg[num_procs-1][inputDatalen];
				//int slavemsg[inputDatalen];
				for( i = 1; i < num_procs; i++){
					MPI_Recv(&slavemsg[i-1],inputDatalen,MPI_INT,i,3,MPI_COMM_WORLD,&stat);
				}
				int updatedgrid[dimension * dimension];
				for (i = 0; i < inputDatalen; i++){
					updatedgrid[i] = inputData[i];
				}
				for (i = inputDatalen; i < dimension * dimension; i++){
					updatedgrid[i] = slavemsg[(i-inputDatalen)/inputDatalen][i%inputDatalen];
				}
				printf("Iteration %d: updated grid\n",iter);
				gridprint(updatedgrid,dimension*dimension);
			}else{
				MPI_Send(inputData,inputDatalen,MPI_INT,0,3,MPI_COMM_WORLD);
			}
		}
	}
    
	/* single processor, no message passing */
	else if ( num_procs == 1){
		int r,c;
		int prevrc[16]={15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
		int nextrc[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0};
		/* data after updates */
		int outputData[ dimension * dimension ];
		int numberOfOne;
		for (i = 0; i < iterations; i++ ){
			for (r = 0; r < dimension; r++){
				for ( c = 0; c < dimension; c++){
					numberOfOne = 0;
					//eight value check
					numberOfOne += inputData[prevrc[r]*dimension + prevrc[c]];
					numberOfOne += inputData[r*dimension + prevrc[c]];
					numberOfOne += inputData[nextrc[r]*dimension + prevrc[c]];
					numberOfOne += inputData[prevrc[r]*dimension + c];
					numberOfOne += inputData[nextrc[r]*dimension + c];
					numberOfOne += inputData[prevrc[r]*dimension + nextrc[c]];
					numberOfOne += inputData[r*dimension + nextrc[c]];
					numberOfOne += inputData[nextrc[r]*dimension + nextrc[c]];
					
					/*implement the logic of game of life*/
					if ((inputData[r*dimension + c] ==0)&&(numberOfOne==3)){
						outputData[r*dimension+c] = 1;
					}else if ((inputData[r*dimension + c]==1)&&((numberOfOne==2)||(numberOfOne==3))){
						outputData[r*dimension+c] = 1;
					}else{
						outputData[r*dimension+c] = 0;
					}
				}
			}
			for(r = 0; r < dimension*dimension; r++){
				inputData[r] = outputData[r];
			}
			
			printf("Iteration %d: updated grid\n",i);
			gridprint(inputData,inputDatalen);
		}
	}
    
	/* Terminate execution environment*/
	MPI_Finalize();
	return 0;
}

/*print out the grid for each iteration*/
void gridprint (int* grid, int length){
	if (grid == NULL){
		return;
	}else{
		int nrows = length / dimension;
		int i,j;
		for( i = 0; i < nrows ; i++){
			for( j = 0; j < dimension; j++){
				printf( "%d " , grid[ i * dimension + j ] );
			}
			printf("\n");
		}
	}
}
