#include <stdio.h>
#include <sys/time.h>
#include "find_next_move_tests.h"
#include "../../src/mcts/find_next_move.h"
#include "../test_util.h"


void findNextMoveDoesNotChangeBoard() {
    Board* board = createBoard();
    MCTSNode* root = createMCTSRootNode();
    RNG rng;
    seedRNG(&rng, 69, 420);
    while (getWinner(board) == NONE) {
        Winner winnerBefore = getWinner(board);
        Square movesArray[TOTAL_SMALL_SQUARES];
        int8_t amountMovesBefore;
        Square* movesBefore = generateMoves(board, movesArray, &amountMovesBefore);
        findNextMove(board, root, &rng, 0.005);
        Square nextMove = getMostPromisingMove(root);
        myAssert(winnerBefore == getWinner(board));
        int8_t amountMovesAfter;
        Square* movesAfter = generateMoves(board, movesArray, &amountMovesAfter);
        myAssert(amountMovesBefore == amountMovesAfter);
        for (int i = 0; i < amountMovesBefore; i++) {
            myAssert(squaresAreEqual(movesBefore[i], movesAfter[i]));
        }
        makePermanentMove(board, nextMove);
        root = updateRoot(root, board, nextMove, &rng);
    }
    freeMCTSTree(root);
    freeBoard(board);
}


void findNextMoveUsesAsMuchTimeAsWasGiven() {
    Board* board = createBoard();
    MCTSNode* root = createMCTSRootNode();
    RNG rng;
    seedRNG(&rng, 69, 420);
    while (getWinner(board) == NONE) {
        struct timeval start, end;
        int timeMs = 100;
        gettimeofday(&start, NULL);
        findNextMove(board, root, &rng, timeMs / 1000.0);
        Square nextMove = getMostPromisingMove(root);
        gettimeofday(&end, NULL);
        double elapsedTime = (double) (end.tv_sec - start.tv_sec) * 1000.0;
        elapsedTime += (double) (end.tv_usec - start.tv_usec) / 1000.0;
        myAssert(elapsedTime >= timeMs);
        if (elapsedTime >= 1.1*timeMs) {
            printf("%f\n", elapsedTime);
        }
        makePermanentMove(board, nextMove);
        root = updateRoot(root, board, nextMove, &rng);
    }
    freeMCTSTree(root);
    freeBoard(board);
}


void runFindNextMoveTests() {
    printf("\tfindNextMoveDoesNotChangeBoard...\n");
    findNextMoveDoesNotChangeBoard();
    printf("\tfindNextMoveUsesAsMuchTimeAsWasGiven...\n");
    findNextMoveUsesAsMuchTimeAsWasGiven();
}