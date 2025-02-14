#include <bits/stdc++.h>
#include <bitset>
#include <iostream>

using namespace std;

// todo: genetic evolution algorithm to tune parameters
// nnue eval

/*when in the uint64, these are the following board indicies
where the number shown is the index given 1 << n
0b
* 62 61 60 59 58 57  (rightmost column)
* 55 54 53 52 51 50
* 48 47 46 45 44 43
* 41 40 39 38 37 36
* 34 33 32 31 30 29
* 27 26 25 24 23 22
* 20 19 18 17 16 15  (leftmost column)


in uint64_t, this looks like
0b 0 000000 0 000000 0 000000 0 000000 0 000000 0 000000 0 000000 000000000000000
      col6     col5     col4     col3     col2     col1     col0 

board rotated 90 degrees counterclockwise (rightmost in each row == bottom of actual game)
0b
     0000000 <- col 6
     0000000
     0000000
top  0000000 bottom
     0000000
     0000000
     0000000 <- col 0
000000000000000 <- 15 free bits
*/

// displays the uint in its raw form
void displayUINT(uint64_t thing) {
    cout << bitset<8 * sizeof(thing)>(thing) << endl;
}

/////////////////////////IF THE COLORS DON'T WORK, UNCOMMENT THE X AND O AND
/// COMMENT THE ESCAPE CODES
void showCurrentBoardState(uint64_t combBoard, uint64_t myBoard) {
    const string blueX = "\033[34mX\033[0m";
    const string redO = "\033[31mX\033[0m";
    const string dash = ".";

    // const string blueX = "X";
    // const string redO = "O";
    //   const string dash = "-";
    cout << "0 1 2 3 4 5 6" << endl;
    int pos = 20;
    for (int j = 0; j < 6; ++j) {
        for (int i = 0; i < 7; ++i) {
            if ((myBoard >> pos) & 1) {
                cout << blueX << " ";
            } else if ((combBoard >> pos) & 1) {
                cout << redO << " ";
            } else {
                cout << dash << " ";
            }
            pos += 7;
        }
        pos -= 50;
        cout << endl;
    }
}

class Plastanium {
   public:
    Plastanium(int depth);
    int getBestMove(uint64_t combBoard, uint64_t myBoard);
    uint64_t getMove(uint64_t combBoard, int col);
    bool isLoss(uint64_t enemyBoard);

   private:
    int searchDepth = 0;

    const int gameOverScore = 450;
	const uint32_t magicNumber = 4286297452; //change to reduce collisions or change method
	
    int gameDepth = 0;

	bool isSymmetric(uint64_t positionKey); //to do
    bool isWin(uint64_t combBoard, uint64_t myBoard);
    uint64_t findThreats(uint64_t combBoard, uint64_t myBoard);
    uint64_t positionToKey(uint64_t combBoard, uint64_t myBoard);
    int staticEval(uint64_t combBoard, uint64_t myBoard);
    int negamax(int depth, int alpha, int beta, uint64_t combBoard, uint64_t myBoard);

	//transposition table
    //128 bits
    //to do: use unused bits in the key for storing depth and score
    //max score is 511 (change scoring values) (9 bits)
    //depth uses 6 bits (max 63 depth but there are only 42 positions anyways)
    //change struct to just uint64

    //to do: figure out how to reduce collisions
	struct posData {
        uint64_t key = 0;
        int32_t score = 0;
		int32_t depth = 0;
    };
	vector<posData> transpositionTable;
};

Plastanium::Plastanium(int depth) { 
	searchDepth = depth;
	// 64mb of table space (roughly) will double when struct is replaced with uint64
	transpositionTable.resize(1<<22);
}

// return the uint64_t with 1 bit set corresponding to the new move position
// (will return 0 if not possible)
uint64_t Plastanium::getMove(uint64_t combBoard, int col) {
    const uint64_t bottomPosition = 0b0000001000000100000010000001000000100000010000001000000000000000;
    const uint64_t colMask[7] = {
        0b0000000000000000000000000000000000000000000111111000000000000000,
        0b0000000000000000000000000000000000001111110000000000000000000000,
        0b0000000000000000000000000000011111100000000000000000000000000000,
        0b0000000000000000000000111111000000000000000000000000000000000000,
        0b0000000000000001111110000000000000000000000000000000000000000000,
        0b0000000011111100000000000000000000000000000000000000000000000000,
        0b0111111000000000000000000000000000000000000000000000000000000000};
    return (combBoard + bottomPosition) & colMask[col];
}

// makes unique 64 bit key with bits set for all of my pieces and bits set for all possible next moves
uint64_t Plastanium::positionToKey(uint64_t combBoard, uint64_t myBoard) {
    const uint64_t bottomPosition = 0b0000001000000100000010000001000000100000010000001000000000000000;
    return combBoard + myBoard + bottomPosition;
}

// is the position lost if its my turn
bool Plastanium::isLoss(uint64_t enemyBoard) {
    //finds 4-in-a-rows
    uint64_t shiftBoard = enemyBoard & (enemyBoard >> 6);  // diagonal negative slope
    if (shiftBoard & (shiftBoard >> 12)) return true;
    shiftBoard = enemyBoard & (enemyBoard >> 7);  // horizontal
    if (shiftBoard & (shiftBoard >> 14)) return true;
    shiftBoard = enemyBoard & (enemyBoard >> 8);  // diagonal positive slope
    if (shiftBoard & (shiftBoard >> 16)) return true;
    shiftBoard = enemyBoard & (enemyBoard >> 1);  // vertical
    if (shiftBoard & (shiftBoard >> 2)) return true;
    return false;
}

// is the position winning if its my turn
bool Plastanium::isWin(uint64_t combBoard, uint64_t myBoard) {
    uint64_t newMoves = combBoard + 0b0000001000000100000010000001000000100000010000001000000000000000;

    //we can check multiple columns at once as long as they are far apart enough that they won't interact
    return isLoss(myBoard | (newMoves & 0b0000000000000001111110000000000000000000000111111000000000000000)) ||
           isLoss(myBoard | (newMoves & 0b0000000011111100000000000000000000001111110000000000000000000000)) ||
           isLoss(myBoard | (newMoves & 0b0111111000000000000000000000011111100000000000000000000000000000)) ||
           isLoss(myBoard | (newMoves & 0b0000000000000000000000111111000000000000000000000000000000000000));
}

// finds all exposed 3-in-a-row threats
uint64_t Plastanium::findThreats(uint64_t combBoard, uint64_t myBoard) {
    // all possible positions on the board
    const uint64_t maskPossibleMoves = 0b0111111011111101111110111111011111101111110111111000000000000000;
    uint64_t totalThreatBoard = 0;

    // positive slope diagonal
    uint64_t tempBoard = myBoard & (myBoard >> 8);
    uint64_t shiftBoard = tempBoard;
    shiftBoard &= (shiftBoard >> 8);
    shiftBoard >>= 8;
    shiftBoard |= (shiftBoard << 32);
    totalThreatBoard |= shiftBoard;
    // positive slope split (XX X)
    shiftBoard = tempBoard;
    shiftBoard >>= 16;
    shiftBoard &= myBoard;
    shiftBoard <<= 8;
    totalThreatBoard |= shiftBoard;
    // positive slope split (X XX)
    shiftBoard = tempBoard;
    shiftBoard <<= 24;
    shiftBoard &= myBoard;
    shiftBoard >>= 8;
    totalThreatBoard |= shiftBoard;

    // negative slope diagonal
    tempBoard = myBoard & (myBoard >> 6);
    shiftBoard = tempBoard;
    shiftBoard &= (shiftBoard >> 6);
    shiftBoard >>= 6;
    shiftBoard |= (shiftBoard << 24);
    totalThreatBoard |= shiftBoard;
    // negative slope split (XX X)
    shiftBoard = tempBoard;
    shiftBoard >>= 12;
    shiftBoard &= myBoard;
    shiftBoard <<= 6;
    totalThreatBoard |= shiftBoard;
    // negative slope split (X XX)
    shiftBoard = tempBoard;
    shiftBoard <<= 18;
    shiftBoard &= myBoard;
    shiftBoard >>= 6;
    totalThreatBoard |= shiftBoard;

    // horizontal
    tempBoard = myBoard & (myBoard >> 7);
    shiftBoard = tempBoard;
    shiftBoard &= (shiftBoard >> 7);
    shiftBoard >>= 7;
    shiftBoard |= (shiftBoard << 28);
    totalThreatBoard |= shiftBoard;
    // horizontal split (XX X)
    shiftBoard = tempBoard;
    shiftBoard >>= 14;
    shiftBoard &= myBoard;
    shiftBoard <<= 7;
    totalThreatBoard |= shiftBoard;
    // horizontal split (X XX)
    shiftBoard = tempBoard;
    shiftBoard <<= 21;
    shiftBoard &= myBoard;
    shiftBoard >>= 7;
    totalThreatBoard |= shiftBoard;

    // vertical
    shiftBoard = myBoard;
    shiftBoard &= (shiftBoard >> 1);
    shiftBoard &= (shiftBoard >> 1);
    shiftBoard >>= 1;
    totalThreatBoard |= shiftBoard;

    // remove threats that are already blocked
    totalThreatBoard &= (~combBoard);
    totalThreatBoard &= maskPossibleMoves;

    return totalThreatBoard;
}

// statically evaluate the position
int Plastanium::staticEval(uint64_t combBoard, uint64_t myBoard) {
    const uint64_t maskPossibleMoves = 0b0111111011111101111110111111011111101111110111111000000000000000;
    const uint64_t bottomPosition = 0b0000001000000100000010000001000000100000010000001000000000000000;
    const uint64_t colMask[7] = {
        0b0000000000000000000000000000000000000000000111111000000000000000,
        0b0000000000000000000000000000000000001111110000000000000000000000,
        0b0000000000000000000000000000011111100000000000000000000000000000,
        0b0000000000000000000000111111000000000000000000000000000000000000,
        0b0000000000000001111110000000000000000000000000000000000000000000,
        0b0000000011111100000000000000000000000000000000000000000000000000,
        0b0111111000000000000000000000000000000000000000000000000000000000};

    uint64_t myThreats = findThreats(combBoard, myBoard);
    uint64_t enemyThreats = findThreats(combBoard, combBoard ^ myBoard);
    uint64_t combThreats = myThreats | enemyThreats;
    uint64_t possibleMoves = combBoard + bottomPosition;

    int positionEval = 0;

    // basic positional evaluation
    positionEval += (__builtin_popcountll(myThreats) - __builtin_popcountll(enemyThreats)) * 7;

    // calculate who wins when the game goes on till the end
    int freePositions = 0;

    // if there are no threats on a column, remove the moves for that column
    for (int i = 0; i < 7; ++i) {
        if (!(combThreats & colMask[i])) {
            possibleMoves &= ~colMask[i];
            freePositions += __builtin_popcountll((~combBoard) & colMask[i]);
        }
    }

    // if even number of positions, current player has
    // to go first on threats, otherwise current player goes second
    bool isMyTurn = !(freePositions & 1);

    // shift all threats 1 spot downwards to find all positions that the opposing
    // player cannot place (or they will lose instantly)
    enemyThreats = (~(enemyThreats >> 1)) & maskPossibleMoves;
    myThreats = (~(myThreats >> 1)) & maskPossibleMoves;
    combThreats = (~(combThreats >> 1)) & maskPossibleMoves;

    // check player turns on threat columns
    while (true) {
        uint64_t idealMoves = possibleMoves & combThreats;
        uint64_t safeMoves = possibleMoves & (isMyTurn ? enemyThreats : myThreats);
        if (idealMoves) {
            possibleMoves += (idealMoves & ~(idealMoves - 1));
        } else if (safeMoves) {
            possibleMoves += (safeMoves & ~(safeMoves - 1));
        } else {
            break;
        }

        ++freePositions;
        isMyTurn = !isMyTurn;
    }

    // if we carry out the player moves on the threats and one player
    // cannot move anymore even though there are moves left, they have lost
    if (possibleMoves & maskPossibleMoves) {
        if (isMyTurn) {
            positionEval -= 8;// tune values
        } else {
            positionEval += 8;
        }
    }
    return positionEval;
}

// alpha beta minimax
int Plastanium::negamax(int depth, int alpha, int beta, uint64_t combBoard, uint64_t myBoard) {
    uint64_t positionKey = positionToKey(combBoard, myBoard);
	
    //based on the size of the transposition table
	uint64_t tableIndex = ((positionKey >> 15) * magicNumber) >> 43;

	int curDepth = gameDepth + searchDepth - depth;
	posData &tableVal = transpositionTable[tableIndex];

	if(tableVal.key == positionKey && tableVal.depth >= curDepth){
		return tableVal.score;
	}

    // prefer to play moves that win faster and lose slower
    int winScore = gameOverScore + depth - gameDepth;
    if (isWin(combBoard, myBoard)) {
		tableVal.depth = 63;//if its a winning position, it doesn't matter what depth we looked at
		tableVal.score = winScore;
        return winScore;
    }

    // if depth is zero, evaluate static position
    if (depth == 0) {

		int evaluation = staticEval(combBoard, myBoard);

		tableVal.key = positionKey;
		tableVal.score = evaluation;
		tableVal.depth = curDepth;
        return evaluation;
    }

    int bestEval = INT_MIN;
    bool foundMove = false;

    //to do
	//check if this position is symmetric, if so, we only need to evaluate one side (namely columns 0, 1, 2, 3)
    //maybe only do check if above certain depth if check is too slow

    // recursion part
    const int searchOrder[7] = {3, 4, 2, 5, 1, 6, 0};
    for (int i = 0; i < 7; ++i) {
        uint64_t newMove = getMove(combBoard, searchOrder[i]);
        if (newMove == 0) {
            continue;
        }
        foundMove = true;

        int eval = -negamax(depth - 1, -beta, -alpha, combBoard | newMove, combBoard ^ myBoard);
        bestEval = max(bestEval, eval);

        if (bestEval > gameOverScore - 50) {
			tableVal.key = positionKey;
			tableVal.depth = 63;
			tableVal.score = bestEval;
            return bestEval;
        }

        alpha = max(bestEval, alpha);

        if (alpha >= beta) {
            break;
        }
    }

	tableVal.key = positionKey;
	tableVal.depth = curDepth;

    // if no move was found and the position isn't won (by earlier checks), the game must be drawn
	tableVal.score = foundMove ? bestEval : 0;
    return foundMove ? bestEval : 0;
}

int Plastanium::getBestMove(uint64_t combBoard, uint64_t myBoard) {
    const int searchOrder[7] = {3, 4, 2, 5, 1, 6, 0};
    int bestCol = -1;
    int bestEval = INT_MIN;
    gameDepth = __builtin_popcountll(combBoard);

	// if we can win on this turn, check which column wins
    // this check is important because we assume any position given in the full
    // search is not a game over position
    if (isWin(combBoard, myBoard)) {
        for (int i = 0; i < 7; ++i) {
            uint64_t newMove = getMove(combBoard, i);
            if (newMove == 0) {
                continue;
            }
            uint64_t newBoard = (myBoard | newMove);

            if (isLoss(newBoard)) {
                return i;
            }
        }
    }

    // full search
    for (int i = 0; i < 7; ++i) {
        uint64_t newMove = getMove(combBoard, searchOrder[i]);
        if (newMove == 0) {
            continue;
        }

        uint64_t newCombBoard = (combBoard | newMove);
        uint64_t enemyBoard = (combBoard ^ myBoard);

        // don't use int_max and int_min cause of overflow when negating
        int curEval = -negamax(searchDepth - 1, -99999, 99999, newCombBoard, enemyBoard);
        if (curEval > bestEval) {
            bestCol = searchOrder[i];
            bestEval = curEval;
            if (bestEval > gameOverScore - 50) {
                break;
            }
        }
    }

    cout << "best eval " << bestEval << endl; // see what the bot thinks

    return bestCol;
}

uint64_t combinedBoard = 0;
uint64_t playerBoard = 0;

int main() {
    cout << "go depth: ";
    int botDepth = 1;
    cin >> botDepth;
    Plastanium bot1(botDepth);

    bool selfPlay = false;
    bool goFirst = false;

    string goFirstInput = "";
    cout << endl
         << "go first? (y/n): ";
    cin >> goFirstInput;
    goFirst = goFirstInput == "y" ? true : false;

    if (!goFirst) {//if the bot goes first, always play on middle column
        combinedBoard = 0b0000000000000000000000000001000000000000000000000000000000000000;
    }

    while (true) {
        uint64_t newMove = 0;
        int moveCol = -1;

        if (selfPlay) {
            cout << "bot 1's turn" << endl;
        }
        showCurrentBoardState(combinedBoard, playerBoard);
        if (bot1.isLoss(combinedBoard ^ playerBoard)) {
            cout << "bot 2 wins" << endl;
            break;
        }

        if (selfPlay) {
            moveCol = bot1.getBestMove(combinedBoard, playerBoard);
        } else {
            bool moveNotSet = true;
            while (moveNotSet) {
                moveCol = -1;
                string playerMove = "";
                cout << "your turn" << endl;
                cin >> playerMove;
                if (playerMove.length() == 1 && isdigit(playerMove[0])) {
                    moveCol = playerMove[0] - '0';
                    if (0 <= moveCol && moveCol <= 6 &&
                        bot1.getMove(combinedBoard, moveCol) != 0) {
                        moveNotSet = false;
                    } else {
                        cout << endl
                             << "that move is not valid, please try again" << endl;
                    }
                } else {
                    cout << endl
                         << "that move is not valid, please try again" << endl;
                }
            }
        }

        if (moveCol == -1) {
            cout << "game drawn" << endl;
            break;
        }
        newMove = bot1.getMove(combinedBoard, moveCol);
        combinedBoard |= newMove;
        playerBoard |= newMove;

        cout << "bot 2's turn" << endl;
        showCurrentBoardState(combinedBoard, playerBoard);
        if (bot1.isLoss(playerBoard)) {
            cout << "bot 1 wins" << endl;
            break;
        }

        moveCol = bot1.getBestMove(combinedBoard, combinedBoard ^ playerBoard);
        if (moveCol == -1) {
            cout << "game drawn" << endl;
            break;
        }
        newMove = bot1.getMove(combinedBoard, moveCol);

        combinedBoard |= newMove;
    }

    cout << "press enter to exit" << endl;
    string exitVal = "";
    cin >> exitVal;
    return 0;
}
