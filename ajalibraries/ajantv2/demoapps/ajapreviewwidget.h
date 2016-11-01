#ifndef AJAPREVIEW_WIDGET_H
#define AJAPREVIEW_WIDGET_H

#include <QBasicTimer>
#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	#include <QtWidgets>
#else
	#include <QtGui>
#endif
#include <QThread>
#include <QList>
#include <QRectF>
#if defined (INCLUDE_AJACC)
	#include "ajacc/includes/ntv2caption608types.h"
#endif	//	defined (INCLUDE_AJACC)

class AJAPreviewWidget;

#define AJAPREVIEW_WIDGET_X (960)
#define AJAPREVIEW_WIDGET_Y (540)

typedef struct 
{
	bool    display;
	QRectF  roiRect;  /// normalize to width and height 0.0-1.0
	QColor  roiRectColor;

} ROIStruct;

typedef QList<ROIStruct> ROIRectList;


class AJAPreviewWidget : public QFrame
{
	Q_OBJECT

	public:
		AJAPreviewWidget (QWidget * parent = 0);

	signals:
		void wheelDelta (int delta);
		void droppedFile (QString fileName);

	public slots:
		void updateFrame (const QImage &image,bool clear);
		void updateFrameWithStatus (const QImage &image,const QString &statusString,bool clear);
		void updateFrameWithROI (const QImage &image,ROIRectList roiList, bool clear);
		void updateROI (ROIRectList roiList);
		void updateStatusString (const QString statusString);
	#if defined (INCLUDE_AJACC)
		void updateCaptionScreen (const ushort * inScreen);
	#endif	//	defined (INCLUDE_AJACC)

	protected:
		void paintEvent (QPaintEvent * event);
		void wheelEvent (QWheelEvent * event);

		void dragEnterEvent (QDragEnterEvent *event);
		void dropEvent (QDropEvent *event);

	private:
		QPixmap				_pixmap;
		QString				_statusString;
		int					_step;
		ROIRectList			_roiList;
		QSize				_unscaledImageSize;
	#if defined (INCLUDE_AJACC)
		QString				_captionStrings [15];
	#endif	//	defined (INCLUDE_AJACC)

};	//	AJAPreviewWidget

#endif	//	AJAPREVIEW_WIDGET_H
