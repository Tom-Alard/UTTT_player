#include <stdio.h>
#include "profile_simulations.h"
#include "../src/handle_turn.h"


void profileSimulations() {
    const int runs = 100;
    int totalSims = 0;
    for (int i = 0; i < runs; i++) {
        Board* board = createBoard();
        MCTSNode* root = createMCTSRootNode();
        Square square = createSquare(1, 0);
        discoverChildNodes(root, board);
        root = updateRoot(root, board, square);
        makePermanentMove(board, square);
        totalSims += findNextMove(board, root, 0.1);
        freeMCTSTree(root);
        freeBoard(board);
    }
    printf("%d\n", totalSims / runs);
}
