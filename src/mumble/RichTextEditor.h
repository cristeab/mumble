// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_RICHTEXTEDITOR_H_
#define MUMBLE_MUMBLE_RICHTEXTEDITOR_H_

#include <QtCore/QtGlobal>
#if QT_VERSION >= 0x050000
# include <QtWidgets/QTextEdit>
#else
# include <QtGui/QTextEdit>
#endif

class LogDocument;

class RichTextHtmlEdit : public QTextEdit {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(RichTextHtmlEdit)
	protected:
		void insertFromMimeData(const QMimeData *source);
	public:
		RichTextHtmlEdit(QWidget *p);
	private:
		LogDocument *m_document;
};

class RichTextEditorLink : public QDialog {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(RichTextEditorLink)
	public:
		RichTextEditorLink(const QString &text = QString(), QWidget *p = NULL);
        QString text() const;
};

class RichTextEditor : public QTabWidget {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(RichTextEditor)
	protected:
		bool bModified;
		bool bChanged;
		bool bReadOnly;
		QColor qcColor;
		bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;
	public:
		RichTextEditor(QWidget *p = NULL);
		bool isModified() const;
	signals:
		/// The accept signal is emitted when Ctrl-Enter is pressed inside the RichTextEditor.
		void accept();
	public slots:
		void setText(const QString &text, bool readonly = false);
		void updateColor(const QColor &);
	protected slots:
		void on_qaImage_triggered();

		void on_qptePlainText_textChanged();
		void onCurrentChanged(int);
};

class RichTextImage {
	public:
		static bool isValidImage(const QByteArray &buf, QByteArray &fmt);
};

#endif
