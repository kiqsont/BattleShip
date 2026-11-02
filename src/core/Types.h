#ifndef BS_CORE_TYPES_H
#define BS_CORE_TYPES_H

#include <QMetaType>
#include <QString>

#include <array>

namespace bs {

/// 棋盘边长（格）。所有格子坐标均为 [0, kBoardSize) 的 row/col。
constexpr int kBoardSize = 10;
constexpr int kCellCount = kBoardSize * kBoardSize;

/// 对局双方。用枚举取代旧版 xxx1 / xxx2 的平行字段。
enum class Side { Player1 = 0, Player2 = 1 };
constexpr int kSideCount = 2;

inline int index(Side side) { return static_cast<int>(side); }
inline Side sideOf(int i) { return i == 0 ? Side::Player1 : Side::Player2; }
inline Side otherSide(Side side) { return side == Side::Player1 ? Side::Player2 : Side::Player1; }
QString sideTitle(Side side);

/// 舰种。长度由 lengthOf() 给出，不再把"船长"混进枚举值里。
enum class ShipType { Submarine, Destroyer, Cruiser, Battleship };
constexpr int kShipTypeCount = 4;

inline constexpr std::array<ShipType, kShipTypeCount> kAllShipTypes{
    ShipType::Battleship, ShipType::Cruiser, ShipType::Destroyer, ShipType::Submarine};

int lengthOf(ShipType type);
QString shipTypeName(ShipType type);

/// 几何方向：Up 表示 row 递减，Down 表示 row 递增，Left/Right 作用于 col。
enum class Direction { Up, Down, Left, Right };
constexpr int kDirectionCount = 4;
inline constexpr std::array<Direction, kDirectionCount> kAllDirections{
    Direction::Up, Direction::Down, Direction::Left, Direction::Right};

/// 旧版布置界面用 "x" / "z" / "y" 三个字母表示方向，其余输入一律按默认方向处理。
Direction directionFromToken(const QString& token);
QString directionToken(Direction direction);

struct Coord {
    int row = 0;
    int col = 0;

    Coord() = default;
    Coord(int r, int c) : row(r), col(c) {}

    bool isValid() const { return row >= 0 && row < kBoardSize && col >= 0 && col < kBoardSize; }
    int flatIndex() const { return row * kBoardSize + col; }
};

inline bool operator==(const Coord& a, const Coord& b) { return a.row == b.row && a.col == b.col; }
inline bool operator!=(const Coord& a, const Coord& b) { return !(a == b); }

/// 从 c 出发沿 d 走 n 步（可能越界，调用方需自行校验 isValid）。
inline Coord step(Coord c, Direction d, int n = 1)
{
    switch (d) {
    case Direction::Up:    return Coord(c.row - n, c.col);
    case Direction::Down:  return Coord(c.row + n, c.col);
    case Direction::Left:  return Coord(c.row, c.col - n);
    case Direction::Right: return Coord(c.row, c.col + n);
    }
    return c;
}

enum class ShotResult { Miss, Hit, Sunk, AlreadyResolved };
enum class PlacementError { None, OutOfBounds, Overlap, NoShipAtCoord };

} // namespace bs

Q_DECLARE_METATYPE(bs::Side)
Q_DECLARE_METATYPE(bs::Coord)

#endif // BS_CORE_TYPES_H
