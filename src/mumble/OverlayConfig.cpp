// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "OverlayConfig.h"

#include "Overlay.h"
#include "OverlayUserGroup.h"
#include "OverlayPositionableItem.h"
#include "OverlayText.h"
#include "User.h"
#include "Channel.h"
#include "Message.h"
#include "Database.h"
#include "NetworkConfig.h"
#include "ServerHandler.h"
#include "MainWindow.h"
#include "GlobalShortcut.h"
#include "PathListWidget.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

#ifdef Q_OS_WIN
#include "../../overlay/overlay_launchers.h"
#include "../../overlay/overlay_whitelist.h"
#include "../../overlay/overlay_blacklist.h"
#endif

static const int OVERLAYCONFIG_PATH_ROLE = Qt::UserRole;
static const int OVERLAYCONFIG_BUILTIN_ROLE = Qt::UserRole + 1;

// Hide overlay config for Mac OS X universal builds
#if !defined(USE_MAC_UNIVERSAL)
static ConfigWidget *OverlayConfigDialogNew(Settings &st) {
	return new OverlayConfig(st);
}

static ConfigRegistrar registrar(6000, OverlayConfigDialogNew);
#endif

void OverlayConfig::initDisplayFps() {
	// set up FPS preview
	qgsFpsPreview.clear();

	qgpiFpsDemo = new QGraphicsPixmapItem();
	refreshFpsDemo();

	qgsFpsPreview.addItem(qgpiFpsDemo);
	qgpiFpsDemo->show();

	qgpiFpsLive = new OverlayPositionableItem(&s.os.qrfFps, true);
	qgpiFpsLive->setZValue(-2.0f);
	refreshFpsLive();
}

void OverlayConfig::initDisplayClock() {
	qgpiTimeLive = new OverlayPositionableItem(&s.os.qrfTime, true);
	qgpiTimeLive->setZValue(-2.0f);
	refreshTimeLive();
}

void OverlayConfig::initDisplay() {
	// set up overlay preview
	qgpiScreen = new QGraphicsPixmapItem();
	qgpiScreen->setPixmap(qpScreen);
	qgpiScreen->setOpacity(0.5f);
	qgpiScreen->setZValue(-10.0f);

	initDisplayFps();
	initDisplayClock();

	qgtiInstructions = new QGraphicsTextItem();
	qgtiInstructions->setHtml(QString::fromLatin1("<ul><li>%1</li><li>%2</li><li>%3</li></ul>").arg(
	                              tr("To move the users, drag the little red dot."),
	                              tr("To resize the users, mouse wheel over a user."),
	                              tr("For more options, right click a user.")
	                          ));
	qgtiInstructions->setOpacity(1.0f);
	qgtiInstructions->setZValue(-5.0f);
	qgtiInstructions->setDefaultTextColor(Qt::white);

	qgs.clear();
	qgs.setSceneRect(QRectF(0, 0, qgpiScreen->pixmap().width(), qgpiScreen->pixmap().height()));

	qgs.addItem(qgpiScreen);
	qgpiScreen->show();

	qgs.addItem(qgpiFpsLive);
	qgpiFpsLive->show();

	qgs.addItem(qgpiTimeLive);
	qgpiTimeLive->show();

	oug = new OverlayUserGroup(&s.os);
	oug->bShowExamples = true;
	qgs.addItem(oug);
	oug->show();

	qgs.addItem(qgtiInstructions);
	qgtiInstructions->show();
}

void OverlayConfig::refreshFpsDemo() {
	bpFpsDemo = OverlayTextLine(QString::fromLatin1("%1").arg(42), s.os.qfFps).createPixmap(s.os.qcFps);
	qgpiFpsDemo->setPixmap(bpFpsDemo);
}

void OverlayConfig::refreshFpsLive() {
	if (s.os.bFps) {
		qgpiFpsLive->setPixmap(bpFpsDemo.scaled(bpFpsDemo.size() * fViewScale));
		qgpiFpsLive->setOffset((-bpFpsDemo.qpBasePoint + QPoint(0, bpFpsDemo.iAscent)) * fViewScale);
	} else {
		qgpiFpsLive->setPixmap(QPixmap());
	}
	qgpiFpsLive->setItemVisible(s.os.bFps);
}

void OverlayConfig::refreshTimeLive() {
	if (s.os.bTime) {
		bpTimeDemo = OverlayTextLine(QString::fromLatin1("%1").arg(QTime::currentTime().toString()), s.os.qfFps).createPixmap(s.os.qcFps);
		qgpiTimeLive->setPixmap(bpTimeDemo.scaled(bpTimeDemo.size() * fViewScale));
		qgpiTimeLive->setOffset((-bpTimeDemo.qpBasePoint + QPoint(0, bpTimeDemo.iAscent)) * fViewScale);
	} else {
		qgpiTimeLive->setPixmap(QPixmap());
	}
	qgpiTimeLive->setItemVisible(s.os.bTime);
}

OverlayConfig::OverlayConfig(Settings &st) :
		ConfigWidget(st),
		qgpiScreen(NULL),
		qgs(),
		qgsFpsPreview(),
		qgpiFpsDemo(NULL),
		oug(NULL),
		qgtiInstructions(NULL),
		fViewScale(1.0f) {

	// grab a desktop screenshot as background
	QRect dsg = QApplication::desktop()->screenGeometry();

#if QT_VERSION > 0x050000
	qpScreen = QGuiApplication::primaryScreen()->grabWindow(QApplication::desktop()->winId());
#else
	qpScreen = QPixmap::grabWindow(QApplication::desktop()->winId(), dsg.x(), dsg.y(), dsg.width(), dsg.height());
#endif
	if (qpScreen.size().isEmpty()) {
		qWarning() << __FUNCTION__ << "failed to grab desktop image, trying desktop widget...";

		qpScreen = QPixmap::grabWidget(QApplication::desktop(), dsg);

		if (qpScreen.size().isEmpty()) {
			qWarning() << __FUNCTION__ << "failed to grab desktop widget image, falling back.";

			QRect desktop_size = QApplication::desktop()->screenGeometry();
			qpScreen = QPixmap(desktop_size.width(), desktop_size.height());
			qpScreen.fill(Qt::darkGreen);
		}
	}

	initDisplay();
}

void OverlayConfig::updateOverlayExclusionModeState() {
}

void OverlayConfig::load(const Settings &r) {
	s.os = r.os;

	// Launchers
	{

		QStringList builtinLaunchers;
#ifdef Q_OS_WIN
		int i = 0;
		while (overlayLaunchers[i]) {
			QString str = QLatin1String(overlayLaunchers[i]);
			builtinLaunchers << str;
			++i;
		}
#endif
	}

	// Whitelist
	{
		QStringList builtinWhitelist;
#ifdef Q_OS_WIN
		int i = 0;
		while (overlayWhitelist[i]) {
			QString str = QLatin1String(overlayWhitelist[i]);
			builtinWhitelist << str;
			++i;
		}
#endif
	}

	// Blacklist
	{
		QStringList builtinBlacklist;
#ifdef Q_OS_WIN
		int i = 0;
		while (overlayBlacklist[i]) {
			QString str = QLatin1String(overlayBlacklist[i]);
			builtinBlacklist << str;
			++i;
		}
#endif
	}

	initDisplay();
	resizeScene(true);
	update();
}

QString OverlayConfig::title() const {
	return tr("Overlay");
}

QIcon OverlayConfig::icon() const {
	return QIcon(QLatin1String("skin:config_osd.png"));
}

void OverlayConfig::save() const {
	g.qs->beginGroup(QLatin1String("overlay"));
	s.os.save();
	g.qs->endGroup();
#ifdef Q_OS_WIN
	// On MS windows force sync so the registry is updated.
	g.qs->sync();
#endif
}

void OverlayConfig::accept() const {
	g.o->forceSettings();
	g.o->setActive(s.os.bEnable);
}

bool OverlayConfig::eventFilter(QObject *obj, QEvent *evt) {
	if (evt->type() == QEvent::Resize)
		QMetaObject::invokeMethod(this, "resizeScene", Qt::QueuedConnection);
	return ConfigWidget::eventFilter(obj, evt);
}

void OverlayConfig::resizeScene(bool force) {
	int ph = qgpiScreen->pixmap().height();
	int pw = qgpiScreen->pixmap().width();

	qgs.setSceneRect(QRectF(0, 0, qgpiScreen->pixmap().width(), qgpiScreen->pixmap().height()));

	fViewScale = static_cast<float>(qgpiScreen->pixmap().height()) / static_cast<float>(qpScreen.height());
	refreshFpsLive();
	refreshTimeLive();

	QFont f = qgtiInstructions->font();
	f.setPointSizeF(qgs.sceneRect().height() / 20.0f);
	qgtiInstructions->setFont(f);

	qgtiInstructions->setPos(qgs.sceneRect().width() / 20.0f, qgs.sceneRect().height() / 10.0f);
	qgtiInstructions->setTextWidth(qgs.sceneRect().width() * 18.0f / 20.0f);

	oug->updateLayout();
	oug->updateUsers();

	qgpiFpsLive->updateRender();
	qgpiTimeLive->updateRender();
}

void OverlayConfig::on_qlwLaunchers_itemSelectionChanged() {
}

void OverlayConfig::on_qcbOverlayExclusionMode_currentIndexChanged(int) {
	updateOverlayExclusionModeState();
}

void OverlayConfig::on_qpbLaunchersAdd_clicked() {
#if defined(Q_OS_WIN)
	QString file = QFileDialog::getOpenFileName(this, tr("Choose executable"), QString(), QLatin1String("*.exe"));
#elif defined(Q_OS_MAC)
	QString file = QFileDialog::getOpenFileName(this, tr("Choose application"), QString(), QLatin1String("*.app"));
#else
	QString file = QString();
#endif
}

void OverlayConfig::on_qpbWhitelistAdd_clicked() {
#if defined(Q_OS_WIN)
	QString file = QFileDialog::getOpenFileName(this, tr("Choose executable"), QString(), QLatin1String("*.exe"));
#elif defined(Q_OS_MAC)
	QString file = QFileDialog::getOpenFileName(this, tr("Choose application"), QString(), QLatin1String("*.app"));
#else
	QString file = QString();
#endif
}

void OverlayConfig::on_qpbFpsFont_clicked() {
	bool ok;
	QFont new_font = QFontDialog::getFont(&ok, s.os.qfFps);

	if (ok) {
		s.os.qfFps = new_font;

		refreshFpsDemo();
		refreshFpsLive();
		refreshTimeLive();
	}
}

void OverlayConfig::on_qpbFpsColor_clicked() {
	QColor color = QColorDialog::getColor(s.os.qcFps);

	if (color.isValid()) {
		s.os.qcFps = color;

		refreshFpsDemo();
		refreshFpsLive();
		refreshTimeLive();
	}
}

void OverlayConfig::on_qpbLoadPreset_clicked() {
	QString fn = QFileDialog::getOpenFileName(this,
	             tr("Load Overlay Presets"),
	             QDir::homePath(),
                 tr("Bubbles overlay presets (*.bubbleslay)"));

	if (fn.isEmpty()) {
		return;
	}

	QSettings qs(fn, QSettings::IniFormat);
	OverlaySettings load_preset;

	qs.beginGroup(QLatin1String("overlay"));
	load_preset.load(&qs);
	qs.endGroup();

#ifdef Q_OS_WIN
	load_preset.qslLaunchers = s.os.qslLaunchers;
	load_preset.qslLaunchersExclude = s.os.qslLaunchersExclude;

	load_preset.qslWhitelist = s.os.qslWhitelist;
	load_preset.qslWhitelistExclude = s.os.qslWhitelistExclude;

	load_preset.qslPaths = s.os.qslPaths;
	load_preset.qslPathsExclude = s.os.qslPathsExclude;

	load_preset.qslBlacklist = s.os.qslBlacklist;
	load_preset.qslBlacklistExclude = s.os.qslBlacklistExclude;
#endif
	load_preset.bEnable = s.os.bEnable;
	s.os = load_preset;

	load(s);
}

void OverlayConfig::on_qpbSavePreset_clicked() {
	QString fn = QFileDialog::getSaveFileName(this,
	             tr("Save Overlay Presets"),
	             QDir::homePath(),
	             tr("Mumble overlay presets (*.mumblelay)"));

	if (fn.isEmpty()) {
		return;
	}

	QSettings qs(fn, QSettings::IniFormat);

	if (!qs.isWritable()) {
		qWarning() << __FUNCTION__ << "preset save file" << fn << "is not writable!";
		return;
	}

	qs.beginGroup(QLatin1String("overlay"));
	s.os.save(&qs);
	qs.remove(QLatin1String("enable"));
	qs.remove(QLatin1String("usewhitelist"));
	qs.remove(QLatin1String("blacklist"));
	qs.remove(QLatin1String("whitelist"));
	qs.remove(QLatin1String("enablelauncherfilter"));
	qs.remove(QLatin1String("launchers"));
	qs.endGroup();
}
