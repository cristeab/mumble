// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "RichTextEditor.h"

#include "Log.h"
#include "MainWindow.h"
#include "XMLTools.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

RichTextHtmlEdit::RichTextHtmlEdit(QWidget *p) : QTextEdit(p) {
	m_document = new LogDocument(this);
	m_document->setDefaultStyleSheet(qApp->styleSheet());
	setDocument(m_document);
}

/* On nix, some programs send utf8, some send wchar_t. Some zeroterminate once, some twice, some not at all.
 */

static QString decodeMimeString(const QByteArray &src) {
	if (src.isEmpty())
		return QString();

	if ((src.length() >= 4) && ((src.length() % sizeof(ushort)) == 0)) {
		const ushort *ptr = reinterpret_cast<const ushort *>(src.constData());
		int len = static_cast<int>(src.length() / sizeof(ushort));
		if ((ptr[0] > 0) && (ptr[0] < 0x7f) && (ptr[1] > 0) && (ptr[1] < 0x7f)) {
			while (len && (ptr[len - 1] == 0))
				--len;
			return QString::fromUtf16(ptr, len);
		}
	}

	if ((sizeof(wchar_t) != sizeof(ushort)) && (src.length() >= static_cast<int>(sizeof(wchar_t))) && ((src.length() % sizeof(wchar_t)) == 0)) {
		const wchar_t *ptr = reinterpret_cast<const wchar_t *>(src.constData());
		int len = static_cast<int>(src.length() / sizeof(wchar_t));
		if (*ptr < 0x7f) {
			while (len && (ptr[len - 1] == 0))
				--len;
			return QString::fromWCharArray(ptr, len);
		}
	}
	const char *ptr = src.constData();
	int len = src.length();
	while (len && (ptr[len - 1] == 0))
		--len;
	return QString::fromUtf8(ptr, len);
}

/* Try really hard to properly decode Mime into something sane.
 */

void RichTextHtmlEdit::insertFromMimeData(const QMimeData *source) {
	QString uri;
	QString title;
	QRegExp newline(QLatin1String("[\\r\\n]"));

#ifndef QT_NO_DEBUG
	qWarning() << "RichTextHtmlEdit::insertFromMimeData" << source->formats();
#endif

	if (source->hasImage()) {
		QImage img = qvariant_cast<QImage>(source->imageData());
		QString html = Log::imageToImg(img);
		if (! html.isEmpty())
			insertHtml(html);
		return;
	}

	QString mozurl = decodeMimeString(source->data(QLatin1String("text/x-moz-url")));
	if (! mozurl.isEmpty()) {
		QStringList lines = mozurl.split(newline);
		qWarning() << mozurl << lines;
		if (lines.count() >= 2) {
			uri = lines.at(0);
			title = lines.at(1);
		}
	}

	if (uri.isEmpty())
		uri = decodeMimeString(source->data(QLatin1String("text/x-moz-url-data")));
	if (title.isEmpty())
		title = decodeMimeString(source->data(QLatin1String("text/x-moz-url-desc")));

	if (uri.isEmpty()) {
		QStringList urls;
#ifdef Q_OS_WIN
		urls = decodeMimeString(source->data(QLatin1String("application/x-qt-windows-mime;value=\"UniformResourceLocatorW\""))).split(newline);
		if (urls.isEmpty())
#endif
			urls = decodeMimeString(source->data(QLatin1String("text/uri-list"))).split(newline);
		if (! urls.isEmpty())
			uri = urls.at(0).trimmed();
	}

	if (uri.isEmpty()) {
		QUrl url(source->text(), QUrl::StrictMode);
		if (url.isValid() && ! url.isRelative()) {
			uri = url.toString();
		}
	}

#ifdef Q_OS_WIN
	if (title.isEmpty() && source->hasFormat(QLatin1String("application/x-qt-windows-mime;value=\"FileGroupDescriptorW\""))) {
		QByteArray qba = source->data(QLatin1String("application/x-qt-windows-mime;value=\"FileGroupDescriptorW\""));
		if (qba.length() == sizeof(FILEGROUPDESCRIPTORW)) {
			const FILEGROUPDESCRIPTORW *ptr = reinterpret_cast<const FILEGROUPDESCRIPTORW *>(qba.constData());
			title = QString::fromWCharArray(ptr->fgd[0].cFileName);
			if (title.endsWith(QLatin1String(".url"), Qt::CaseInsensitive))
				title = title.left(title.length() - 4);
		}
	}
#endif

	if (! uri.isEmpty()) {
		if (title.isEmpty())
			title = uri;
#if QT_VERSION >= 0x050000
		uri = uri.toHtmlEscaped();
		title = title.toHtmlEscaped();
#else
		uri = Qt::escape(uri);
		title = Qt::escape(title);
#endif

		insertHtml(QString::fromLatin1("<a href=\"%1\">%2</a>").arg(uri, title));
		return;
	}

	QString html = decodeMimeString(source->data(QLatin1String("text/html")));
	if (! html.isEmpty()) {
		insertHtml(html);
		return;
	}

	QTextEdit::insertFromMimeData(source);
}

RichTextEditorLink::RichTextEditorLink(const QString &txt, QWidget *p) : QDialog(p) {
}

QString RichTextEditorLink::text() const {
	return QString();
}

RichTextEditor::RichTextEditor(QWidget *p) : QTabWidget(p) {
	bChanged = false;
	bModified = false;
	bReadOnly = false;
}

bool RichTextEditor::isModified() const {
	return bModified;
}

void RichTextEditor::on_qaImage_triggered() {
	QPair<QByteArray, QImage> choice = g.mw->openImageFile();

	QByteArray &qba = choice.first;

	if (qba.isEmpty())
		return;

	if ((g.uiImageLength > 0) && (static_cast<unsigned int>(qba.length()) > g.uiImageLength)) {
		QMessageBox::warning(this, tr("Failed to load image"), tr("Image file too large to embed in document. Please use images smaller than %1 kB.").arg(g.uiImageLength /1024));
		return;
	}

	QBuffer qb(&qba);
	qb.open(QIODevice::ReadOnly);

	QByteArray format = QImageReader::imageFormat(&qb);
	qb.close();
}

void RichTextEditor::onCurrentChanged(int index) {
	if (! bChanged)
		return;

	bChanged = false;
}

void RichTextEditor::on_qptePlainText_textChanged() {
	bModified = true;
	bChanged = true;
}

void RichTextEditor::updateColor(const QColor &col) {
	if (col == qcColor)
		return;
	qcColor = col;

	QRect r(0,0,24,24);

	QPixmap qpm(r.size());
	QPainter qp(&qpm);
	qp.fillRect(r, col);
	qp.setPen(col.darker());
	qp.drawRect(r.adjusted(0, 0, -1, -1));
}

void RichTextEditor::setText(const QString &txt, bool readonly) {
	bChanged = false;
	bModified = false;
	bReadOnly = readonly;
}

bool RichTextEditor::eventFilter(QObject *obj, QEvent *evt) {
	return false;
}

bool RichTextImage::isValidImage(const QByteArray &ba, QByteArray &fmt) {
	QBuffer qb;
	qb.setData(ba);
	if (!qb.open(QIODevice::ReadOnly)) {
		return false;
	}

	QByteArray detectedFormat = QImageReader::imageFormat(&qb).toLower();
	if (detectedFormat == QByteArray("png") || detectedFormat == QByteArray("jpg")
            || detectedFormat == QByteArray("jpeg") || detectedFormat == QByteArray("gif")) {
		fmt = detectedFormat;
		return true;
	}

	return false;
}
