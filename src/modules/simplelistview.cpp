/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "simplelistview.h"

#include <QApplication>
#include <QMouseEvent>
#include <QScrollBar>
#include <QModelIndex>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <DGuiApplicationHelper>

#include "simplelistmodel.h"

SimpleListView::SimpleListView(int mode, QWidget *parent)
    : DListView(parent)
{
    m_mode = mode;

    setVerticalScrollMode(ScrollPerPixel); //鼠标滚轮滚动一次一个像素
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); //设置垂直滚条
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked); //单击已选中的内容双击单元格时编辑
    setFocusPolicy(Qt::NoFocus);
    setAutoScroll(false);
    setSelectionBehavior(QAbstractItemView::SelectRows); //选中一行
    setSelectionMode(QAbstractItemView::SingleSelection); //选中单个目标
    if (m_mode == 0)
        setFixedHeight(99);

    if (m_mode == 1)
        setMouseTracking(true);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &SimpleListView::adjustScrollbarMargins);
}

SimpleListView::~SimpleListView()
{
}

/**
 * @brief 添加右键菜单
 */
void SimpleListView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu(this);
    //缺翻译
    QAction *copy = new QAction(tr("Copy"), menu);
    QAction *clean = new QAction(tr("Delete"), menu);
    menu->addAction(copy);
    menu->addAction(clean);
    connect(copy, &QAction::triggered, this, [ = ]() {
        static_cast<SimpleListModel *>(model())->copyToClipboard(indexAt(event->pos()).row());
    });
    connect(clean, &QAction::triggered, this, [ = ]() {
        static_cast<SimpleListModel *>(model())->deleteItem(indexAt(event->pos()).row());
    });
    if (indexAt(event->pos()).row() >= 0 && m_itemfill) {
        menu->exec(event->globalPos());
    }
    delete menu;
}

/**
 * @brief 给私有变量赋值，历史记录有数据
 */
void SimpleListView::listItemFill(bool itemfill)
{
    m_itemfill = itemfill;
}

/**
 * @brief 更改delegate参数，实现delegate的hover状态
 */
void SimpleListView::mouseMoveEvent(QMouseEvent *e)
{
    if (m_mode == 1 && m_itemfill) {
        if (m_ispressed == true) {
            if (indexAt(m_presspoint) != indexAt(e->pos()))
                m_presschanged = true;
            else
                m_presschanged = false;
            QListView::mouseMoveEvent(e);
        } else {
            static_cast<SimpleListDelegate *>(itemDelegate(indexAt(e->pos())))->paintback(indexAt(e->pos()), 0);
            QWidget::mouseMoveEvent(e);
        }
    } else {
        if (e->x() < width() - 12) {
            QWidget::mouseMoveEvent(e);
        } else {
            QListView::mouseMoveEvent(e);
        }
    }
}

/**
 * @brief 设置滚动区域边距
 */
void SimpleListView::adjustScrollbarMargins()
{
    if (!isVisible()) {
        return;
    }
    QEvent event(QEvent::LayoutRequest);
    QApplication::sendEvent(this, &event);

    if (!verticalScrollBar()->visibleRegion().isEmpty()) {
        setViewportMargins(0, 0, -verticalScrollBar()->sizeHint().width(), 0);
    } else {
        setViewportMargins(0, 0, 0, 0);
    }
}

/**
 * @brief 更改delegate参数，实现delegate的press状态
 */
void SimpleListView::mousePressEvent(QMouseEvent *event)
{
    static_cast<SimpleListModel *>(model())->refrushModel();
    if (m_mode == 1 && event->buttons() & Qt::LeftButton) {
        if (indexAt(event->pos()).row() > -1) {
            m_presspoint = event->pos();
            static_cast<SimpleListDelegate *>(itemDelegate(indexAt(m_presspoint)))->paintback(indexAt(m_presspoint), 2);
            m_currentrow = indexAt(m_presspoint).row();
            m_ispressed = true;
        }
    }
//    DListView::mousePressEvent(event);
}

/**
 * @brief 更改delegate参数，实现delegate的release状态
 */
void SimpleListView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_mode == 1) {
        static_cast<SimpleListModel *>(model())->refrushModel();
        if (m_currentrow == indexAt(event->pos()).row() && m_currentrow >= 0) {
            emit obtainingHistorical(indexAt(event->pos()));
            static_cast<SimpleListDelegate *>(itemDelegate(indexAt(m_presspoint)))->paintback(indexAt(m_presspoint), 1);
        } else {
            if (m_presschanged == true) {
                static_cast<SimpleListDelegate *>(itemDelegate(indexAt(event->pos())))->paintback(indexAt(event->pos()), 1);
            } else
                static_cast<SimpleListDelegate *>(itemDelegate(indexAt(m_presspoint)))->paintback(indexAt(m_presspoint), 0);
        }
        m_ispressed = false;
        m_presspoint = QPoint();
    }
    DListView::mouseReleaseEvent(event);
}
