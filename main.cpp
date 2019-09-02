/****************************************************************************
* Author: Martin C. Foster
* Date: Aug 21,2019
* The getch functions were found here:
* https://www.daniweb.com/programming/software-development/
*                                     threads/410155/gcc-equivalent-for-getch
*
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <exception>

// Uncomment the following if building on linux.
#define LINUX_APP

#ifdef LINUX_APP

#include <termios.h>
#include <unistd.h>

#else
#include <conio.h>
#endif


const int Rows = 8;
const int Cols = 8;
static const int BoardSize = 65;
const char NewBoard[BoardSize] =
        " b b b bb b b b  b b b b        "
        "        r r r r  r r r rr r r r ";
// MCF Debug
//       12345678   12345678   12345678   12345678
//        " b   b  " "        " "        " "    r   "
//        "   r    " "        " " r      " "        ";

static char *main_board = nullptr;

static char userColor = 'b';
static char currentColor = 'b';
static char compColor = 'r';

#ifdef LINUX_APP

int getch(void);

int getche(void);

#endif

struct TLocation {
    int row, col;

    bool operator==(const TLocation &a) const
    {
        return (row == a.row && col == a.col);
    }
};

struct TMove {
    TLocation from, to;
    int score;

    bool operator==(const TMove &a) const
    {
        return (from == a.from && to == a.to);
    }
};

struct TChecker {
    char color;
    TLocation loc;
};

static TLocation jumpPos;
static bool jumped;

bool RunGame();

bool computerMove(char *board);

bool humanMove(char *board);

void doMove(char *board, TMove move);

void checkKings(char *board);

TLocation getLocation();

int scoreMoves(char *board, std::vector<TMove *> *moves, char color);

std::vector<TMove *> *copyMoveList(std::vector<TMove *> *moves);

void showBoard(char *board);

std::vector<TMove *> *getValidMoves(char *board, char color);

int getScore(char *board);
bool checkMove(const char *board, std::vector<TMove *> *moves, int rowIdx, int colIdx,
               int r_plus, int c_plus, bool jumps);
bool checkJump(const char *board, std::vector<TMove *> *moves,
               int rowIdx, int colIdx, int r_plus, int c_plus, bool *jumps);
void clearMoves(std::vector<TMove *> *moves);
void checkerMoves(const char *board, std::vector<TMove *> *moves, int rowIdx, int colIdx, bool &jumps);
bool doChecks(const char *board, std::vector<TMove *> *moves, int rowIdx, int colIdx, bool &jumps, int r_plus,
              int c_plus);

int main()
{
    printf("  Checkers");
    showBoard(const_cast<char *>(NewBoard));
    RunGame();
    return 0;
}

/****************************************************************************
 * Main game loop
 * @return
 */
bool RunGame()
{
    bool gameOver = false;

    main_board = strdup(NewBoard);
    while (!gameOver) {
        if (userColor == currentColor) {
            gameOver = !humanMove(main_board);
            if (gameOver)
                printf("Sorry, you have been defeated.");
        } else {
            gameOver = !computerMove(main_board);
            if (gameOver)
                printf("Congratulations! You have defeated the computer.");
        }

        showBoard(main_board);
    } // end while
    return gameOver;
} // end run game

/****************************************************************************
 * Its the computers turn!
 * @param board
 * @return
 */
bool computerMove(char *board)
{
    bool moveOk = true;
    bool done = false;

    std::vector<TMove *> *moveList = getValidMoves(board, currentColor);
    while (!done) {
        if (moveList->size() < 1) // no moves?
        {
            moveOk = false;
            done = true;
        } else {
            int idx = scoreMoves(board, moveList, compColor); //rand() % moveList->size();
            if (idx < 0)
                idx = 0;
            if (idx >= moveList->size())
                idx = moveList->size() - 1;
            {
                // MCF Debug print list & scores
                for (TMove *obj : *moveList)
                    printf("From:%d,%d  To:%d,%d Score = %d\n",
                           obj->from.row, obj->from.col, obj->to.row, obj->to.col, obj->score);
                TMove move = *(*moveList)[idx];
                printf("Selected move, from:%d,%d  to:%d,%d Score = %d\n",
                       move.from.row, move.from.col, move.to.row, move.to.col, move.score);

                doMove(board, move);
                clearMoves(moveList);
                if (jumped) {
                    bool jumps = false;
                    // Look for double jump
                    jumped = false;
                    for (int r_plus = -1; r_plus <= 1; r_plus += 2)
                        for (int c_plus = -1; c_plus <= 1; c_plus += 2)
                            checkJump(board, moveList, jumpPos.row - 1, jumpPos.col - 1,
                                      r_plus, c_plus, &jumps);
                }
                // Found move in the valid move list
                if (moveList->size() < 1)  // No more jumps found
                {
                    done = true;
                    moveOk = true;
                }
            }
        }
    } // end while not done loop.
    currentColor = userColor; // Human's turn
    clearMoves(moveList);
    delete (moveList);
    return moveOk;
} // computerMove

/****************************************************************************
 * Get move from the user
 * @param board
 * @return true for valid move. False means no valid moves.
 */
bool humanMove(char *board)
{
    bool moveOk = false;
    bool anotherJump = false;
    TMove move;
    bool done = false;
    bool invalid = false;

    std::vector<TMove *> *moveList = getValidMoves(board, userColor);

    if (moveList->size() < 1) // no moves?
    {
        done = true;
    }
    // MCF debug
    // printf("Number of valid moves: %d \n", (int)moveList->size());
    while (!done) {
        // MCF Debug
        for (TMove *obj : *moveList)
            printf("From:%d,%d  To:%d,%d\n", obj->from.row, obj->from.col, obj->to.row, obj->to.col);
        if (!anotherJump) // just need to location
        {
            printf("\nFrom row,col: ");
            move.from = getLocation();
        }
        printf("\nTo row,col: ");
        move.to = getLocation();
        move.score = 0;
        invalid = true;
        for (TMove *obj : *moveList) {
            if (*obj == move) {
                invalid = false;
                doMove(board, move);
                clearMoves(moveList);
                if (jumped) {
                    // Look for double jump
                    jumped = false;
                    for (int r_plus = -1; r_plus <= 1; r_plus += 2)
                        for (int c_plus = -1; c_plus <= 1; c_plus += 2)
                            checkJump(board, moveList, jumpPos.row - 1, jumpPos.col - 1,
                                      r_plus, c_plus, &anotherJump);
                }
                // Found move in the valid move list
                if (moveList->size() < 1)  // No more jumps found
                {
                    done = true;
                    moveOk = true;
                } else {
                    // From location is where we jumped to.
                    move.from = move.to;
                }
            }
        } // next obj
        showBoard(board);
        if (!done) {
            if(invalid)
                printf("\nInvalid move.\n");
            else
                printf("\nAnother jump\n");
        }
    } // end while

    if (!moveOk)
        printf("\nInvalid"
               " move.\n");

    currentColor = compColor; // Computer's turn
    clearMoves(moveList);
    delete (moveList);
    return moveOk;
} // humanMove

/****************************************************************************
 * Move the selected checker on the board.
 * @param board
 * @param move
 */
void doMove(char *board, TMove move)
{
    int fromIdx = ((move.from.row - 1) * Cols) + (move.from.col - 1);
    int toIdx = ((move.to.row - 1) * Cols) + (move.to.col - 1);
    int jumpIdx = -1;

    if (abs(move.from.row - move.to.row) == 2) {
        int jr = move.to.row + ((move.from.row - move.to.row) / 2);
        int jc = move.to.col + ((move.from.col - move.to.col) / 2);
        jumpIdx = ((jr - 1) * Cols) + (jc - 1);
        jumped = true;
        jumpPos = move.to;
    }
    char checker = board[fromIdx];
    board[fromIdx] = ' '; // clear from location
    board[toIdx] = checker; // clear from location
    if ((jumpIdx >= 0) && (jumpIdx < BoardSize))
        board[jumpIdx] = ' ';
    checkKings(main_board);
} // doMove

/***************************************************************************************
 *
 * @param board
 */
void checkKings(char *board)
{
    char color = 'r'; // reds on row 1 to king
    for (int rowIdx = 0; rowIdx < Rows; rowIdx += 7) {
        if (rowIdx > 0)
            color = 'b'; // blacks on row 8 to king
        for (int colIdx = 0; colIdx < Cols; colIdx++) {
            int idx = (rowIdx * Cols) + colIdx;
            if (board[idx] == color)
                // 1101 1111
                board[idx] &= 0xDF; // to upper
        }
    }
}

/****************************************************************************
 * Get the location from the user.
 * @return
 */
TLocation getLocation()
{
    TLocation loc;
    char c;
    bool done = false;
    // Get the row
    do {
        c = static_cast<char>(getch());
        if ((c > '0') && (c < '9')) {
            putchar(c); //show the digit
            loc.row = (c - '0');
            done = true;
        }
    } while (!done);
    done = false;
    // get a coma
    do {
        c = static_cast<char>(getch());
        if (c == ',') {
            putchar(c); //show the digit
            done = true;
        }
    } while (!done);
    done = false;
    // get a column
    do {
        c = static_cast<char>(getch());
        if ((c > '0') && (c < '9')) {
            putchar(c); //show the digit
            loc.col = (c - '0');
            done = true;
        }
    } while (!done);
    putchar('\n');
    return loc;
} // getLocation

/****************************************************************************
 * Get the value score for each move in the moves list.
 * @param board
 * @param moves
 * @param color - Color of the checker being moved
 * @return index for best move.
 */
int scoreMoves(char *board, std::vector<TMove *> *moves, char color)
{
    static int level = 0;
    static int winCount = 0;
    static int stallCnt = 0;
    char *tempBoard;
    const int MaxStallCnt = 30;
    level++;
    printf("L=%d, ",level);
    // MCF: debug
    if(level > 20)
        level=level;
    int moveIdx = 0;
    int hiScore = 0;
    int moveCount = 0;
//    printf("level: %d Move count %d color: %c\n", level, static_cast<int>(moves->size()), color);
    for (TMove *pMove : *moves) {
        moveCount++;
        if(level == 1)
            stallCnt = 0;
        else {
            stallCnt++;
        }
        try{
            std::vector<TMove *> *tempMoves = new(std::vector<TMove *>);
            tempBoard = strdup(board);
            if(tempBoard == NULL)
                printf("Level: %d, 'strdup' failed\n",level);

            doMove(tempBoard, *pMove);
            if (jumped) {
                bool jumps = false;
                if(stallCnt>1)
                    stallCnt--; // not stalled if jumping
                // Look for double jump
                jumped = false;
                for (int r_plus = -1; r_plus <= 1; r_plus += 2)
                    for (int c_plus = -1; c_plus <= 1; c_plus += 2)
                        checkJump(tempBoard, tempMoves, jumpPos.row - 1, jumpPos.col - 1,
                                  r_plus, c_plus, &jumps);
            }
            // Found pMove in the valid pMove list
            if (tempMoves->size() > 0)  // More jumps found
            {
                scoreMoves(tempBoard, tempMoves, color);
                if(level == 1) {
                    pMove->score = winCount;
                    if(hiScore < pMove->score)
                        hiScore = pMove->score;
                    winCount = 0;
                }
            } else {
                if (color == compColor)
                    color = userColor;
                else
                    color = compColor;
            }
            clearMoves(tempMoves);
            delete (tempMoves);

            std::vector<TMove *> *moveList = getValidMoves(tempBoard, color);
            if(moveList->size() >0){
                // some move sanerios will go forever
                if(!((level >1) && (stallCnt > MaxStallCnt)))
                {
                    scoreMoves(tempBoard, moveList, color);
                    if(level == 1) {
                        pMove->score = winCount;
                        if(hiScore < pMove->score)
                            hiScore = pMove->score;
                        winCount = 0;
                    }
                }
                else {
                    if(getScore(board) > 99)
                        winCount++;
                }
            }
            else {
                if (color == userColor)
                    winCount++;
            }
            clearMoves(moveList);
            delete(moveList);
            free(tempBoard);
        }
        catch(std::exception &e){
            printf("Level: %d, Exception: %s\n",level , e.what());
        }
    } // nextmove

    if(level == 1)
    {
        stallCnt = 0;
        std::vector<int> idxList;
        int idx = 0;
        for (TMove *move : *moves) {
            if (move->score >= hiScore) {
                idxList.insert(idxList.end(), idx);
            }
            idx++;
        }
        int sz = static_cast<int>(idxList.size());
        if (sz > 0) {
            moveIdx = idxList[rand() % sz];
        }
        idxList.clear();
        printf("Move index = %d\n", moveIdx);
    }
    level--;
    return moveIdx;
} // scoreMoves()

/****************************************************************************
 * Return a copy of a move list vector.
 * @param moves
 * @return
 */
std::vector<TMove *> *copyMoveList(std::vector<TMove *> *moves)
{
    std::vector<TMove *> *moves2 = new(std::vector<TMove *>);

    for (TMove *move : *moves) {
        TMove *ptr = new TMove();
        ptr->from = move->from;
        ptr->to = move->to;
        ptr->score = move->score;
        // Save move options
        moves2->insert(moves2->end(), ptr);
    }
    return moves2;
}

/****************************************************************************
 * Get a score for the board based on the radio of computer checkers to
 * human checkers.
 * @param board
 * @return Score for computer color, higher is better
 */
int getScore(char *board)
{
    double score = 0;
    double userCheckers = 0, compCheckers = 0;
    int idx = 0;

    char c;
    for (int rowIdx = 0; rowIdx < Rows; rowIdx++) {
        for (int colIdx = 0; colIdx < Cols; colIdx++) {
            c = board[idx++];
            if (c == userColor) {
                userCheckers++;
                if ((colIdx) == 0 || (colIdx == (Cols - 1)))
                    userCheckers += .1; // Slightly better along the wall
            } else if (c == (userColor & 0xDF))
                userCheckers += 1.5; // kings count more
            else if (c == compColor) {
                compCheckers++;
                if ((colIdx) == 0 || (colIdx == (Cols - 1)))
                    compCheckers += .1; // Slightly better along the wall
            } else if (c == (compColor & 0xDF))
                compCheckers += 1.5; // kings count more
        } // next colIdx
    } // next rowIdx
    if (userCheckers < 1.0)
        score = 15.0; // hi score
    else
        score = compCheckers / userCheckers;
    return static_cast<int>(score * 100.0);
} // showBoard

/****************************************************************************
 * Get a list of valid moves for the specified checker color
 * @param color
 * @return
 */
std::vector<TMove *> *getValidMoves(char *board, char color)
{
    bool jumps = false;

    std::vector<TMove *> *moves = new std::vector<TMove *>();
    int idx = 0;
    for (int rowIdx = 0; rowIdx < Rows; rowIdx++) {
        for (int colIdx = 0; colIdx < Cols; colIdx++) {
            char checkerColor = board[idx];

            // ignore color case
            if ((color | 0x20) == (checkerColor | 0x20)) {
                checkerMoves(board, moves, rowIdx, colIdx, jumps);
            }
            idx++;
        } // next colIdx
    } // next rowIdx
    return moves;
} // getValidMoves

/****************************************************************************
 * Get all valid moves for the checker at the specified position
 * @param board
 * @param moves
 * @param rowIdx
 * @param colIdx
 * @param jumps
 */
void checkerMoves(const char *board, std::vector<TMove *> *moves,
                  int rowIdx, int colIdx, bool &jumps)
{
    int r_plus = -1;
    int idx = (rowIdx * Cols) + colIdx;
    char checkerColor = board[idx];

    for (int c_plus = -1; c_plus <= 1; c_plus += 2) {
        r_plus = -1;
        // Red & kings moves up from the bottom.
        if (checkerColor == 'r' || checkerColor == 'R'
            || checkerColor == 'B') {
            if ((rowIdx > 0) && (rowIdx < Rows)) {
                jumps = doChecks(board, moves, rowIdx, colIdx, jumps, r_plus, c_plus);
            }
        }
        // Black & kings moves down from the top.
        if (checkerColor == 'b' || checkerColor == 'R'
            || checkerColor == 'B') {
            r_plus = 1;
            if ((rowIdx >= 0) && (rowIdx < Rows - 1)) {
                jumps = doChecks(board, moves, rowIdx, colIdx, jumps, r_plus, c_plus);
            }
        }
    } // next c_plus
}

bool doChecks(const char *board, std::vector<TMove *> *moves, int rowIdx, int colIdx, bool &jumps, int r_plus,
              int c_plus)
{
    if ((colIdx >= 0) && (colIdx < Cols)) {
        if (!checkMove(board, moves, rowIdx, colIdx,
                       r_plus, c_plus, jumps)) {
            checkJump(board, moves,
                      rowIdx, colIdx, r_plus, c_plus, &jumps);
        }

    }
    return jumps;
} // doChecks

/****************************************************************************
 * Check specific move, if valid add it to the moves list.
 * @param board
 * @param moves
 * @param rowIdx
 * @param colIdx
 * @param r_plus
 * @param c_plus
 */
bool checkMove(const char *board, std::vector<TMove *> *moves, int rowIdx, int colIdx,
               int r_plus, int c_plus, bool jumps)
{
    bool moveOk = false;
    if (((rowIdx + r_plus) >= 0) && ((rowIdx + r_plus) < Rows) &&
        ((colIdx + c_plus) >= 0) && ((colIdx + c_plus) < Cols)) {
        int moveIdx = ((rowIdx + r_plus) * Cols) + (colIdx + c_plus);
        char moveSquare = board[moveIdx];
        if (moveSquare == ' ') // empty move
        {
            // If a jumpwas found, other moves are not options
            if (!jumps) {
                TMove *ptr = new TMove();
                ptr->from.row = rowIdx + 1;
                ptr->from.col = colIdx + 1;
                ptr->to.row = (rowIdx + r_plus) + 1;
                ptr->to.col = (colIdx + c_plus) + 1;
                // Save move options
                moves->insert(moves->end(), ptr);
            }
            moveOk = true;
        }
    }
    return moveOk;
}

bool checkJump(const char *board, std::vector<TMove *> *moves,
               int rowIdx, int colIdx, int r_plus, int c_plus, bool *jumps)
{
    int moveIdx = ((rowIdx + r_plus) * Cols) + (colIdx + c_plus);
    int idx = rowIdx * Cols + colIdx;
    char moveSquare = board[moveIdx];
    bool jumpOk = false;
    int r_jump_to = rowIdx + r_plus * 2;
    int c_jump_to = colIdx + c_plus * 2;

    char color = board[idx];
    // If move square is a jumpable checker
    if (((color | 0x20) != (moveSquare | 0x20)) && (moveSquare != ' ')
        && (r_jump_to >= 0) && (r_jump_to < Rows)
        && (c_jump_to >= 0) && (c_jump_to < Cols)) {

        if (((color == 'b') && (r_plus > 0)) ||
            ((color == 'r') && (r_plus < 0)) || !(color & 0x20)) {
            moveIdx = (r_jump_to * Cols) + c_jump_to;
            moveSquare = board[moveIdx];
            // If move square is a jumpable checker
            if (moveSquare == ' ') // empty move
            {
                TMove *ptr = new TMove();
                ptr->from.row = rowIdx + 1;
                ptr->from.col = colIdx + 1;
                ptr->to.row = r_jump_to + 1;
                ptr->to.col = c_jump_to + 1;
                if (!*jumps) {
                    clearMoves(moves);
                    *jumps = true;
                }
                // save jump options
                moves->insert(moves->end(), ptr);
                jumpOk = true;
            }
        }
    }
    return jumpOk;
} // checkJump

void clearMoves(std::vector<TMove *> *moves)
{
    for (TMove *obj : *moves)
        delete obj;
    moves->clear();
}

/****************************************************************************
 * Print the provided checker board
 * @param board
 */
void showBoard(char *board)
{
    const char *topScale = "      1   2   3   4   5   6   7   8 ";
    const char *line = "    ---------------------------------";
    int idx = 0;
    printf("\n%s", topScale);
    for (int rowIdx = 0; rowIdx < Rows; rowIdx++) {
        printf("\n%s\n", line);
        printf(" %d  |", (rowIdx + 1));
        for (int colIdx = 0; colIdx < Cols; colIdx++) {
            printf(" %c |", board[idx++]);
        }
    }
    printf("\n%s\n", line);
} // showBoard



#ifdef LINUX_APP

/****************************************************************************
 * reads from keypress, doesn't echo
 * */
int getch(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

/* reads from keypress, echoes */
int getche(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

#endif
