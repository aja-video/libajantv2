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
			currentRect.setLeft   ((double) offsetX  +  roiRectInfo.roiRect.left()   * hScale);
			currentRect.setRight  ((double) offsetX  +  roiRectInfo.roiRect.right()  * hScale);
			currentRect.setTop    ((double) offsetY  +  roiRectInfo.roiRect.top()    * vScale);
			currentRect.setBottom ((double) offsetY  +  roiRectInfo.roiRect.bottom() * vScale);
			//qDebug() << "ROI2:" << currentRect.left() << currentRect.right() << currentRect.top() << currentRect.bottom();
			painter.drawRect (currentRect);
		}
	}

	QFont f ("Lucida");
	f.setPointSize (18);
	setFont (f);
	painter.setPen (qRgba (255, 255, 255, 255));
	painter.drawText (30, 40, _statusString);

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

}	//	paintEvent


void AJAPreviewWidget::updateFrame (const QImage &image,bool clear)
{
	_unscaledImageSize.setHeight(image.height());
	_unscaledImageSize.setWidth(image.width());

    if ( image.width() > this->width())
        _pixmap = QPixmap::fromImage(image.scaledToWidth(this->width(),Qt::FastTransformation),0);
    else if ( image.height() > this->height())
        _pixmap = QPixmap::fromImage(image.scaledToHeight(this->height(),Qt::FastTransformation),0);
    else
        _pixmap = QPixmap::fromImage(image,0);

	setAttribute(Qt::WA_OpaquePaintEvent, clear ? false : true);
	repaint();
}


void AJAPreviewWidget::updateFrameWithStatus (const QImage &image,const QString& statusString,bool clear)
{
	_statusString = statusString;
	_unscaledImageSize.setHeight(image.height());
	_unscaledImageSize.setWidth(image.width());

	QPixmap tempPixmap = QPixmap::fromImage(image,0);
	if ( image.width() > this->width())
		_pixmap = tempPixmap.scaledToWidth(this->width(),Qt::FastTransformation);
	else if ( image.height() > this->height())
		_pixmap = tempPixmap.scaledToHeight(this->height(),Qt::FastTransformation);
	else
		_pixmap = tempPixmap;

	setAttribute(Qt::WA_OpaquePaintEvent, clear ? false : true);
	repaint();
}

void AJAPreviewWidget::updateFrameWithROI (const QImage & image, ROIRectList roiList, bool clear)
{
	_roiList = roiList;
	_unscaledImageSize.setHeight (image.height());
	_unscaledImageSize.setWidth (image.width());
	updateFrame (image, clear);
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

void AJAPreviewWidget::wheelEvent ( QWheelEvent * event )
{
	int delta = event->delta();
	qDebug("wheel delta = %d",delta);
	emit wheelDelta(delta);
}

void AJAPreviewWidget::dragEnterEvent( QDragEnterEvent *ev )
{
	if (ev->mimeData()->hasFormat("text/uri-list"))
		ev->acceptProposedAction();
	else
		ev->setAccepted( false );
}


void AJAPreviewWidget::dropEvent ( QDropEvent * ev )
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

#if 0
	QFile rmfFile(fileName);
	rmfFile.open(QIODevice::ReadOnly);
	unsigned int num32BitWordsInImage = (1930*8*1089*2)/(3*4);
	unsigned int* rawBuffer = new unsigned int[num32BitWordsInImage];
	//bool s = rmfFile.seek(0x10000);
	rmfFile.read((char*)rawBuffer,num32BitWordsInImage*4);
	rmfFile.close();
	QTime t;
	t.start();

	//QString rawFileName = fileName + ".canonraw";
	//QFile canonRawFile(rawFileName);
	//canonRawFile.open(QIODevice::WriteOnly);
	//canonRawFile.write((const char*)rawBuffer,num32BitWordsInImage*4);
	//canonRawFile.close();

	// first make byte array 
	unsigned char* rgbBuffer = new unsigned char[3860*2178];

	// 1930 RG Pairs per line * 8 bytes per 3 RG Pairs * 1089 RG Pair Lines
	// Plus
	// 1930 BG Pairs per line * 8 bytes per 3 BG Pairs * 1089 BG Pair Lines
	unsigned char* buffer = rgbBuffer;
	for (unsigned int rawCount = 0; rawCount < (num32BitWordsInImage); rawCount++ )
	{
		unsigned int currentValue = rawBuffer[rawCount];
		*buffer++ = (currentValue >> 4);
		*buffer++ = (currentValue >> 14);
		*buffer++ = (currentValue >> 24);
	}

 

	// now we have an 8 bit array with 1930 RG 8 bit pairs followed by 1930 BG 8 bit pairs for 2178 lines.
	// we will now grab 1920x1080 RGB32 out of this array
	// start by just taking upper left out of array
	QImage previewImage(1920,1080,QImage::Format_RGB32); 
	unsigned char* pBits = (unsigned char*) previewImage.bits();
	for ( unsigned int lineCount = 0; lineCount < 1080; lineCount++ )
	{
		unsigned char* lineBuffer = &rgbBuffer[lineCount*3860*2];
		for ( unsigned int pixelCount = 0; pixelCount < 1920; pixelCount++ )
		{
			unsigned char r = lineBuffer[pixelCount*2];
			unsigned char g = lineBuffer[pixelCount*2+1];
			unsigned char b = lineBuffer[1930*2+(pixelCount*2)+1];
			//uint8_t a = 0xFF;
			*pBits++ = b; //Blue
			*pBits++ = g; //Green
			*pBits++ = r; //Red
			*pBits++ = 0xFF; // Alpha

		}
	}


	updateFrame(previewImage,true);
	qDebug("Time elapsed: %d ms", t.elapsed());

	delete [] rgbBuffer; rgbBuffer = 0;
	delete [] rawBuffer; rawBuffer = 0;
#endif
}

