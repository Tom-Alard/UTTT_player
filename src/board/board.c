#include <assert.h>
#include "board.h"
#include "../mcts/mcts_node.h"
#include "../misc/util.h"


Winner calculateWinner(uint16_t player1BigBoard, uint16_t player2BigBoard, Player currentPlayer) {
    if (currentPlayer == PLAYER1 && isWin(player1BigBoard & (player1BigBoard ^ player2BigBoard))) {
        return WIN_P1;
    }
    if (currentPlayer == PLAYER2 && isWin(player2BigBoard & (player1BigBoard ^ player2BigBoard))) {
        return WIN_P2;
    }
    if ((player1BigBoard | player2BigBoard) == 511) {
        int player1AmountBoardsWon = __builtin_popcount(player1BigBoard);
        int player2AmountBoardsWon = __builtin_popcount(player2BigBoard);
        return player1AmountBoardsWon > player2AmountBoardsWon
               ? WIN_P1
               : player1AmountBoardsWon < player2AmountBoardsWon
                 ? WIN_P2
                 : DRAW;
    }
    return NONE;
}


#define MEGABYTE (1024*1024ULL)
#define NODES_SIZE (512*MEGABYTE)
#define NUM_NODES (NODES_SIZE / sizeof(MCTSNode))
Board* createBoard() {
    Board* board = safeMalloc(sizeof(Board));
    initializePlayerBitBoard(&board->state.player1);
    initializePlayerBitBoard(&board->state.player2);
    board->state.currentPlayer = PLAYER1;
    board->state.currentBoard = ANY_BOARD;
    board->state.winner = NONE;
    board->state.ply = 0;
    board->stateCheckpoint = board->state;
    board->nodes = safeMalloc(NUM_NODES * sizeof(MCTSNode));
    board->currentNodeIndex = 0;
    board->me = PLAYER2;
    return board;
}


void freeBoard(Board* board) {
    safeFree(board->nodes);
    safeFree(board);
}


int allocateNodes(Board* board, uint8_t amount) {
    int result = (amount > NUM_NODES - board->currentNodeIndex)? 0 : board->currentNodeIndex;
    board->currentNodeIndex = result + amount;
    return result;
}


uint16_t extractCombinedSmallBoard(Board* board, uint8_t boardIndex) {
    return extractSmallBoard(&board->state.player1, boardIndex) | extractSmallBoard(&board->state.player2, boardIndex);
}


int8_t generateMoves(Board* board, Square moves[TOTAL_SMALL_SQUARES]) {
    if (board->state.winner != NONE) {
        return 0;
    }
    __uint128_t mask;
    if (board->state.currentBoard == ANY_BOARD) {
        mask = 0;
        uint16_t undecidedSmallBoards = ~(board->state.player1.bigBoard | board->state.player2.bigBoard) & 511;
        while (undecidedSmallBoards) {
            uint8_t boardIndex = __builtin_ffs(undecidedSmallBoards) - 1;
            mask |= (__uint128_t) 511 << (9 * boardIndex);
            undecidedSmallBoards &= undecidedSmallBoards - 1;
        }
    } else {
        mask = (__uint128_t) 511 << (9 * board->state.currentBoard);
    }
    __uint128_t occupiedSquares = board->state.player1.marks | board->state.player2.marks;
    __uint128_t legalMoves = ~occupiedSquares & mask;
    uint64_t lowBits = legalMoves;
    uint64_t highBits = legalMoves >> 64;
    int8_t i = 0;
    while (lowBits) {
        int squareIndex = __builtin_ffsl((int64_t) lowBits) - 1;
        Square s = {squareIndex / 9, squareIndex % 9};
        moves[i++] = s;
        lowBits &= lowBits - 1;
    }
    while (highBits) {
        int squareIndex = __builtin_ffsl((int64_t) highBits) + 63;
        Square s = {squareIndex / 9, squareIndex % 9};
        moves[i++] = s;
        highBits &= highBits - 1;
    }
    return i;
}


uint8_t getNextBoard(Board* board, uint8_t previousPosition) {
    bool smallBoardIsDecided = BIT_CHECK(board->state.player1.bigBoard | board->state.player2.bigBoard, previousPosition);
    return smallBoardIsDecided ? ANY_BOARD : previousPosition;
}


bool nextBoardIsEmpty(Board* board) {
    uint8_t currentBoard = board->state.currentBoard;
    return currentBoard != ANY_BOARD && extractCombinedSmallBoard(board, currentBoard) == 0;
}


void revertToCheckpoint(Board* board) {
    board->state = board->stateCheckpoint;
}


void updateCheckpoint(Board* board) {
    board->stateCheckpoint = board->state;
}


void makeTemporaryMove(Board* board, Square square) {
    assert(square.board == board->state.currentBoard || board->state.currentBoard == ANY_BOARD);
    assert(!BIT_CHECK(extractCombinedSmallBoard(board, square.board), square.position));
    assert(board->state.winner == NONE);

    PlayerBitBoard* p1 = &board->state.player1;
    if (setSquareOccupied(p1 + board->state.currentPlayer, p1 + !board->state.currentPlayer, square)) {
        board->state.winner = calculateWinner(board->state.player1.bigBoard, board->state.player2.bigBoard, board->state.currentPlayer);
    }
    board->state.currentPlayer ^= 1;
    board->state.currentBoard = getNextBoard(board, square.position);
    board->state.ply++;
}


void makePermanentMove(Board* board, Square square) {
    makeTemporaryMove(board, square);
    updateCheckpoint(board);
}


Winner getWinnerAfterMove(Board* board, Square square) {
    State tempCheckpoint = board->state;
    makeTemporaryMove(board, square);
    Winner winner = board->state.winner;
    board->state = tempCheckpoint;
    return winner;
}
