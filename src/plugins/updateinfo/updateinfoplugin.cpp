/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "settingspage.h"
#include "updateinfoplugin.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/shellcommand.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDate>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMetaEnum>
#include <QPointer>
#include <QProcessEnvironment>
#include <QTimer>

namespace {
    static const char UpdaterGroup[] = "Updater";
    static const char MaintenanceToolKey[] = "MaintenanceTool";
    static const char AutomaticCheckKey[] = "AutomaticCheck";
    static const char CheckIntervalKey[] = "CheckUpdateInterval";
    static const char LastCheckDateKey[] = "LastCheckDate";
    static const quint32 OneMinute = 60000;
    static const quint32 OneHour = 3600000;
    static const char InstallUpdates[] = "UpdateInfo.InstallUpdates";
}

using namespace Core;

namespace UpdateInfo {
namespace Internal {

class UpdateInfoPluginPrivate
{
public:
    QString m_maintenanceTool;
    QPointer<ShellCommand> m_checkUpdatesCommand;
    QPointer<FutureProgress> m_progress;
    QString m_collectedOutput;
    QTimer *m_checkUpdatesTimer = nullptr;

    struct Settings
    {
        bool automaticCheck = true;
        UpdateInfoPlugin::CheckUpdateInterval checkInterval = UpdateInfoPlugin::WeeklyCheck;
    };
    Settings m_settings;
    QDate m_lastCheckDate;
};

UpdateInfoPlugin::UpdateInfoPlugin()
    : d(new UpdateInfoPluginPrivate)
{
    d->m_checkUpdatesTimer = new QTimer(this);
    d->m_checkUpdatesTimer->setTimerType(Qt::VeryCoarseTimer);
    d->m_checkUpdatesTimer->setInterval(OneHour);
    connect(d->m_checkUpdatesTimer, &QTimer::timeout,
            this, &UpdateInfoPlugin::doAutoCheckForUpdates);
}

UpdateInfoPlugin::~UpdateInfoPlugin()
{
    stopCheckForUpdates();
    if (!d->m_maintenanceTool.isEmpty())
        saveSettings();

    delete d;
}

void UpdateInfoPlugin::startAutoCheckForUpdates()
{
    doAutoCheckForUpdates();

    d->m_checkUpdatesTimer->start();
}

void UpdateInfoPlugin::stopAutoCheckForUpdates()
{
    d->m_checkUpdatesTimer->stop();
}

void UpdateInfoPlugin::doAutoCheckForUpdates()
{
    if (d->m_checkUpdatesCommand)
        return; // update task is still running (might have been run manually just before)

    if (nextCheckDate().isValid() && nextCheckDate() > QDate::currentDate())
        return; // not a time for check yet

    startCheckForUpdates();
}

void UpdateInfoPlugin::startCheckForUpdates()
{
    stopCheckForUpdates();

    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.set("QT_LOGGING_RULES", "*=false");
    d->m_checkUpdatesCommand = new ShellCommand({}, env);
    d->m_checkUpdatesCommand->setDisplayName(tr("Checking for Updates"));
    connect(d->m_checkUpdatesCommand, &ShellCommand::stdOutText, this, &UpdateInfoPlugin::collectCheckForUpdatesOutput);
    connect(d->m_checkUpdatesCommand, &ShellCommand::finished, this, &UpdateInfoPlugin::checkForUpdatesFinished);
    d->m_checkUpdatesCommand->addJob({Utils::FilePath::fromString(d->m_maintenanceTool), {"--checkupdates"}},
                                     60 * 3, // 3 minutes timeout
                                     /*workingDirectory=*/{},
                                     [](int /*exitCode*/) { return Utils::QtcProcess::FinishedWithSuccess; });
    d->m_checkUpdatesCommand->execute();
    d->m_progress = d->m_checkUpdatesCommand->futureProgress();
    if (d->m_progress) {
        d->m_progress->setKeepOnFinish(FutureProgress::KeepOnFinishTillUserInteraction);
        d->m_progress->setSubtitleVisibleInStatusBar(true);
    }
    emit checkForUpdatesRunningChanged(true);
}

void UpdateInfoPlugin::stopCheckForUpdates()
{
    if (!d->m_checkUpdatesCommand)
        return;

    d->m_collectedOutput.clear();
    d->m_checkUpdatesCommand->disconnect();
    d->m_checkUpdatesCommand->cancel();
    d->m_checkUpdatesCommand = nullptr;
    emit checkForUpdatesRunningChanged(false);
}

void UpdateInfoPlugin::collectCheckForUpdatesOutput(const QString &contents)
{
    d->m_collectedOutput += contents;
}

struct Update
{
    QString name;
    QString version;
};

static QList<Update> availableUpdates(const QDomDocument &document)
{
    if (document.isNull() || !document.firstChildElement().hasChildNodes())
        return {};
    QList<Update> result;
    const QDomNodeList updates = document.firstChildElement().elementsByTagName("update");
    for (int i = 0; i < updates.size(); ++i) {
        const QDomNode node = updates.item(i);
        if (node.isElement()) {
            const QDomElement element = node.toElement();
            if (element.hasAttribute("name"))
                result.append({element.attribute("name"), element.attribute("version")});
        }
    }
    return result;
}

void UpdateInfoPlugin::checkForUpdatesFinished()
{
    setLastCheckDate(QDate::currentDate());

    QDomDocument document;
    document.setContent(d->m_collectedOutput);

    stopCheckForUpdates();

    if (!document.isNull() && document.firstChildElement().hasChildNodes()) {
        // progress details are shown until user interaction for the "no updates" case,
        // so we can show the "No updates found" text, but if we have updates we don't
        // want to keep it around
        if (d->m_progress)
            d->m_progress->setKeepOnFinish(FutureProgress::HideOnFinish);
        emit newUpdatesAvailable(true);
        Utils::InfoBarEntry info(InstallUpdates, tr("New updates are available. Start the update?"));
        info.setCustomButtonInfo(tr("Start Update"), [this] {
            Core::ICore::infoBar()->removeInfo(InstallUpdates);
            startUpdater();
        });
        const QList<Update> updates = availableUpdates(document);
        info.setDetailsWidgetCreator([updates]() -> QWidget * {
            const QString updateText = Utils::transform(updates, [](const Update &u) {
                                           return u.version.isEmpty()
                                                      ? u.name
                                                      : tr("%1 (%2)", "Package name and version")
                                                            .arg(u.name, u.version);
                                       }).join("</li><li>");
            auto label = new QLabel;
            label->setText("<qt><p>" + tr("Available updates:") + "<ul><li>" + updateText
                           + "</li></ul></p></qt>");
            label->setContentsMargins(0, 0, 0, 8);
            return label;
        });
        Core::ICore::infoBar()->removeInfo(InstallUpdates); // remove any existing notifications
        Core::ICore::infoBar()->unsuppressInfo(InstallUpdates);
        Core::ICore::infoBar()->addInfo(info);
    } else {
        emit newUpdatesAvailable(false);
        if (d->m_progress)
            d->m_progress->setSubtitle(tr("No updates found."));
    }
}

bool UpdateInfoPlugin::isCheckForUpdatesRunning() const
{
    return d->m_checkUpdatesCommand;
}

void UpdateInfoPlugin::extensionsInitialized()
{
    if (isAutomaticCheck())
        QTimer::singleShot(OneMinute, this, &UpdateInfoPlugin::startAutoCheckForUpdates);
}

bool UpdateInfoPlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    loadSettings();

    if (d->m_maintenanceTool.isEmpty()) {
        *errorMessage = tr("Could not determine location of maintenance tool. Please check "
            "your installation if you did not enable this plugin manually.");
        return false;
    }

    if (!QFileInfo(d->m_maintenanceTool).isExecutable()) {
        *errorMessage = tr("The maintenance tool at \"%1\" is not an executable. Check your installation.")
            .arg(d->m_maintenanceTool);
        d->m_maintenanceTool.clear();
        return false;
    }

    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &UpdateInfoPlugin::saveSettings);

    (void) new SettingsPage(this);

    QAction *checkForUpdatesAction = new QAction(tr("Check for Updates"), this);
    checkForUpdatesAction->setMenuRole(QAction::ApplicationSpecificRole);
    Core::Command *checkForUpdatesCommand = Core::ActionManager::registerAction(checkForUpdatesAction, "Updates.CheckForUpdates");
    connect(checkForUpdatesAction, &QAction::triggered, this, &UpdateInfoPlugin::startCheckForUpdates);
    ActionContainer *const helpContainer = ActionManager::actionContainer(Core::Constants::M_HELP);
    helpContainer->addAction(checkForUpdatesCommand, Constants::G_HELP_UPDATES);

    return true;
}

void UpdateInfoPlugin::loadSettings() const
{
    UpdateInfoPluginPrivate::Settings def;
    QSettings *settings = ICore::settings();
    const QString updaterKey = QLatin1String(UpdaterGroup) + '/';
    d->m_maintenanceTool = settings->value(updaterKey + MaintenanceToolKey).toString();
    d->m_lastCheckDate = settings->value(updaterKey + LastCheckDateKey, QDate()).toDate();
    d->m_settings.automaticCheck
        = settings->value(updaterKey + AutomaticCheckKey, def.automaticCheck).toBool();
    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator(mo->indexOfEnumerator(CheckIntervalKey));
    if (QTC_GUARD(me.isValid())) {
        const QString checkInterval = settings
                                          ->value(updaterKey + CheckIntervalKey,
                                                  me.valueToKey(def.checkInterval))
                                          .toString();
        bool ok = false;
        const int newValue = me.keyToValue(checkInterval.toUtf8(), &ok);
        if (ok)
            d->m_settings.checkInterval = static_cast<CheckUpdateInterval>(newValue);
    }
}

void UpdateInfoPlugin::saveSettings()
{
    UpdateInfoPluginPrivate::Settings def;
    Utils::QtcSettings *settings = ICore::settings();
    settings->beginGroup(UpdaterGroup);
    settings->setValueWithDefault(LastCheckDateKey, d->m_lastCheckDate, QDate());
    settings->setValueWithDefault(AutomaticCheckKey,
                                  d->m_settings.automaticCheck,
                                  def.automaticCheck);
    // Note: don't save MaintenanceToolKey on purpose! This setting may be set only by installer.
    // If creator is run not from installed SDK, the setting can be manually created here:
    // [CREATOR_INSTALLATION_LOCATION]/share/qtcreator/QtProject/QtCreator.ini or
    // [CREATOR_INSTALLATION_LOCATION]/Qt Creator.app/Contents/Resources/QtProject/QtCreator.ini on OS X
    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator(mo->indexOfEnumerator(CheckIntervalKey));
    settings->setValueWithDefault(CheckIntervalKey,
                                  QString::fromUtf8(me.valueToKey(d->m_settings.checkInterval)),
                                  QString::fromUtf8(me.valueToKey(def.checkInterval)));
    settings->endGroup();
}

bool UpdateInfoPlugin::isAutomaticCheck() const
{
    return d->m_settings.automaticCheck;
}

void UpdateInfoPlugin::setAutomaticCheck(bool on)
{
    if (d->m_settings.automaticCheck == on)
        return;

    d->m_settings.automaticCheck = on;
    if (on)
        startAutoCheckForUpdates();
    else
        stopAutoCheckForUpdates();
}

UpdateInfoPlugin::CheckUpdateInterval UpdateInfoPlugin::checkUpdateInterval() const
{
    return d->m_settings.checkInterval;
}

void UpdateInfoPlugin::setCheckUpdateInterval(UpdateInfoPlugin::CheckUpdateInterval interval)
{
    if (d->m_settings.checkInterval == interval)
        return;

    d->m_settings.checkInterval = interval;
}

QDate UpdateInfoPlugin::lastCheckDate() const
{
    return d->m_lastCheckDate;
}

void UpdateInfoPlugin::setLastCheckDate(const QDate &date)
{
    if (d->m_lastCheckDate == date)
        return;

    d->m_lastCheckDate = date;
    emit lastCheckDateChanged(date);
}

QDate UpdateInfoPlugin::nextCheckDate() const
{
    return nextCheckDate(d->m_settings.checkInterval);
}

QDate UpdateInfoPlugin::nextCheckDate(CheckUpdateInterval interval) const
{
    if (!d->m_lastCheckDate.isValid())
        return QDate();

    if (interval == DailyCheck)
        return d->m_lastCheckDate.addDays(1);
    if (interval == WeeklyCheck)
        return d->m_lastCheckDate.addDays(7);
    return d->m_lastCheckDate.addMonths(1);
}

void UpdateInfoPlugin::startUpdater()
{
    Utils::QtcProcess::startDetached({Utils::FilePath::fromString(d->m_maintenanceTool), {"--updater"}});
}

} //namespace Internal
} //namespace UpdateInfo
