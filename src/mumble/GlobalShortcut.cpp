// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "GlobalShortcut.h"

#include "AudioInput.h"
#include "ClientUser.h"
#include "Channel.h"
#include "Database.h"
#include "MainWindow.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

/**
 * Used to save the global, unique, platform specific GlobalShortcutEngine.
 */
GlobalShortcutEngine *GlobalShortcutEngine::engine = NULL;

static ConfigWidget *GlobalShortcutConfigDialogNew(Settings &st) {
	return new GlobalShortcutConfig(st);
}

static ConfigRegistrar registrar(1200, GlobalShortcutConfigDialogNew);

static const QString UPARROW = QString::fromUtf8("\xE2\x86\x91 ");

ShortcutKeyWidget::ShortcutKeyWidget(QWidget *p) : QLineEdit(p) {
	setReadOnly(true);
	clearFocus();
	bModified = false;
	displayKeys();
}

QList<QVariant> ShortcutKeyWidget::getShortcut() const {
	return qlButtons;
}

void ShortcutKeyWidget::setShortcut(const QList<QVariant> &buttons) {
	qlButtons = buttons;
	displayKeys();
}

void ShortcutKeyWidget::focusInEvent(QFocusEvent *) {
	setText(tr("Press Shortcut"));

	QPalette pal=parentWidget()->palette();
	pal.setColor(QPalette::Base, pal.color(QPalette::Base).dark(120));
	setPalette(pal);

	setForegroundRole(QPalette::Button);
	GlobalShortcutEngine::engine->resetMap();
	connect(GlobalShortcutEngine::engine, SIGNAL(buttonPressed(bool)), this, SLOT(updateKeys(bool)));
	installEventFilter(this);
}

void ShortcutKeyWidget::focusOutEvent(QFocusEvent *e) {
	if ((e->reason() == Qt::TabFocusReason) || (e->reason() == Qt::BacktabFocusReason))
		return;

	setPalette(parentWidget()->palette());
	clearFocus();
	disconnect(GlobalShortcutEngine::engine, SIGNAL(buttonPressed(bool)), this, SLOT(updateKeys(bool)));
	displayKeys();
	removeEventFilter(this);
}

bool ShortcutKeyWidget::eventFilter(QObject *, QEvent *evt) {
	if ((evt->type() == QEvent::KeyPress) || (evt->type() == QEvent::MouseButtonPress))
		return true;
	return false;
}

void ShortcutKeyWidget::mouseDoubleClickEvent(QMouseEvent *) {
	bModified = true;
	qlButtons.clear();
	clearFocus();
	displayKeys();
}

void ShortcutKeyWidget::updateKeys(bool last) {
	qlButtons = GlobalShortcutEngine::engine->qlActiveButtons;
	bModified = true;

	if (qlButtons.isEmpty())
		return;

	if (last)
		clearFocus();
	else
		displayKeys(false);
}

void ShortcutKeyWidget::displayKeys(bool last) {
	QStringList keys;

	foreach(QVariant button, qlButtons) {
		QString id = GlobalShortcutEngine::engine->buttonName(button);
		if (! id.isEmpty())
			keys << id;
	}
	setText(keys.join(QLatin1String(" + ")));
	emit keySet(keys.count() > 0, last);
}

ShortcutActionWidget::ShortcutActionWidget(QWidget *p) : MUComboBox(p) {
	int idx = 0;

	insertItem(idx, tr("Unassigned"));
	setItemData(idx, -1);
#ifndef Q_OS_MAC
	setSizeAdjustPolicy(AdjustToContents);
#endif

	idx++;

	foreach(GlobalShortcut *gs, GlobalShortcutEngine::engine->qmShortcuts) {
		insertItem(idx, gs->name);
		setItemData(idx, gs->idx);
		if (! gs->qsToolTip.isEmpty())
			setItemData(idx, gs->qsToolTip, Qt::ToolTipRole);
		if (! gs->qsWhatsThis.isEmpty())
			setItemData(idx, gs->qsWhatsThis, Qt::WhatsThisRole);
		idx++;
	}
}

void ShortcutActionWidget::setIndex(int idx) {
	setCurrentIndex(findData(idx));
}

unsigned int ShortcutActionWidget::index() const {
	return itemData(currentIndex()).toUInt();
}

ShortcutToggleWidget::ShortcutToggleWidget(QWidget *p) : MUComboBox(p) {
	int idx = 0;

	insertItem(idx, tr("Off"));
	setItemData(idx, -1);
	idx++;

	insertItem(idx, tr("Toggle"));
	setItemData(idx, 0);
	idx++;

	insertItem(idx, tr("On"));
	setItemData(idx, 1);
	idx++;
}

void ShortcutToggleWidget::setIndex(int idx) {
	setCurrentIndex(findData(idx));
}

int ShortcutToggleWidget::index() const {
	return itemData(currentIndex()).toInt();
}

void iterateChannelChildren(QTreeWidgetItem *root, Channel *chan, QMap<int, QTreeWidgetItem *> &map) {
	foreach(Channel *c, chan->qlChannels) {
		QTreeWidgetItem *sub = new QTreeWidgetItem(root, QStringList(c->qsName));
		sub->setData(0, Qt::UserRole, c->iId);
		map.insert(c->iId, sub);
		iterateChannelChildren(sub, c, map);
	}
}

ShortcutTargetDialog::ShortcutTargetDialog(const ShortcutTarget &st, QWidget *pw) : QDialog(pw) {
	stTarget = st;

	// If we are connected to a server also add all connected players with certificates to the list
	if (g.uiSession) {
		QMap<QString, QString> others;
		QMap<QString, QString>::const_iterator i;

		QReadLocker lock(& ClientUser::c_qrwlUsers);
		foreach(ClientUser *p, ClientUser::c_qmUsers) {
			if ((p->uiSession != g.uiSession) && p->qsFriendName.isEmpty() && ! p->qsHash.isEmpty()) {
				others.insert(p->qsName, p->qsHash);
				qmHashNames.insert(p->qsHash, p->qsName);
			}
		}
	}

	QMap<QString, QString> users;

	foreach(const QString &hash, st.qlUsers) {
		if (qmHashNames.contains(hash))
			users.insert(qmHashNames.value(hash), hash);
		else
			users.insert(QString::fromLatin1("#%1").arg(hash), hash);
	}

	{
		QMap<QString, QString>::const_iterator i;
		for (i=users.constBegin(); i != users.constEnd(); ++i) {
			QListWidgetItem *itm = new QListWidgetItem(i.key());
			itm->setData(Qt::UserRole, i.value());
		}
	}
}

ShortcutTarget ShortcutTargetDialog::target() const {
	return stTarget;
}

void ShortcutTargetDialog::accept() {
	QDialog::accept();
}

void ShortcutTargetDialog::on_qrbUsers_clicked() {
	stTarget.bUsers = true;
}

void ShortcutTargetDialog::on_qrbChannel_clicked() {
	stTarget.bUsers = false;
}

void ShortcutTargetDialog::on_qpbAdd_clicked() {
}

void ShortcutTargetDialog::on_qpbRemove_clicked() {
}

ShortcutTargetWidget::ShortcutTargetWidget(QWidget *p) : QFrame(p) {
	qleTarget = new QLineEdit();
	qleTarget->setReadOnly(true);

	qtbEdit = new QToolButton();
	qtbEdit->setText(tr("..."));
	qtbEdit->setFocusPolicy(Qt::ClickFocus);
	qtbEdit->setObjectName(QLatin1String("qtbEdit"));

	QHBoxLayout *l = new QHBoxLayout(this);
	l->setContentsMargins(0,0,0,0);
	l->addWidget(qleTarget, 1);
	l->addWidget(qtbEdit);

	QMetaObject::connectSlotsByName(this);
}

/**
 * This function returns a textual representation of the given shortcut target st.
 */
QString ShortcutTargetWidget::targetString(const ShortcutTarget &st) {
	if (st.bUsers) {
		if (! st.qlUsers.isEmpty()) {
			QMap<QString, QString> hashes;

			QReadLocker lock(& ClientUser::c_qrwlUsers);
			foreach(ClientUser *p, ClientUser::c_qmUsers) {
				if (! p->qsHash.isEmpty()) {
					hashes.insert(p->qsHash, p->qsName);
				}
			}

			QStringList users;
			foreach(const QString &hash, st.qlUsers) {
				QString name;
				if (hashes.contains(hash)) {
					name = hashes.value(hash);
				} else {
					name = g.db->getFriend(hash);
					if (name.isEmpty())
						name = QString::fromLatin1("#%1").arg(hash);
				}
				users << name;
			}

			users.sort();
			return users.join(tr(", "));
		}
	} else {
		if (st.iChannel < 0) {
			switch (st.iChannel) {
				case SHORTCUT_TARGET_ROOT:
					return tr("Root");
				case SHORTCUT_TARGET_PARENT:
					return tr("Parent");
				case SHORTCUT_TARGET_CURRENT:
					return tr("Current");
				default:
					if(st.iChannel <= SHORTCUT_TARGET_PARENT_SUBCHANNEL)
						return (UPARROW + tr("Subchannel #%1").arg(SHORTCUT_TARGET_PARENT_SUBCHANNEL + 1 - st.iChannel));
					else
						return tr("Subchannel #%1").arg(SHORTCUT_TARGET_CURRENT - st.iChannel);
			}
		} else {
			Channel *c = Channel::get(st.iChannel);
			if (c)
				return c->qsName;
			else
				return tr("Invalid");
		}
	}
	return tr("Empty");
}

ShortcutTarget ShortcutTargetWidget::target() const {
	return stTarget;
}

void ShortcutTargetWidget::setTarget(const ShortcutTarget &st) {
	stTarget = st;
	qleTarget->setText(ShortcutTargetWidget::targetString(st));
}

void ShortcutTargetWidget::on_qtbEdit_clicked() {
	ShortcutTargetDialog *std = new ShortcutTargetDialog(stTarget, this);
	if (std->exec() == QDialog::Accepted) {
		stTarget = std->target();
		qleTarget->setText(ShortcutTargetWidget::targetString(stTarget));

		// Qt bug? Who knows, but since there won't be focusOut events for this widget anymore,
		// we need to force the commit.
		QWidget *p = parentWidget();
		while (p) {
			if (QAbstractItemView *qaiv = qobject_cast<QAbstractItemView *>(p)) {
				QStyledItemDelegate *qsid = qobject_cast<QStyledItemDelegate *>(qaiv->itemDelegate());
				if (qsid) {
					QMetaObject::invokeMethod(qsid, "_q_commitDataAndCloseEditor",
					                          Qt::QueuedConnection, Q_ARG(QWidget*, this));
				}
				break;
			}
			p = p->parentWidget();
		}
	}
	delete std;
}

ShortcutDelegate::ShortcutDelegate(QObject *p) : QStyledItemDelegate(p) {
	QItemEditorFactory *factory = new QItemEditorFactory;

	factory->registerEditor(QVariant::List, new QStandardItemEditorCreator<ShortcutKeyWidget>());
	factory->registerEditor(QVariant::UInt, new QStandardItemEditorCreator<ShortcutActionWidget>());
	factory->registerEditor(QVariant::Int, new QStandardItemEditorCreator<ShortcutToggleWidget>());
	factory->registerEditor(static_cast<QVariant::Type>(QVariant::fromValue(ShortcutTarget()).userType()), new QStandardItemEditorCreator<ShortcutTargetWidget>());
	factory->registerEditor(QVariant::String, new QStandardItemEditorCreator<QLineEdit>());
	factory->registerEditor(QVariant::Invalid, new QStandardItemEditorCreator<QWidget>());
	setItemEditorFactory(factory);
}

ShortcutDelegate::~ShortcutDelegate() {
	delete itemEditorFactory();
	setItemEditorFactory(NULL);
}

/**
 * Provides textual representations for the mappings done for the edit behaviour.
 */
QString ShortcutDelegate::displayText(const QVariant &item, const QLocale &loc) const {
	if (item.type() == QVariant::List) {
		return GlobalShortcutEngine::buttonText(item.toList());
	} else if (item.type() == QVariant::Int) {
		int v = item.toInt();
		if (v > 0)
			return tr("On");
		else if (v < 0)
			return tr("Off");
		else
			return tr("Toggle");
	} else if (item.type() == QVariant::UInt) {
		GlobalShortcut *gs = GlobalShortcutEngine::engine->qmShortcuts.value(item.toInt());
		if (gs)
			return gs->name;
		else
			return tr("Unassigned");
	} else if (item.userType() == QVariant::fromValue(ShortcutTarget()).userType()) {
		return ShortcutTargetWidget::targetString(item.value<ShortcutTarget>());
	}

	qWarning("ShortcutDelegate::displayText Unknown type %d", item.type());

	return QStyledItemDelegate::displayText(item,loc);
}

GlobalShortcutConfig::GlobalShortcutConfig(Settings &st) : ConfigWidget(st) {
	installEventFilter(this);

	bool canSuppress = GlobalShortcutEngine::engine->canSuppress();
	bool canDisable = GlobalShortcutEngine::engine->canDisable();
}

bool GlobalShortcutConfig::eventFilter(QObject* /*object*/, QEvent *e) {
#ifdef Q_OS_MAC
	if (e->type() == QEvent::WindowActivate) {
		if (! g.s.bSuppressMacEventTapWarning) {
			qwWarningContainer->setVisible(showWarning());
		}
	}
#else
	Q_UNUSED(e)
#endif
	return false;
}

bool GlobalShortcutConfig::showWarning() const {
#ifdef Q_OS_MAC
# if MAC_OS_X_VERSION_MAX_ALLOWED >= 1090
	if (QSysInfo::MacintoshVersion >= QSysInfo::MV_MAVERICKS) {
		return !AXIsProcessTrustedWithOptions(NULL);
	} else
# endif
	{
		return !QFile::exists(QLatin1String("/private/var/db/.AccessibilityAPIEnabled"));
	}
#endif
	return false;
}

void GlobalShortcutConfig::on_qpbOpenAccessibilityPrefs_clicked() {
	QStringList args;
	args << QLatin1String("/Applications/System Preferences.app");
	args << QLatin1String("/System/Library/PreferencePanes/UniversalAccessPref.prefPane");
	(void) QProcess::startDetached(QLatin1String("/usr/bin/open"), args);
}

void GlobalShortcutConfig::on_qpbSkipWarning_clicked() {
	// Store to both global and local settings.  The 'Skip' is live, as in
	// we don't expect the user to click Apply for their choice to work.
	g.s.bSuppressMacEventTapWarning = s.bSuppressMacEventTapWarning = true;
}

void GlobalShortcutConfig::commit() {
}

void GlobalShortcutConfig::on_qcbEnableGlobalShortcuts_stateChanged(int state) {
	bool b = state == Qt::Checked;

	// We have to enable this here. Otherwise, adding new shortcuts wouldn't work.
	GlobalShortcutEngine::engine->setEnabled(b);
}

void GlobalShortcutConfig::on_qpbAdd_clicked(bool) {
	commit();
	Shortcut sc;
	sc.iIndex = -1;
	sc.bSuppress = false;
	qlShortcuts << sc;
	reload();
}

void GlobalShortcutConfig::on_qpbRemove_clicked(bool) {
	commit();
}

void GlobalShortcutConfig::on_qtwShortcuts_currentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *) {
}

void GlobalShortcutConfig::on_qtwShortcuts_itemChanged(QTreeWidgetItem *item, int) {
}

QString GlobalShortcutConfig::title() const {
	return tr("Shortcuts");
}

QIcon GlobalShortcutConfig::icon() const {
	return QIcon(QLatin1String("skin:config_shortcuts.png"));
}

void GlobalShortcutConfig::load(const Settings &r) {
	qlShortcuts = r.qlShortcuts;

	// The 'Skip' button is supposed to be live, meaning users do not need to click Apply for
	// their choice of skipping to apply.
	//
	// To make this work well, we set the setting on load. This is to make 'Reset' and 'Restore Defaults'
	// work as expected.
	g.s.bSuppressMacEventTapWarning = s.bSuppressMacEventTapWarning = r.bSuppressMacEventTapWarning;
	reload();
}

void GlobalShortcutConfig::save() const {
}

QTreeWidgetItem *GlobalShortcutConfig::itemForShortcut(const Shortcut &sc) const {
	QTreeWidgetItem *item = new QTreeWidgetItem();
	::GlobalShortcut *gs = GlobalShortcutEngine::engine->qmShortcuts.value(sc.iIndex);

	item->setData(0, Qt::DisplayRole, static_cast<unsigned int>(sc.iIndex));
	if (sc.qvData.isValid() && gs && (sc.qvData.type() == gs->qvDefault.type()))
		item->setData(1, Qt::DisplayRole, sc.qvData);
	else if (gs)
		item->setData(1, Qt::DisplayRole, gs->qvDefault);
	item->setData(2, Qt::DisplayRole, sc.qlButtons);
	item->setCheckState(3, sc.bSuppress ? Qt::Checked : Qt::Unchecked);
	item->setFlags(item->flags() | Qt::ItemIsEditable);


	if (gs) {
		if (! gs->qsToolTip.isEmpty())
			item->setData(0, Qt::ToolTipRole, gs->qsToolTip);
		if (! gs->qsWhatsThis.isEmpty())
			item->setData(0, Qt::WhatsThisRole, gs->qsWhatsThis);
	}

	item->setData(2, Qt::ToolTipRole, tr("Shortcut button combination."));
	item->setData(2, Qt::WhatsThisRole, tr("<b>This is the global shortcut key combination.</b><br />"
	                                       "Click this field and then press the desired key/button combo "
	                                       "to rebind. Double-click to clear."));

	item->setData(3, Qt::ToolTipRole, tr("Suppress keys from other applications"));
	item->setData(3, Qt::WhatsThisRole, tr("<b>This hides the button presses from other applications.</b><br />"
	                                       "Enabling this will hide the button (or the last button of a multi-button combo) "
	                                       "from other applications. Note that not all buttons can be suppressed."));

	return item;
}

void GlobalShortcutConfig::reload() {
}

void GlobalShortcutConfig::accept() const {
	GlobalShortcutEngine::engine->bNeedRemap = true;
	GlobalShortcutEngine::engine->needRemap();
	GlobalShortcutEngine::engine->setEnabled(g.s.bShortcutEnable);
}


GlobalShortcutEngine::GlobalShortcutEngine(QObject *p) : QThread(p) {
	bNeedRemap = true;
	needRemap();
}

GlobalShortcutEngine::~GlobalShortcutEngine() {
	QSet<ShortcutKey *> qs;
	foreach(const QList<ShortcutKey*> &ql, qlShortcutList)
		qs += ql.toSet();

	foreach(ShortcutKey *sk, qs)
		delete sk;
}

void GlobalShortcutEngine::remap() {
	bNeedRemap = false;

	QSet<ShortcutKey *> qs;
	foreach(const QList<ShortcutKey*> &ql, qlShortcutList)
		qs += ql.toSet();

	foreach(ShortcutKey *sk, qs)
		delete sk;

	qlButtonList.clear();
	qlShortcutList.clear();
	qlDownButtons.clear();

	foreach(const Shortcut &sc, g.s.qlShortcuts) {
		GlobalShortcut *gs = qmShortcuts.value(sc.iIndex);
		if (gs && ! sc.qlButtons.isEmpty()) {
			ShortcutKey *sk = new ShortcutKey;
			sk->s = sc;
			sk->iNumUp = sc.qlButtons.count();
			sk->gs = gs;

			foreach(const QVariant &button, sc.qlButtons) {
				int idx = qlButtonList.indexOf(button);
				if (idx == -1) {
					qlButtonList << button;
					qlShortcutList << QList<ShortcutKey *>();
					idx = qlButtonList.count() - 1;
				}
				qlShortcutList[idx] << sk;
			}
		}
	}
}

void GlobalShortcutEngine::run() {
}

bool GlobalShortcutEngine::canSuppress() {
	return false;
}

void GlobalShortcutEngine::setEnabled(bool) {
}

bool GlobalShortcutEngine::enabled() {
	return true;
}

bool GlobalShortcutEngine::canDisable() {
	return false;
}

void GlobalShortcutEngine::resetMap() {
	tReset.restart();
	qlActiveButtons.clear();
}

void GlobalShortcutEngine::needRemap() {
}

/**
 * This function gets called internally to update the state
 * of a button.
 *
 * @return True if button is suppressed, otherwise false
*/
bool GlobalShortcutEngine::handleButton(const QVariant &button, bool down) {
	bool already = qlDownButtons.contains(button);
	if (already == down)
		return qlSuppressed.contains(button);
	if (down)
		qlDownButtons << button;
	else
		qlDownButtons.removeAll(button);

	if (tReset.elapsed() > 100000) {
		if (down) {
			qlActiveButtons.removeAll(button);
			qlActiveButtons << button;
		}
		emit buttonPressed(! down);
	}

	if (down) {
		AudioInputPtr ai = g.ai;
		if (ai.get()) {
			// XXX: This is a data race: we write to ai->activityState
			// (accessed by the AudioInput thread) from the main thread.
			if (ai->activityState == AudioInput::ActivityStateIdle) {
				ai->activityState = AudioInput::ActivityStateReturnedFromIdle;
			}
			ai->tIdle.restart();
		}
	}

	int idx = qlButtonList.indexOf(button);
	if (idx == -1)
		return false;

	bool suppress = false;

	foreach(ShortcutKey *sk, qlShortcutList.at(idx)) {
		if (down) {
			sk->iNumUp--;
			if (sk->iNumUp == 0) {
				GlobalShortcut *gs = sk->gs;
				if (sk->s.bSuppress) {
					suppress = true;
					qlSuppressed << button;
				}
				if (! gs->qlActive.contains(sk->s.qvData)) {
					gs->qlActive << sk->s.qvData;
					emit gs->triggered(true, sk->s.qvData);
					emit gs->down(sk->s.qvData);
				}
			} else if (sk->iNumUp < 0) {
				sk->iNumUp = 0;
			}
		} else {
			if (qlSuppressed.contains(button)) {
				suppress = true;
				qlSuppressed.removeAll(button);
			}
			sk->iNumUp++;
			if (sk->iNumUp == 1) {
				GlobalShortcut *gs = sk->gs;
				if (gs->qlActive.contains(sk->s.qvData)) {
					gs->qlActive.removeAll(sk->s.qvData);
					emit gs->triggered(false, sk->s.qvData);
				}
			} else if (sk->iNumUp > sk->s.qlButtons.count()) {
				sk->iNumUp = sk->s.qlButtons.count();
			}
		}
	}
	return suppress;
}

void GlobalShortcutEngine::add(GlobalShortcut *gs) {
	if (! GlobalShortcutEngine::engine) {
		GlobalShortcutEngine::engine = GlobalShortcutEngine::platformInit();
		GlobalShortcutEngine::engine->setEnabled(g.s.bShortcutEnable);
	}

	GlobalShortcutEngine::engine->qmShortcuts.insert(gs->idx, gs);
	GlobalShortcutEngine::engine->bNeedRemap = true;
	GlobalShortcutEngine::engine->needRemap();
}

void GlobalShortcutEngine::remove(GlobalShortcut *gs) {
	engine->qmShortcuts.remove(gs->idx);
	engine->bNeedRemap = true;
	engine->needRemap();
	if (engine->qmShortcuts.isEmpty()) {
		delete engine;
		GlobalShortcutEngine::engine = NULL;
	}
}

QString GlobalShortcutEngine::buttonText(const QList<QVariant> &list) {
	QStringList keys;

	foreach(QVariant button, list) {
		QString id = GlobalShortcutEngine::engine->buttonName(button);
		if (! id.isEmpty())
			keys << id;
	}
	return keys.join(QLatin1String(" + "));
}

GlobalShortcut::GlobalShortcut(QObject *p, int index, QString qsName, QVariant def) : QObject(p) {
	idx = index;
	name=qsName;
	qvDefault = def;
	GlobalShortcutEngine::add(this);
}

GlobalShortcut::~GlobalShortcut() {
	GlobalShortcutEngine::remove(this);
}
