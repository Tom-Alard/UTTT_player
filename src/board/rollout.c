#include <assert.h>
#include "rollout.h"
#include "../nn/forward.h"


Square generateMove(Board* board, RNG* rng) {
    uint8_t currentBoard = board->state.currentBoard;
    uint8_t randomMoveIndex;
    if (currentBoard == ANY_BOARD) {
        randomMoveIndex = generateBoundedRandomNumber(rng, board->state.totalAmountOfOpenSquares);
        currentBoard = 0;
        while (currentBoard < 9 && randomMoveIndex - board->state.amountOfOpenSquaresBySmallBoard[currentBoard] >= 0) {
            randomMoveIndex -= board->state.amountOfOpenSquaresBySmallBoard[currentBoard++];
        }
    } else {
        randomMoveIndex = generateBoundedRandomNumber(rng, board->state.amountOfOpenSquaresBySmallBoard[currentBoard]);
    }
    uint16_t bitBoard = ~(board->state.player1.smallBoards[currentBoard] | board->state.player2.smallBoards[currentBoard]) & 511;
    return openSquares[bitBoard][currentBoard][randomMoveIndex];
}


void makeRandomTemporaryMove(Board* board, RNG* rng) {
    assert(board->state.winner == NONE && "makeRandomTemporaryMove: there is already a winner");
    Player player = getCurrentPlayer(board);
    uint16_t smallBoardsWithWinningMove = board->state.instantWinBoards[player] & board->state.instantWinSmallBoards[player];
    if (hasWinningMove(board, smallBoardsWithWinningMove)) {
        board->state.winner = board->state.currentPlayer + 1;
        return;
    }
    Square move = generateMove(board, rng);
    makeTemporaryMove(board, move);
}


#define EPT_RANDOM_MOVES 0
float rollout(Board* board, RNG* rng, Player player) {
    for (int i = 0; i < EPT_RANDOM_MOVES && getWinner(board) == NONE; i++) {
        makeRandomTemporaryMove(board, rng);
    }
    Winner winner = getWinner(board);
    if (winner != NONE) {
        return winner == DRAW? 0.5f : player + 1 == winner? 1.0f : 0.0f;
    }
    return 1.0f - neuralNetworkEval(board);
}