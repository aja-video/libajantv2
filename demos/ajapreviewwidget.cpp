/* SPDX-License-Identifier: MIT */
#include "ajapreviewwidget.h"

/***************************************************************************************/


AJAPreviewWidget::AJAPreviewWidget (QWidget *parent)
	:	QFrame (parent)
{
	qRegisterMetaType<QImage>("QImage");
	qRegisterMetaType<ROIRectList>("ROIRectList");
	setAcceptDrops(true);
	#if defined (INCLUDE_AJACC)
		for (unsigned ndx (0);  ndx < 15;  ndx++)
			_captionStrings [ndx] = QString (32, QChar (32));
	#endif	//	defined (INCLUDE_AJACC)
	_clear = false;
	_render = false;
}

void AJAPreviewWidget::paintEvent (QPaintEvent * /* event */)
{
	QPainter painter (this);

	const int offsetX	(_pixmap.width()  < width()  ? (width()  - _pixmap.width())  / 2 : 0);
	const int offsetY	(_pixmap.height() < height() ? (height() - _pixmap.height()) / 2 : 0);

	painter.drawPixmap (offsetX, offsetY, _pixmap);
	for (int i (0);  i < _roiList.size();  i++)
	{
		const ROIStruct & roiRectInfo	(_roiList [i]);
		//qDebug() << "ROI:" << roiRectInfo.roiRect.left() << roiRectInfo.roiRect.right() << roiRectInfo.roiRect.top() << roiRectInfo.roiRect.bottom();
		const double	hScale	(double (_pixmap.width()) / double (_unscaledImageSize.width()));
		const double	vScale	(double (_pixmap.height()) / double (_unscaledImageSize.height()));
		//qDebug() << "hScale=" << hScale << "vScale=" << vScale;
		if (roiRectInfo.display)
		{
			QPen currentPen (roiRectInfo.roiRectColor);
			currentPen.setWidth (4);
			painter.setPen (currentPen);
			QRectF currentRect;	//(roiRectInfo.roiRect);
			currentRect.setLeft   (double(offsetX)  +  roiRectInfo.roiRect.left()   * hScale);
			currentRect.setRight  (double(offsetX)  +  roiRectInfo.roiRect.right()  * hScale);
			currentRect.setTop    (double(offsetY)  +  roiRectInfo.roiRect.top()    * vScale);
			currentRect.setBottom (double(offsetY)  +  roiRectInfo.roiRect.bottom() * vScale);
			//qDebug() << "ROI2:" << currentRect.left() << currentRect.right() << currentRect.top() << currentRect.bottom();
			painter.drawRect (currentRect);
		}
	}

	//	Draw the status message using a dark gray outline and white interior, for better visibility...
	QFont f ("Lucida");
	f.setPointSize(32);
	f.setStyleStrategy(QFont::PreferOutline);
	f.setLetterSpacing(QFont::PercentageSpacing, 110.0);
	f.setWeight(QFont::Bold);
	const QBrush whiteBrush(Qt::white);
	const QPen blackPen(QBrush(Qt::darkGray), 2.0);
	QPainterPath pp;
	pp.addText(QPointF(30.0, 40.0), f, _statusString);
	painter.fillPath(pp, whiteBrush);
	painter.strokePath(pp, blackPen);
//	setFont(f);
//	painter.setPen (qRgba (255, 255, 255, 255));
//	painter.drawText (30, 40, _statusString);

	#if defined (INCLUDE_AJACC)
		QFont			captionFont ("Courier");
		captionFont.setPointSize (24);
		QFontMetrics	metrics (captionFont);
		const int		offX	(width()  / 2  -  32 * metrics.averageCharWidth() / 2);
		const int		offY	(height() / 2  -  15 * (metrics.height() + metrics.leading()) / 2);
		setFont (captionFont);
		painter.setPen (qRgba (255, 255, 255, 0));
		for (unsigned ndx (0);  ndx < 15;  ndx++)
			painter.drawText (offX, offY + ndx * (metrics.height() + metrics.leading()), _captionStrings [ndx]);
	#endif	//	defined (INCLUDE_AJACC)

	_render = false;
}	//	paintEvent


void AJAPreviewWidget::updateFrame (const QImage &image, bool clear)
{
//	qDebug() << "Update Frame";
	_unscaledImageSize.setHeight(image.height());
	_unscaledImageSize.setWidth(image.width());
	_image = image;
	_clear = clear;

	if (!_render)
	{
		_render = true;
		QMetaObject::invokeMethod(this, "renderFrame", Qt::QueuedConnection);
	}
}

void AJAPreviewWidget::updateFrameWithStatus (const QImage &image, const QString& statusString, bool clear)
{
//    qDebug() << "Update Frame With Status";

	_statusString = statusString;
	_unscaledImageSize.setHeight(image.height());
	_unscaledImageSize.setWidth(image.width());
	_image = image;
	_clear = clear;

	if (!_render)
	{
		_render = true;
		QMetaObject::invokeMethod(this, "renderFrame", Qt::QueuedConnection);
	}
}

void AJAPreviewWidget::updateFrameWithROI (const QImage & image, ROIRectList roiList, bool clear)
{
	_roiList = roiList;
	_unscaledImageSize.setHeight (image.height());
	_unscaledImageSize.setWidth (image.width());
	_image = image;
	_clear = clear;

	if (!_render)
	{
		_render = true;
		QMetaObject::invokeMethod(this, "renderFrame", Qt::QueuedConnection);
	}
}

void AJAPreviewWidget::updateROI (ROIRectList roiList)
{
	_roiList = roiList;
}

void AJAPreviewWidget::updateStatusString(const QString statusString)
{
	_statusString = statusString;
}

#if defined (INCLUDE_AJACC)
	void AJAPreviewWidget::updateCaptionScreen (const ushort * pInScreen)
	{
		const ushort *	pCell	(pInScreen);
		for (UWord row (NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
			for (UWord col (NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
				_captionStrings [row-1].replace (col-1, 1, QChar (*pCell++));
	}
#endif	//	defined (INCLUDE_AJACC)

void AJAPreviewWidget::renderFrame ()
{
//	qDebug() << "Render Frame";
	if (_image.width() > this->width())
		_pixmap = QPixmap::fromImage(_image.scaledToWidth(this->width(), Qt::SmoothTransformation), Qt::ImageConversionFlags());
	else if (_image.height() > this->height())
		_pixmap = QPixmap::fromImage(_image.scaledToHeight(this->height(), Qt::SmoothTransformation), Qt::ImageConversionFlags());
	else
		_pixmap = QPixmap::fromImage(_image, Qt::ImageConversionFlags());

	setAttribute(Qt::WA_OpaquePaintEvent, _clear ? false : true);
	repaint();
}


void AJAPreviewWidget::dragEnterEvent( QDragEnterEvent *ev )
{
	if (ev->mimeData()->hasFormat("text/uri-list"))
		ev->acceptProposedAction();
	else
		ev->setAccepted( false );
}


void AJAPreviewWidget::dropEvent (QDropEvent * ev)
{
	QList<QUrl> urls = ev->mimeData()->urls();
	if (urls.isEmpty())
		return;

	QString fileName = urls.first().toLocalFile();
	if (fileName.isEmpty())
		return;

	QImage imageFile(fileName);
	if ( !imageFile.isNull())
	{
		updateFrame(imageFile,true);
		return;
	}
	emit droppedFile(fileName);
}
