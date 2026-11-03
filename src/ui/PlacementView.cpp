#include "ui/PlacementView.h"

#include "ui/BoardWidget.h"
#include "ui/GameControls.h"
#include "ui/Theme.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPalette>
#include <QVBoxLayout>

namespace bs {
namespace ui {
namespace {

QLineEdit* makeNumberEdit(QWidget* parent)
{
    auto* edit = new QLineEdit(parent);
    edit->setFixedWidth(40);
    edit->setAlignment(Qt::AlignCenter);
    edit->setMaxLength(2);
    styleLineEdit(edit);
    return edit;
}

QLabel* makeFixedLabel(const QString& text, int width, QWidget* parent, int pixelSize = 13,
                       bool strong = false)
{
    auto* label = new QLabel(text, parent);
    label->setFixedWidth(width);
    styleLabel(label, pixelSize, strong);
    return label;
}

/// 方向下拉：用中文 + 箭头替代旧版 x / z / y 字母 token。
void fillDirectionCombo(QComboBox* combo)
{
    combo->addItem(QStringLiteral("↓ 下"), static_cast<int>(Direction::Down));
    combo->addItem(QStringLiteral("↑ 上"), static_cast<int>(Direction::Up));
    combo->addItem(QStringLiteral("← 左"), static_cast<int>(Direction::Left));
    combo->addItem(QStringLiteral("→ 右"), static_cast<int>(Direction::Right));
    combo->setCurrentIndex(0); // 与旧版"留空=下"一致
    combo->setToolTip(QStringLiteral("船头朝向：船体从起始格沿该方向延伸"));
}

Direction chosenDirection(const QComboBox* combo)
{
    return static_cast<Direction>(combo->currentData().toInt());
}

} // namespace

PlacementView::PlacementView(GameSession* session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    m_title = new QLabel(this);
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setFont(theme::displayFont(24));
    QPalette titlePalette = m_title->palette();
    titlePalette.setColor(QPalette::WindowText, theme::ink());
    m_title->setPalette(titlePalette);

    auto* hint = new QLabel(
        QStringLiteral("① 选舰种　② 点棋盘选船头　③ 选方向　④ 点放置"), this);
    hint->setAlignment(Qt::AlignCenter);
    styleLabel(hint, 13);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    m_board = new BoardWidget(this);
    m_board->setGridLabelsVisible(true);
    connect(m_board, &BoardWidget::cellActivated, this, &PlacementView::onBoardClicked);

    layout->addStretch(1);
    layout->addWidget(m_title, 0, Qt::AlignHCenter);
    layout->addWidget(hint, 0, Qt::AlignHCenter);
    layout->addWidget(m_board, 0, Qt::AlignHCenter);
    layout->addSpacing(4);
    layout->addWidget(buildPanel());
    layout->addStretch(1);

    connect(m_group, QOverload<int>::of(&QButtonGroup::buttonClicked), this,
            &PlacementView::setActiveRow);

    m_rows[0].select->setChecked(true);
    setActiveRow(0);
    setSide(Side::Player1);
}

QWidget* PlacementView::buildPanel()
{
    auto* panel = new QWidget(this);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    for (int i = 0; i < kShipTypeCount; ++i)
        layout->addWidget(buildShipRow(i, kAllShipTypes[static_cast<size_t>(i)]));

    layout->addWidget(buildDeleteRow());
    layout->addSpacing(6);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);

    auto* save = new ThemeButton(QStringLiteral("保存"), panel, ThemeButton::Accent::Seal);
    save->setFixedWidth(96);
    buttons->addWidget(save);

    auto* back = new ThemeButton(QStringLiteral("返回"), panel);
    back->setFixedWidth(96);
    buttons->addWidget(back);

    layout->addLayout(buttons);

    connect(save, &ThemeButton::clicked, this, &PlacementView::commit);
    connect(back, &ThemeButton::clicked, this, &PlacementView::backRequested);

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

    row.select = new ThemeButton(shipTypeName(type), rowWidget);
    row.select->setFixedWidth(76);
    row.select->setCheckable(true);
    row.select->setToolTip(QStringLiteral("选中后，点击棋盘即可把船头坐标填入本行"));
    m_group->addButton(row.select, rowIndex);
    layout->addWidget(row.select);

    row.remaining = makeFixedLabel(QString(), 48, rowWidget);
    layout->addWidget(row.remaining);

    row.x = makeNumberEdit(rowWidget);
    row.x->setPlaceholderText(QStringLiteral("行"));
    row.x->setToolTip(QStringLiteral("起始行号，1-10；也可直接点击棋盘"));
    layout->addWidget(row.x);

    row.y = makeNumberEdit(rowWidget);
    row.y->setPlaceholderText(QStringLiteral("列"));
    row.y->setToolTip(QStringLiteral("起始列号，1-10；也可直接点击棋盘"));
    layout->addWidget(row.y);

    row.direction = new QComboBox(rowWidget);
    row.direction->setFixedWidth(76);
    fillDirectionCombo(row.direction);
    styleComboBox(row.direction);
    layout->addWidget(row.direction);

    row.place = new ThemeButton(QStringLiteral("放置"), rowWidget);
    row.place->setFixedWidth(60);
    layout->addWidget(row.place);

    layout->addStretch(1);

    connect(row.place, &ThemeButton::clicked, this, [this, rowIndex] {
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

    layout->addWidget(makeFixedLabel(QStringLiteral("删除船只"), 76, rowWidget, 13, true));
    layout->addWidget(makeFixedLabel(QString(), 48, rowWidget));

    m_deleteX = makeNumberEdit(rowWidget);
    m_deleteX->setPlaceholderText(QStringLiteral("行"));
    m_deleteX->setToolTip(QStringLiteral("要删除的船所在行号，1-10"));
    layout->addWidget(m_deleteX);

    m_deleteY = makeNumberEdit(rowWidget);
    m_deleteY->setPlaceholderText(QStringLiteral("列"));
    m_deleteY->setToolTip(QStringLiteral("要删除的船所在列号，1-10"));
    layout->addWidget(m_deleteY);

    // 占位，和上面的方向下拉对齐（用 QLabel 而不是禁用的输入框，避免多余的编辑框）。
    layout->addWidget(makeFixedLabel(QString(), 76, rowWidget));

    auto* button = new ThemeButton(QStringLiteral("删除"), rowWidget);
    button->setFixedWidth(60);
    layout->addWidget(button);

    layout->addStretch(1);

    connect(button, &ThemeButton::clicked, this, &PlacementView::deleteShip);
    return rowWidget;
}

void PlacementView::setSide(Side side)
{
    m_side = side;
    m_hasPending = false;
    m_title->setText(sideTitle(m_side) + QStringLiteral("  布阵"));
    refreshBoard();
    refreshCounters();
}

void PlacementView::setActiveRow(int rowIndex)
{
    m_activeRow = rowIndex;
    m_hasPending = false;
    for (int i = 0; i < kShipTypeCount; ++i) {
        m_rows[static_cast<size_t>(i)].select->setAccent(
            i == rowIndex ? ThemeButton::Accent::Seal : ThemeButton::Accent::Ink);
    }
    refreshBoard();
}

void PlacementView::onBoardClicked(Coord coord)
{
    m_pending = coord;
    m_hasPending = true;

    ShipRow& row = m_rows[static_cast<size_t>(m_activeRow)];
    row.x->setText(QString::number(coord.row + 1));
    row.y->setText(QString::number(coord.col + 1));
    refreshBoard();
}

void PlacementView::refreshBoard()
{
    const Board& board = m_session->board(m_side);
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const Coord coord(row, col);
            const CellView view = board.view(coord);
            CellVisual visual;
            if (view.occupied) {
                visual.ship = view.type;
                visual.direction = view.direction;
                visual.kind = view.isHead ? CellVisual::Kind::ShipHead : CellVisual::Kind::ShipBody;
            } else if (m_hasPending && coord == m_pending) {
                visual.kind = CellVisual::Kind::Selected;
            }
            // 布置阶段所有格子都可点：点击只是把坐标填进输入框。
            m_board->setCell(row, col, visual, true);
        }
    }
}

void PlacementView::refreshCounters()
{
    const Board& board = m_session->board(m_side);
    for (ShipRow& row : m_rows) {
        const int remaining = board.remainingToPlace(row.type);
        row.remaining->setText(QStringLiteral("剩 %1").arg(remaining));
        row.place->setEnabled(remaining > 0);
        row.select->setEnabled(remaining > 0);
    }

    // 当前舰种放满了就自动跳到下一个还没放完的舰种。
    if (!m_rows[static_cast<size_t>(m_activeRow)].select->isEnabled()) {
        for (int i = 0; i < kShipTypeCount; ++i) {
            if (m_rows[static_cast<size_t>(i)].select->isEnabled()) {
                m_rows[static_cast<size_t>(i)].select->setChecked(true);
                setActiveRow(i);
                break;
            }
        }
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
    QMessageBox::critical(this, QStringLiteral("布置失败"), message);
}

void PlacementView::placeShip(const ShipRow& row)
{
    Board& board = m_session->board(m_side);
    if (board.remainingToPlace(row.type) <= 0) {
        QMessageBox::critical(this, QStringLiteral("布置失败"),
                              QStringLiteral("没有多余的%1了").arg(shipTypeName(row.type)));
        return;
    }

    bool ok = false;
    const Coord head = readCoord(row.x, row.y, &ok);
    const Direction direction = chosenDirection(row.direction);
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
    m_hasPending = false;

    refreshBoard();
    refreshCounters();
}

void PlacementView::deleteShip()
{
    // 沿用旧版行为：两个框都填了才处理。
    if (m_deleteX->text().isEmpty() || m_deleteY->text().isEmpty())
        return;

    // 与放置框保持一致：X = 行，Y = 列。
    bool ok = false;
    const Coord coord = readCoord(m_deleteX, m_deleteY, &ok);
    if (!ok || !m_session->board(m_side).removeShipAt(coord)) {
        showPlacementError(PlacementError::NoShipAtCoord, ShipType::Submarine);
        return;
    }

    m_deleteX->clear();
    m_deleteY->clear();
    m_hasPending = false;
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
        const auto answer = QMessageBox::question(this, QStringLiteral("确认保存"),
                                                  QStringLiteral("确定保存该配置吗？"),
                                                  QMessageBox::Save | QMessageBox::Cancel);
        if (answer == QMessageBox::Save)
            emit placementFinished();
        return;
    }

    const auto answer = QMessageBox::warning(this, QStringLiteral("返回主菜单"),
                                             QStringLiteral("没有使用完所有的船只，是否返回主界面？"),
                                             QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes)
        emit placementFinished();
}

} // namespace ui
} // namespace bs
