#include <stdio.h>
#include <sys/time.h>
#include "find_next_move_tests.h"
#include "../../src/mcts/find_next_move.h"
#include "../test_util.h"


void findNextMoveDoesNotChangeBoard() {
    Board* board = createBoard();
    int rootIndex = createMCTSRootNode(board);
    while (board->state.winner == NONE) {
        Winner winnerBefore = board->state.winner;
        Square movesBefore[TOTAL_SMALL_SQUARES];
        int8_t amountMovesBefore = generateMoves(board, movesBefore);
        findNextMove(board, rootIndex, 0.005);
        Square nextMove = getMostPromisingMove(board, &board->nodes[rootIndex]);
        myAssert(winnerBefore == board->state.winner);
        Square movesAfter[TOTAL_SMALL_SQUARES];
        int8_t amountMovesAfter = generateMoves(board, movesAfter);
        myAssert(amountMovesBefore == amountMovesAfter);
        for (int i = 0; i < amountMovesBefore; i++) {
            myAssert(squaresAreEqual(movesBefore[i], movesAfter[i]));
        }
        makePermanentMove(board, nextMove);
        rootIndex = updateRoot(&board->nodes[rootIndex], board, nextMove);
    }
    freeBoard(board);
}


void findNextMoveUsesAsMuchTimeAsWasGiven() {
    Board* board = createBoard();
    int rootIndex = createMCTSRootNode(board);
    while (board->state.winner == NONE) {
        struct timeval start, end;
        int timeMs = 100;
        gettimeofday(&start, NULL);
        findNextMove(board, rootIndex, timeMs / 1000.0);
        Square nextMove = getMostPromisingMove(board, &board->nodes[rootIndex]);
        gettimeofday(&end, NULL);
        double elapsedTime = (double) (end.tv_sec - start.tv_sec) * 1000.0;
        elapsedTime += (double) (end.tv_usec - start.tv_usec) / 1000.0;
        myAssert(elapsedTime >= timeMs);
        if (elapsedTime >= 1.1*timeMs) {
            printf("%f\n", elapsedTime);
        }
        makePermanentMove(board, nextMove);
        rootIndex = updateRoot(&board->nodes[rootIndex], board, nextMove);
    }
    freeBoard(board);
}


void runFindNextMoveTests() {
    printf("\tfindNextMoveDoesNotChangeBoard...\n");
    findNextMoveDoesNotChangeBoard();
    printf("\tfindNextMoveUsesAsMuchTimeAsWasGiven...\n");
    findNextMoveUsesAsMuchTimeAsWasGiven();
}