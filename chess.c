

typedef enum {
  PIECE_EMPTY = 0,
  PIECE_PAWN,
  PIECE_KING,
  PIECE_QUEEN,
  PIECE_ROOK,
  PIECE_KNIGHT,
  PIECE_BISHOP,
  PIECE_COUNT,
} Piece_Type;

typedef enum {
  COLOR_EMPTY,
  COLOR_BLACK,
  COLOR_WHITE,
} Piece_Color;

typedef struct {
  Piece_Color color[64];
  Piece_Type piece[64];
} Pieces;

typedef struct {
  Pieces pieces;
} Board;
