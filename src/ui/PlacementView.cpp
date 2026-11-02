#include "ui/PlacementView.h"

#include "ui/BoardWidget.h"
#include "ui/Pixmaps.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace bs {
namespace ui {
namespace {

QLineEdit* makeNumberEdit(QWidget* parent)
{
    auto* edit = new QLineEdit(parent);
    edit->setFixedWidth(36);
    edit->setAlignment(Qt::AlignCenter);
    edit->setMaxLength(2);
    return edit;
}

QLabel* makeFixedLabel(const QString& text, int width, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setFixedWidth(width);
    return label;
}

} // namespace

PlacementView::PlacementView(GameSession* session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

    m_title = new QLabel(this);
    m_title->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    m_title->setFont(titleFont);

    m_board = new BoardWidget(this);
    m_board->setGridLabelsVisible(true);

    layout->addStretch(1);
    layout->addWidget(m_title);
    layout->addWidget(m_board, 0, Qt::AlignHCenter);
    layout->addSpacing(6);
    layout->addWidget(buildPanel());
    layout->addStretch(1);

    setSide(Side::Player1);
}

QWidget* PlacementView::buildPanel()
{
    auto* panel = new QWidget(this);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    for (int i = 0; i < kShipTypeCount; ++i)
        layout->addWidget(buildShipRow(i, kAllShipTypes[static_cast<size_t>(i)]));

    layout->addWidget(buildDeleteRow());
    layout->addSpacing(6);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);

    auto* save = new QPushButton(QStringLiteral("保存"), panel);
    save->setFixedWidth(93);
    buttons->addWidget(save);

    auto* back = new QPushButton(QStringLiteral("返回"), panel);
    back->setFixedWidth(93);
    buttons->addWidget(back);

    layout->addLayout(buttons);

    connect(save, &QPushButton::clicked, this, &PlacementView::commit);
    connect(back, &QPushButton::clicked, this, &PlacementView::backRequested);

    return panel;
}

QWidget* PlacementView::buildShipRow(int rowIndex, ShipType type)
{
    auto* rowWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    ShipRow& row = m_rows[static_cast<size_t>(rowIndex)];
    row.type = type;

    layout->addWidget(makeFixedLabel(shipTypeName(type), 64, rowWidget));

    row.remaining = makeFixedLabel(QString(), 52, rowWidget);
    layout->addWidget(row.remaining);

    row.x = makeNumberEdit(rowWidget);
    row.x->setPlaceholderText(QStringLiteral("行"));
    row.x->setToolTip(QStringLiteral("起始行号，1-10"));
    layout->addWidget(row.x);

    row.y = makeNumberEdit(rowWidget);
    row.y->setPlaceholderText(QStringLiteral("列"));
    row.y->setToolTip(QStringLiteral("起始列号，1-10"));
    layout->addWidget(row.y);

    row.direction = makeNumberEdit(rowWidget);
    row.direction->setMaxLength(1);
    row.direction->setPlaceholderText(QStringLiteral("向"));
    row.direction->setToolTip(QStringLiteral("方向：x 上 / z 右 / y 左 / 留空 下"));
    layout->addWidget(row.direction);

    auto* place = new QPushButton(QStringLiteral("放置"), rowWidget);
    place->setFixedWidth(60);
    layout->addWidget(place);
    row.place = place;

    layout->addStretch(1);

    connect(place, &QPushButton::clicked, this, [this, rowIndex] {
        placeShip(m_rows[static_cast<size_t>(rowIndex)]);
    });

    return rowWidget;
}

QWidget* PlacementView::buildDeleteRow()
{
    auto* rowWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    layout->addWidget(makeFixedLabel(QStringLiteral("删除"), 64, rowWidget));
    layout->addWidget(makeFixedLabel(QString(), 52, rowWidget));

    m_deleteX = makeNumberEdit(rowWidget);
    m_deleteX->setPlaceholderText(QStringLiteral("列"));
    m_deleteX->setToolTip(QStringLiteral("列号，1-10"));
    layout->addWidget(m_deleteX);

    m_deleteY = makeNumberEdit(rowWidget);
    m_deleteY->setPlaceholderText(QStringLiteral("行"));
    m_deleteY->setToolTip(QStringLiteral("行号，1-10"));
    layout->addWidget(m_deleteY);

    auto* filler = makeNumberEdit(rowWidget);
    filler->setEnabled(false);
    layout->addWidget(filler);

    auto* button = new QPushButton(QStringLiteral("删除"), rowWidget);
    button->setFixedWidth(60);
    layout->addWidget(button);

    layout->addStretch(1);

    connect(button, &QPushButton::clicked, this, &PlacementView::deleteShip);
    return rowWidget;
}

void PlacementView::setSide(Side side)
{
    m_side = side;
    m_title->setText(sideTitle(m_side) + QStringLiteral("  布阵"));
    refreshBoard();
    refreshCounters();
}

void PlacementView::refreshBoard()
{
    const Board& board = m_session->board(m_side);
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const CellView view = board.view(Coord(row, col));
            CellVisual visual;
            if (view.occupied) {
                visual.ship = view.type;
                visual.direction = view.direction;
                visual.kind = view.isHead ? CellVisual::Kind::ShipHead : CellVisual::Kind::ShipBody;
            }
            m_board->setCell(row, col, visual, false);
        }
    }
}

void PlacementView::refreshCounters()
{
    const Board& board = m_session->board(m_side);
    for (ShipRow& row : m_rows) {
        const int remaining = board.remainingToPlace(row.type);
        row.remaining->setText(QStringLiteral("剩余 %1").arg(remaining));
        row.place->setEnabled(remaining > 0);
    }
}

Coord PlacementView::readCoord(QLineEdit* rowEdit, QLineEdit* colEdit, bool* ok) const
{
    bool rowOk = false;
    bool colOk = false;
    const int row = rowEdit->text().toInt(&rowOk);
    const int col = colEdit->text().toInt(&colOk);
    *ok = rowOk && colOk;
    return Coord(row - 1, col - 1);
}

void PlacementView::showPlacementError(PlacementError error, ShipType type)
{
    QString message;
    switch (error) {
    case PlacementError::OutOfBounds:
        message = QStringLiteral("该位置不满足布置%1的要求").arg(shipTypeName(type));
        break;
    case PlacementError::Overlap:
        message = QStringLiteral("与其它船只相撞");
        break;
    case PlacementError::NoShipAtCoord:
        message = QStringLiteral("该位置没有可删除船只");
        break;
    case PlacementError::None:
        return;
    }
    QMessageBox::critical(this, QStringLiteral("critical"), message);
}

void PlacementView::placeShip(const ShipRow& row)
{
    Board& board = m_session->board(m_side);
    if (board.remainingToPlace(row.type) <= 0) {
        QMessageBox::critical(this, QStringLiteral("critical"),
                              QStringLiteral("没有多余的%1了").arg(shipTypeName(row.type)));
        return;
    }

    bool ok = false;
    const Coord head = readCoord(row.x, row.y, &ok);
    const Direction direction = directionFromToken(row.direction->text());
    if (!ok) {
        showPlacementError(PlacementError::OutOfBounds, row.type);
        return;
    }

    const PlacementError error = board.canPlace(row.type, head, direction);
    if (error != PlacementError::None) {
        showPlacementError(error, row.type);
        return;
    }

    board.place(row.type, head, direction);
    row.x->clear();
    row.y->clear();
    row.direction->clear();

    refreshBoard();
    refreshCounters();
}

void PlacementView::deleteShip()
{
    // 沿用旧版行为：两个框都填了才处理。
    if (m_deleteX->text().isEmpty() || m_deleteY->text().isEmpty())
        return;

    // 注意：旧版删除框的 X 作用于"列"、Y 作用于"行"，与放置框相反。
    // 这里原样保留以免改变现有操作习惯，详见 docs/REFACTORING_PLAN.md 的待确认项。
    bool ok = false;
    const Coord coord = readCoord(m_deleteY, m_deleteX, &ok);
    if (!ok || !m_session->board(m_side).removeShipAt(coord)) {
        showPlacementError(PlacementError::NoShipAtCoord, ShipType::Submarine);
        return;
    }

    m_deleteX->clear();
    m_deleteY->clear();
    refreshBoard();
    refreshCounters();
}

void PlacementView::commit()
{
    const Board& board = m_session->board(m_side);
    bool allPlaced = true;
    for (ShipType type : kAllShipTypes) {
        if (board.remainingToPlace(type) > 0) {
            allPlaced = false;
            break;
        }
    }

    if (allPlaced) {
        const auto answer = QMessageBox::question(this, QStringLiteral("Check"),
                                                  QStringLiteral("确定保存该配置吗？"),
                                                  QMessageBox::Save | QMessageBox::Cancel);
        if (answer == QMessageBox::Save)
            emit placementFinished();
        return;
    }

    const auto answer = QMessageBox::warning(this, QStringLiteral("Back?"),
                                             QStringLiteral("没有使用完所有的船只，是否返回主界面？"),
                                             QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes)
        emit placementFinished();
}

} // namespace ui
} // namespace bs
