//
// MIT License
//
// Copyright (c) 2019 Hubert Gruniaux
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

#include "editor.hpp"

// ========================================================
// class Editor
// ========================================================

Editor::Editor(QWidget *parent) : QWidget(parent)
{
    m_image = QPixmap();
    m_watermarkPreview = new WatermarkEditor(this);
    m_croppingPreview = new CropEditor(this);
    connect(m_croppingPreview, &CropEditor::cropResized, this, &Editor::cropResized);
    connect(m_croppingPreview, &CropEditor::cropMoved, this, &Editor::cropMoved);
    connect(m_croppingPreview, &CropEditor::cropEdited, this, &Editor::cropEdited);
    connect(m_croppingPreview, &CropEditor::cropEdited, [this]() { m_watermarkPreview->setCrop(m_croppingPreview->m_crop); emit edited(); });
    setWatermarkAlpha(1.0);
    setWatermarkSize(1.0);
    setWatermarkAnchor(AnchorCenter);
}

QPixmap Editor::generate() const
{
    if(m_image.isNull()) {
        return QPixmap();
    } else {
        QRect crop = m_croppingPreview->m_crop;
        QPixmap result = m_image.copy(crop);
        QPainter painter(&result);
        m_watermarkPreview->drawWatermark(&painter, false);
        return result;
    }
}

QPointF Editor::zoomFactor() const
{
    return m_factor;
}
QRect Editor::mapTo(const QRect& rect) const
{
    QPointF factor = zoomFactor();
    return QRect(QPoint(qRound(rect.x() * factor.x()), qRound(rect.y() * factor.y())),
                 QSize(qRound(rect.width() * factor.x()), qRound(rect.height() * factor.y())));
}
QPoint Editor::mapTo(const QPoint& point) const
{
    QPointF factor = zoomFactor();
    return QPoint(qRound(point.x() * factor.x()), qRound(point.y() * factor.y()));
}
QSize Editor::mapTo(const QSize& size) const
{
    QPointF factor = zoomFactor();
    return QSize(qRound(size.width() * factor.x()), qRound(size.height() * factor.y()));
}
QRect Editor::mapFrom(const QRect& rect) const
{
    QPointF factor = zoomFactor();
    return QRect(QPoint(qRound(rect.x() / factor.x()), qRound(rect.y() / factor.y())),
                 QSize(qRound(rect.width() / factor.x()), qRound(rect.height() / factor.y())));
}
QPoint Editor::mapFrom(const QPoint& point) const
{
    QPointF factor = zoomFactor();
    return QPoint(qRound(point.x() / factor.x()), qRound(point.y() / factor.y()));
}
QSize Editor::mapFrom(const QSize& size) const
{
    QPointF factor = zoomFactor();
    return QSize(qRound(size.width() / factor.x()), qRound(size.height() / factor.y()));
}

void Editor::setImage(const QPixmap& image)
{
    m_image = image;
    m_watermarkPreview->resize(image.size());
    m_watermarkPreview->setVisible(!image.isNull());
    m_watermarkPreview->update();
    m_croppingPreview->resize(image.size());
    m_croppingPreview->setVisible(!image.isNull());
    m_croppingPreview->update();
    zoom(1.0);
    emit edited();
}

void Editor::setCropRect(const QRect& rect)
{
    m_croppingPreview->m_crop = rect;
    m_watermarkPreview->m_crop = rect;
    m_croppingPreview->update();
    m_watermarkPreview->update();
    emit edited();
}
void Editor::setCropSize(const QSize& size)
{
    setCropRect(QRect(m_croppingPreview->m_crop.topLeft(), size));
}
void Editor::setCropPosition(const QPoint& pos)
{
     setCropRect(QRect(pos, m_croppingPreview->m_crop.size()));
}

void Editor::setWatermarkImage(const QPixmap& image)
{
    m_watermarkPreview->setWatermark(image);
    emit edited();
}
void Editor::setWatermarkAnchor(WatermarkAnchor anchor)
{
    m_watermarkPreview->m_anchor = anchor;
    m_watermarkPreview->updatePosition();
    m_watermarkPreview->update();
    emit edited();
}
void Editor::setWatermarkAlpha(qreal alpha)
{
    m_watermarkPreview->m_alpha = alpha;
    m_watermarkPreview->update();
    emit edited();
}
void Editor::setWatermarkSize(qreal size)
{
    m_watermarkPreview->m_size = size;
    m_watermarkPreview->update();
    emit edited();
}
void Editor::setWatermarkColor(const QColor &color)
{
    m_watermarkPreview->setColor(color);
    emit edited();
}
void Editor::setWatermarkResize(bool resize)
{
    m_watermarkPreview->m_resize = resize;
    m_watermarkPreview->update();
    emit edited();
}
void Editor::setWatermarkColorize(bool colorize)
{
    m_watermarkPreview->setColorize(colorize);
    emit edited();
}

void Editor::zoom(qreal factor)
{
    m_zoom = factor;
    m_imageRect.setSize(m_image.size().scaled(size() * factor, Qt::KeepAspectRatio));
    m_imageRect.moveTo(width()/2 - m_imageRect.width()/2, height()/2 - m_imageRect.height()/2);
    m_factor = QPointF(qreal(m_image.width()) / qreal(m_imageRect.width()),
                       qreal(m_image.height()) / qreal(m_imageRect.height()));
    updateEditors();
}

void Editor::resizeEvent(QResizeEvent*)
{
    zoom(m_zoom);
}
void Editor::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(m_imageRect, m_image);
}

void Editor::updateEditors()
{
    m_watermarkPreview->resize(m_imageRect.size());
    m_watermarkPreview->move(m_imageRect.topLeft());
    m_croppingPreview->resize(m_imageRect.size());
    m_croppingPreview->move(m_imageRect.topLeft());
}

// ========================================================
// class CropEditor
// ========================================================

CropEditor::CropEditor(QWidget *parent) : QWidget(parent)
{
    m_dragging = false;
    m_resizing = false;
    m_crop = QRect(0, 0, 100, 100);
    m_dragOrigin = QPoint();
    setMouseTracking(true);
}

void CropEditor::paintEvent(QPaintEvent*)
{
    const QColor backgroundColor = QColor(0, 0, 0, 128);
    const QColor borderColor = QColor(0xEE, 0xEE, 0xEE);
    const QColor gridColor = QColor(0xCC, 0xCC, 0xCC);
    const QColor textColor = QColor(0xFF, 0xFF, 0xFF);

    QPainter painter(this);
    QRect crop = editor()->mapFrom(m_crop);

    { // ===== Background =====
        // Configuration
        painter.setBrush(backgroundColor);
        painter.setPen(Qt::NoPen);
        // Points
        int left = crop.x();
        int top = crop.y();
        int right = crop.x() + crop.width();
        int bottom = crop.y() + crop.height();
        // Drawing
        painter.drawRect(0, 0, width(), top);
        painter.drawRect(0,  bottom, width(), height() - bottom);
        painter.drawRect(0, top, left, crop.height());
        painter.drawRect(right, top, width() - right, crop.height());
    }

    { // ===== Simple Border =====
        // Configuration
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(gridColor, 1));
        // Drawing
        painter.drawRect(crop.adjusted(0, 0, -1, -1));
    }

    { // ===== Border =====
        // Configuration
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(borderColor, 3));
        // Points
        int length = qMin(20, qMin(crop.width(), crop.height()));
        int left = crop.x();
        int top = crop.y();
        int right = crop.x() + crop.width()-1;
        int bottom = crop.y() + crop.height()-1;
        int startX = crop.x() + (crop.width()/2) - length/2;
        int endX = crop.x() + (crop.width()/2) + length/2;
        int startY = crop.y() + (crop.height()/2) - length/2;
        int endY = crop.y() + (crop.height()/2) + length/2;
        // Drawing
        { // ===== Corners =====
            // Top Left Corner
            painter.drawLine(left, top, left + length, top);
            painter.drawLine(left, top, left, top + length);
            // Top Right Corner
            painter.drawLine(right,top, right - length, top);
            painter.drawLine(right, top, right, top + length);
            // Bottom Left Corner
            painter.drawLine(left, bottom, left + length, bottom);
            painter.drawLine(left, bottom, left, bottom - length);
            // Bottom Right Corner
            painter.drawLine(right, bottom, right - length, bottom);
            painter.drawLine(right, bottom, right, bottom - length);
        }
        { // ===== Lines =====
            // Top Line
            painter.drawLine(startX, top, endX, top);
            // Left Line
            painter.drawLine(left, startY, left, endY);
            // Bottom Line
            painter.drawLine(startX, bottom, endX, bottom);
            // Right Line
            painter.drawLine(right, startY, right, endY);
        }
    }

    { // ===== Grid =====
        // Configuration
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(gridColor, 1));
        // Points
        int left = crop.x();
        int top = crop.y();
        int right = crop.x() + crop.width()-1;
        int bottom = crop.y() + crop.height()-1;
        int firstX = crop.x() + (crop.width() * 1/3);
        int secondX = crop.x() + (crop.width() * 2/3);
        int firstY = crop.y() + (crop.height() * 1/3);
        int secondY = crop.y() + (crop.height() * 2/3);
        // Drawing
        painter.drawLine(firstX, top, firstX, bottom);
        painter.drawLine(secondX, top, secondX, bottom);
        painter.drawLine(left, firstY, right, firstY);
        painter.drawLine(left, secondY, right, secondY);
    }

    { // ===== Text =====
        painter.save();
        // Configuration
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(textColor, 1));
        // Text
        QString width = QString("%0px").arg(m_crop.width());
        QString height = QString("%0px").arg(m_crop.height());
        // Points
        QFontMetrics metrics(painter.font());
        int padding = 0;
        int widthTextWidth = metrics.horizontalAdvance(width);
        int heightTextWidth = metrics.horizontalAdvance(height);
        int textHeight = metrics.height();
        double widthRadians = 12 * 2.0 * 3.141592654 / 12;
        int widthX = crop.center().x() + qRound((crop.width()/2) * qSin(widthRadians));
        int widthY = crop.center().y() + qRound((-crop.height()/2 - padding - textHeight) * qCos(widthRadians));
        double heightRadians = 9 * 2.0 * 3.141592654 / 12;
        int heightX = crop.center().x() + qRound((crop.width()/2 + padding + textHeight) * qSin(heightRadians));
        int heightY = crop.center().y() + qRound((-crop.height()/2) * qCos(heightRadians));
        // Drawing
        QTransform t1;
        t1.translate(widthX - widthTextWidth/2, widthY);
        t1.rotateRadians(widthRadians);
        painter.setTransform(t1);
        painter.drawText(0, 0, width);
        QTransform t2;
        t2.translate(heightX, heightY + heightTextWidth/2);
        t2.rotateRadians(heightRadians);
        painter.setTransform(t2);
        painter.drawText(0, 0, height);
        painter.restore();
    }
}
void CropEditor::mousePressEvent(QMouseEvent* event)
{
    auto corner = resizeCorner(event->pos());
    switch (corner) {
    case AnchorTopLeft:
    case AnchorBottomRight:
    case AnchorTopRight:
    case AnchorBottomLeft:
    case AnchorLeft:
    case AnchorRight:
    case AnchorTop:
    case AnchorBottom:
        m_resizing = true;
        m_resizingCorner = corner;
        break;
    default: {
        const QRect crop = editor()->mapFrom(m_crop);
            if(crop.contains(event->pos())) {
                setCursor(Qt::ClosedHandCursor);
                m_dragging = true;
                m_dragOrigin = editor()->mapTo(event->pos() - crop.topLeft());
            }
        } break;
    }
}
void CropEditor::mouseMoveEvent(QMouseEvent* event)
{
    QSize size = editor()->imageSize();
    if(m_dragging) {
        QPoint temp = editor()->mapTo(event->pos()) - m_dragOrigin;
        QPoint pos = QPoint(qBound(0, (size.width() - m_crop.width()), temp.x()),
                            qBound(0, (size.height() - m_crop.height()), temp.y()));
        m_crop.moveTo(pos);
        emit cropMoved(pos);
        emit cropEdited();
        update();
    } else if(m_resizing) {
        QRect rect = m_crop;
        QPoint tl = rect.topLeft();
        QPoint br = rect.bottomRight();
        QPoint pos = editor()->mapTo(event->pos());
        pos = QPoint(qBound(0, pos.x(), size.width()-1), qBound(0, pos.y(), size.height()-1));

        switch(m_resizingCorner) {
        case AnchorTopLeft: m_crop.setTopLeft(QPoint(qMin(pos.x(), br.x()), qMin(pos.y(), br.y()))); break;
        case AnchorTopRight: m_crop.setTopRight(QPoint(qMax(pos.x(), tl.x()), qMin(pos.y(), br.y()))); break;
        case AnchorBottomLeft: m_crop.setBottomLeft(QPoint(qMin(pos.x(), br.x()), qMax(pos.y(), tl.y()))); break;
        case AnchorBottomRight: m_crop.setBottomRight(QPoint(qMax(pos.x(), tl.x()), qMax(pos.y(), tl.y()))); break;
        case AnchorTop: m_crop.setTop(qMin(pos.y(), br.y())); break;
        case AnchorBottom: m_crop.setBottom(qMax(pos.y(), tl.y())); break;
        case AnchorLeft: m_crop.setLeft(qMin(pos.x(), br.x())); break;
        case AnchorRight: m_crop.setRight(qMax(pos.x(), tl.x())); break;
        default: break;
        }

        emit cropResized(m_crop.size());

        switch(m_resizingCorner) {
        case AnchorTopLeft: m_crop.moveBottomRight(rect.bottomRight()); break;
        case AnchorTopRight: m_crop.moveBottomLeft(rect.bottomLeft()); break;
        case AnchorBottomLeft: m_crop.moveTopRight(rect.topRight()); break;
        case AnchorBottomRight: m_crop.moveTopLeft(rect.topLeft()); break;
        case AnchorTop: m_crop.moveBottom(rect.bottom()); break;
        case AnchorBottom: m_crop.moveTop(rect.top()); break;
        case AnchorLeft: m_crop.moveRight(rect.right()); break;
        case AnchorRight: m_crop.moveLeft(rect.left()); break;
        default: break;
        }

        m_crop.setWidth(qMin(m_crop.width(), size.width() - m_crop.x()));
        m_crop.setHeight(qMin(m_crop.height(), size.height() - m_crop.y()));

        emit cropResized(m_crop.size());
        emit cropMoved(m_crop.topLeft());
        emit cropEdited();

        update();
    } else {
        auto anchor = resizeCorner(event->pos());
        switch (anchor) {
        case AnchorTopLeft:
        case AnchorBottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case AnchorTopRight:
        case AnchorBottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case AnchorLeft:
        case AnchorRight:
            setCursor(Qt::SizeHorCursor);
            break;
        case AnchorTop:
        case AnchorBottom:
            setCursor(Qt::SizeVerCursor);
            break;
        default:
                setCursor(Qt::ArrowCursor);
            break;
        }
    }
}
void CropEditor::mouseReleaseEvent(QMouseEvent*)
{
    setCursor(Qt::ArrowCursor);
    m_dragging = false;
    m_resizing = false;
}

WatermarkAnchor CropEditor::resizeCorner(const QPoint& pos)
{
    const QRect crop = editor()->mapFrom(m_crop);
    const int length = qMin(20, qMin(crop.width(), crop.height()));
    const QSize resizeSize(length, length);
    const QSize vResizer(length, 15);
    const QSize hResizer(15, length);
    const QPoint halfSize(resizeSize.width()/2, resizeSize.height()/2);
    const QPoint vHalfSize(vResizer.width()/2, vResizer.height()/2);
    const QPoint hHalfSize(hResizer.width()/2, hResizer.height()/2);
    if(QRect(crop.topLeft()-halfSize, resizeSize).contains(pos)) { // Top Left
        return AnchorTopLeft;
    } else if(QRect(crop.topRight()-halfSize, resizeSize).contains(pos)) { // Top Right
        return AnchorTopRight;
    } else if(QRect(crop.bottomLeft()-halfSize, resizeSize).contains(pos)) { // Bottom Left
        return AnchorBottomLeft;
    } else if(QRect(crop.bottomRight()-halfSize, resizeSize).contains(pos)) { // Bottom Right
        return AnchorBottomRight;
    } else if(QRect(QPoint(crop.left(), crop.y()+crop.height()/2)-vHalfSize, vResizer).contains(pos)) { // Left
        return AnchorLeft;
    } else if(QRect(QPoint(crop.right(), crop.y()+crop.height()/2)-vHalfSize, vResizer).contains(pos)) { // Right
        return AnchorRight;
    } else if(QRect(QPoint(crop.x()+crop.width()/2, crop.top())-hHalfSize, hResizer).contains(pos)) { // Top
        return AnchorTop;
    } else if(QRect(QPoint(crop.x()+crop.width()/2, crop.bottom())-hHalfSize, hResizer).contains(pos)) { // Bottom
        return AnchorBottom;
    } else {
        return AnchorCenter;
    }
}

// ========================================================
// class WatermarkEditor
// ========================================================

WatermarkEditor::WatermarkEditor(QWidget *parent) : QWidget(parent)
{
    m_crop = QRect(0, 0, 0, 0);
    m_resize = true;
    m_alpha = 1.;
    m_size = 1.;
    m_anchor = AnchorCenter;
    m_pos = QPoint(0, 0);
    m_watermark = QPixmap();
    m_coloredWatermark = QPixmap();
    m_color = QColor(0, 0, 0, 128);
    m_colorize = false;
}

void WatermarkEditor::drawWatermark(QPainter* painter, bool scaled)
{
    if(!m_watermark.isNull()) {
        painter->save();
        {
            // Computes the watermark position
            QRect crop = scaled ? editor()->mapFrom(m_crop) : m_crop;
            QRect rect;
            QSize size;
            if(m_resize) {
                size = m_watermark.size().scaled(crop.width() * 1/3, crop.height() * 1/3, Qt::KeepAspectRatio);
                size = size.scaled(size * m_size, Qt::KeepAspectRatio);
            } else {
                size = scaled ? editor()->mapFrom(m_watermark.size()) : m_watermark.size();
            }
            if(m_anchor != AnchorRepeated) {
                updatePosition();
                rect = QRect(scaled ? editor()->mapFrom(m_pos) : m_pos, size);
            }

            // Draw the watermark
            QPixmap image = (m_colorize ? m_coloredWatermark : m_watermark).scaled(size);
            if(scaled) painter->translate(crop.topLeft());
            painter->setOpacity(m_alpha);
            if(m_anchor != AnchorRepeated) {
                painter->drawPixmap(rect, image);
            } else {
                QBrush brush(image);
                painter->setBrush(brush);
                painter->setPen(Qt::NoPen);
                painter->drawRect(QRect(QPoint(0, 0), crop.size()));
            }
        }
        painter->restore();
    }
}

void WatermarkEditor::updatePosition()
{
    int cropWidth = m_crop.width();
    int cropHeight = m_crop.height();
    int halfCropWidth = m_crop.width()/2;
    int halfCropHeight = m_crop.height()/2;

    QSize size;
    if(m_resize) {
        size = m_watermark.size().scaled(m_crop.width() * 1/3, m_crop.height() * 1/3, Qt::KeepAspectRatio);
        size = size.scaled(size * m_size, Qt::KeepAspectRatio);
    } else {
        size = m_watermark.size();
    }
    int width = size.width();
    int height = size.height();
    int halfWidth = size.width() / 2;
    int halfHeight = size.height() / 2;

    switch(m_anchor) {
    case AnchorRepeated: m_pos = QPoint(0, 0); break;
    case AnchorTop: m_pos = QPoint(halfCropWidth - halfWidth, 0); break;
    case AnchorTopLeft: m_pos = QPoint(0, 0); break;
    case AnchorTopRight: m_pos = QPoint(cropWidth - width, 0); break;
    case AnchorLeft: m_pos = QPoint(0, halfCropHeight - halfHeight); break;
    case AnchorRight: m_pos = QPoint(cropWidth - width, halfCropHeight - halfHeight); break;
    case AnchorBottom: m_pos = QPoint(halfCropWidth - halfWidth, cropHeight - height); break;
    case AnchorBottomLeft: m_pos = QPoint(0, cropHeight - height); break;
    case AnchorBottomRight: m_pos = QPoint(cropWidth - width, cropHeight - height); break;
    case AnchorCenter: m_pos = QPoint(halfCropWidth - halfWidth, halfCropHeight - halfHeight); break;
    }
}
void WatermarkEditor::setCrop(const QRect& crop)
{
    m_crop = crop;
    updatePosition();
    update();
}
void WatermarkEditor::setWatermark(const QPixmap &pixmap)
{
    m_watermark = pixmap;
    if(m_colorize) {
        setColor(m_color);
    } else {
        m_coloredWatermark = m_watermark;
    }
    updatePosition();
    update();
}
void WatermarkEditor::setColor(const QColor &color)
{
    m_coloredWatermark = m_watermark;
    if(!m_coloredWatermark.isNull()) {
        QPainter painter(&m_coloredWatermark);
        painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        painter.fillRect(m_coloredWatermark.rect(), color);
        painter.end();
    }

    m_color = color;
    update();
}
void WatermarkEditor::setColorize(bool colorize)
{
    m_colorize = colorize;
    update();
}

void WatermarkEditor::paintEvent(QPaintEvent*)
{ 
    QPainter painter(this);
    drawWatermark(&painter);
}
