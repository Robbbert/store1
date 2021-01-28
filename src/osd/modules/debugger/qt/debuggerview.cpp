// license:BSD-3-Clause
// copyright-holders:Andrew Gardner
#include "emu.h"
#include "debuggerview.h"

#include "modules/lib/osdobj_common.h"

#include <QtCore/QMimeData>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
#define horizontalAdvance width
#endif

DebuggerView::DebuggerView(
		debug_view_type type,
		running_machine &machine,
		QWidget *parent) :
	QAbstractScrollArea(parent),
	m_machine(machine),
	m_view(nullptr),
	m_preferBottom(false)
{
	// I like setting the font per-view since it doesn't override the menu fonts.
	const char *const selectedFont(downcast<osd_options &>(m_machine.options()).debugger_font());
	const float selectedFontSize(downcast<osd_options &>(m_machine.options()).debugger_font_size());
	QFont viewFontRequest((!*selectedFont || !strcmp(selectedFont, OSDOPTVAL_AUTO)) ? "Courier New" : selectedFont);
	viewFontRequest.setFixedPitch(true);
	viewFontRequest.setStyleHint(QFont::TypeWriter);
	viewFontRequest.setPointSize((selectedFontSize <= 0) ? 11 : selectedFontSize);
	setFont(viewFontRequest);

	m_view = m_machine.debug_view().alloc_view(
			type,
			DebuggerView::debuggerViewUpdate,
			this);

	connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &DebuggerView::verticalScrollSlot);
	connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &DebuggerView::horizontalScrollSlot);
}


DebuggerView::~DebuggerView()
{
	if (m_view)
		m_machine.debug_view().free_view(*m_view);
}

void DebuggerView::paintEvent(QPaintEvent *event)
{
	// Tell the MAME debug view how much real estate is available
	QFontMetrics actualFont = fontMetrics();
	const double fontWidth = actualFont.horizontalAdvance(QString(100, '_')) / 100.;
	const int fontHeight = std::max(1, actualFont.lineSpacing());
	m_view->set_visible_size(debug_view_xy(width()/fontWidth, height()/fontHeight));


	// Handle the scroll bars
	const int horizontalScrollCharDiff = m_view->total_size().x - m_view->visible_size().x;
	const int horizontalScrollSize = horizontalScrollCharDiff < 0 ? 0 : horizontalScrollCharDiff;
	horizontalScrollBar()->setRange(0, horizontalScrollSize);

	// If the horizontal scroll bar appears, make sure to adjust the vertical scrollbar accordingly
	const int verticalScrollAdjust = horizontalScrollSize > 0 ? 1 : 0;

	const int verticalScrollCharDiff = m_view->total_size().y - m_view->visible_size().y;
	const int verticalScrollSize = verticalScrollCharDiff < 0 ? 0 : verticalScrollCharDiff+verticalScrollAdjust;
	const bool atEnd = verticalScrollBar()->value() == verticalScrollBar()->maximum();
	verticalScrollBar()->setRange(0, verticalScrollSize);
	if (m_preferBottom && atEnd)
		verticalScrollBar()->setValue(verticalScrollSize);

	// Draw the viewport widget
	QPainter painter(viewport());
	painter.fillRect(0, 0, width(), height(), QBrush(Qt::white));
	painter.setBackgroundMode(Qt::OpaqueMode);
	painter.setBackground(QColor(255,255,255));

	// Background control
	QBrush bgBrush;
	bgBrush.setStyle(Qt::SolidPattern);
	painter.setPen(QPen(QColor(0,0,0)));

	size_t viewDataOffset = 0;
	const debug_view_xy& visibleCharDims = m_view->visible_size();
	const debug_view_char* viewdata = m_view->viewdata();
	for (int y = 0; y < visibleCharDims.y; y++)
	{
		int width = 1;
		for (int x = 0; x < visibleCharDims.x; viewDataOffset += width, x += width)
		{
			const unsigned char textAttr = viewdata[viewDataOffset].attrib;

			// Text color handling
			QColor fgColor(0,0,0);
			QColor bgColor(255,255,255);

			if (textAttr & DCA_VISITED)
				bgColor.setRgb(0xc6, 0xe2, 0xff);

			if (textAttr & DCA_ANCILLARY)
				bgColor.setRgb(0xe0, 0xe0, 0xe0);

			if (textAttr & DCA_SELECTED)
				bgColor.setRgb(0xff, 0x80, 0x80);

			if (textAttr & DCA_CURRENT)
				bgColor.setRgb(0xff, 0xff, 0x00);

			if ((textAttr & DCA_SELECTED) && (textAttr & DCA_CURRENT))
				bgColor.setRgb(0xff,0xc0,0x80);

			if (textAttr & DCA_CHANGED)
				fgColor.setRgb(0xff, 0x00, 0x00);

			if (textAttr & DCA_INVALID)
				fgColor.setRgb(0x00, 0x00, 0xff);

			if (textAttr & DCA_DISABLED)
			{
				fgColor.setRgb(
						(fgColor.red()   + bgColor.red())   >> 1,
						(fgColor.green() + bgColor.green()) >> 1,
						(fgColor.blue()  + bgColor.blue())  >> 1);
			}

			if(textAttr & DCA_COMMENT)
				fgColor.setRgb(0x00, 0x80, 0x00);

			bgBrush.setColor(bgColor);
			painter.setBackground(bgBrush);
			painter.setPen(QPen(fgColor));

			QString text(QChar(viewdata[viewDataOffset].byte));
			for (width = 1; x + width < visibleCharDims.x; width++)
			{
				if (textAttr != viewdata[viewDataOffset + width].attrib)
					break;
				text.append(QChar(viewdata[viewDataOffset + width].byte));
			}

			// Your characters are not guaranteed to take up the entire length x fontWidth x fontHeight, so fill before.
			painter.fillRect(x*fontWidth, y*fontHeight, width*fontWidth, fontHeight, bgBrush);

			// There is a touchy interplay between font height, drawing difference, visible position, etc
			// Fonts don't get drawn "down and to the left" like boxes, so some wiggling is needed.
			painter.drawText(x*fontWidth, (y*fontHeight + (fontHeight*0.80)), text);
		}
	}
}

void DebuggerView::keyPressEvent(QKeyEvent* event)
{
	if (m_view == nullptr)
		return QWidget::keyPressEvent(event);

	Qt::KeyboardModifiers keyMods = QApplication::keyboardModifiers();
	const bool ctrlDown = keyMods.testFlag(Qt::ControlModifier);

	int keyPress = -1;
	switch (event->key())
	{
	case Qt::Key_Up:
		keyPress = DCH_UP;
		break;
	case Qt::Key_Down:
		keyPress = DCH_DOWN;
		break;
	case Qt::Key_Left:
		keyPress = DCH_LEFT;
		if (ctrlDown)
			keyPress = DCH_CTRLLEFT;
		break;
	case Qt::Key_Right:
		keyPress = DCH_RIGHT;
		if (ctrlDown)
			keyPress = DCH_CTRLRIGHT;
		break;
	case Qt::Key_PageUp:
		keyPress = DCH_PUP;
		break;
	case Qt::Key_PageDown:
		keyPress = DCH_PDOWN;
		break;
	case Qt::Key_Home:
		keyPress = DCH_HOME;
		if (ctrlDown) keyPress = DCH_CTRLHOME;
		break;
	case Qt::Key_End:
		keyPress = DCH_END;
		if (ctrlDown) keyPress = DCH_CTRLEND;
		break;
	case Qt::Key_0: keyPress = '0'; break;
	case Qt::Key_1: keyPress = '1'; break;
	case Qt::Key_2: keyPress = '2'; break;
	case Qt::Key_3: keyPress = '3'; break;
	case Qt::Key_4: keyPress = '4'; break;
	case Qt::Key_5: keyPress = '5'; break;
	case Qt::Key_6: keyPress = '6'; break;
	case Qt::Key_7: keyPress = '7'; break;
	case Qt::Key_8: keyPress = '8'; break;
	case Qt::Key_9: keyPress = '9'; break;
	case Qt::Key_A: keyPress = 'a'; break;
	case Qt::Key_B: keyPress = 'b'; break;
	case Qt::Key_C: keyPress = 'c'; break;
	case Qt::Key_D: keyPress = 'd'; break;
	case Qt::Key_E: keyPress = 'e'; break;
	case Qt::Key_F: keyPress = 'f'; break;
	default:
		return QWidget::keyPressEvent(event);
	}

	m_view->set_cursor_visible(true);
	m_view->process_char(keyPress);

	// Catch the view up with the cursor
	verticalScrollBar()->setValue(m_view->visible_position().y);

	viewport()->update();
	update();
}


void DebuggerView::mousePressEvent(QMouseEvent *event)
{
	if (!m_view)
		return;

	QFontMetrics actualFont = fontMetrics();
	const double fontWidth = actualFont.horizontalAdvance(QString(100, '_')) / 100.;
	const int fontHeight = std::max(1, actualFont.lineSpacing());

	debug_view_xy topLeft = m_view->visible_position();
	debug_view_xy clickViewPosition;
	clickViewPosition.x = topLeft.x + (event->x() / fontWidth);
	clickViewPosition.y = topLeft.y + (event->y() / fontHeight);

	if (event->button() == Qt::LeftButton)
	{
		m_view->process_click(DCK_LEFT_CLICK, clickViewPosition);
	}
	else if (event->button() == Qt::MiddleButton)
	{
		m_view->process_click(DCK_MIDDLE_CLICK, clickViewPosition);
	}
	else if (event->button() == Qt::RightButton)
	{
		if (m_view->cursor_supported())
		{
			m_view->set_cursor_position(clickViewPosition);
			m_view->set_cursor_visible(true);
		}
	}

	viewport()->update();
	update();
}


void DebuggerView::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *const menu = new QMenu(this);
	addItemsToContextMenu(menu);
	menu->popup(event->globalPos());
}


void DebuggerView::addItemsToContextMenu(QMenu *menu)
{
	QAction *const copyAct = new QAction("Copy Visible", menu);
	QAction *const pasteAct = new QAction("Paste", menu);
	pasteAct->setEnabled(QApplication::clipboard()->mimeData()->hasText());
	connect(copyAct, &QAction::triggered, this, &DebuggerView::copyVisibleSlot);
	connect(pasteAct, &QAction::triggered, this, &DebuggerView::pasteSlot);
	menu->addAction(copyAct);
	menu->addAction(pasteAct);
}


void DebuggerView::verticalScrollSlot(int value)
{
	m_view->set_visible_position(debug_view_xy(horizontalScrollBar()->value(), value));
}


void DebuggerView::horizontalScrollSlot(int value)
{
	m_view->set_visible_position(debug_view_xy(value, verticalScrollBar()->value()));
}


void DebuggerView::copyVisibleSlot()
{
	// get visible text
	debug_view_xy const visarea = m_view->visible_size();
	debug_view_char const *viewdata = m_view->viewdata();
	if (!viewdata)
		return;

	// turn into a plain string, trimming trailing whitespace
	std::string text;
	for (uint32_t row = 0; row < visarea.y; row++, viewdata += visarea.x)
	{
		std::string::size_type const start = text.length();
		for (uint32_t col = 0; col < visarea.x; ++col)
			text += wchar_t(viewdata[col].byte);
		std::string::size_type const nonblank = text.find_last_not_of("\t\n\v\r ");
		if ((nonblank != std::string::npos) && (nonblank >= start))
			text.resize(nonblank + 1);
		text += "\n";
	}

	// copy to the clipboard
	QApplication::clipboard()->setText(text.c_str());
}


void DebuggerView::pasteSlot()
{
	for (QChar ch : QApplication::clipboard()->text())
	{
		if ((32 <= ch.unicode()) && (127 >= ch.unicode()))
			m_view->process_char(ch.unicode());
	}
}


void DebuggerView::debuggerViewUpdate(debug_view &debugView, void *osdPrivate)
{
	// Get a handle to the DebuggerView being updated and redraw
	DebuggerView *dView = reinterpret_cast<DebuggerView *>(osdPrivate);
	dView->verticalScrollBar()->setValue(dView->view()->visible_position().y);
	dView->horizontalScrollBar()->setValue(dView->view()->visible_position().x);
	dView->viewport()->update();
	dView->update();
	emit dView->updated();
}
