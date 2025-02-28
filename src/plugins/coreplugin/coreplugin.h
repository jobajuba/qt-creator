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

#pragma once

#include <qglobal.h>

#include <extensionsystem/iplugin.h>
#include <utils/environment.h>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}

namespace Core {
namespace Internal {

class EditMode;
class MainWindow;
class Locator;

class CorePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Core.json")

public:
    CorePlugin();
    ~CorePlugin() override;

    static CorePlugin *instance();

    bool initialize(const QStringList &arguments, QString *errorMessage = nullptr) override;
    void extensionsInitialized() override;
    bool delayedInitialize() override;
    ShutdownFlag aboutToShutdown() override;
    QObject *remoteCommand(const QStringList & /* options */,
                           const QString &workingDirectory,
                           const QStringList &args) override;

    static Utils::Environment startupSystemEnvironment();
    static Utils::EnvironmentItems environmentChanges();
    static void setEnvironmentChanges(const Utils::EnvironmentItems &changes);
    static QString msgCrashpadInformation();

public slots:
    void fileOpenRequest(const QString&);

#if defined(WITH_TESTS)
private slots:
    void testVcsManager_data();
    void testVcsManager();
    // Locator:
    void test_basefilefilter();
    void test_basefilefilter_data();

    void testOutputFormatter();
#endif

private:
    static void addToPathChooserContextMenu(Utils::PathChooser *pathChooser, QMenu *menu);
    static void setupSystemEnvironment();
    void checkSettings();
    void warnAboutCrashReporing();

    MainWindow *m_mainWindow = nullptr;
    EditMode *m_editMode = nullptr;
    Locator *m_locator = nullptr;
    Utils::Environment m_startupSystemEnvironment;
    Utils::EnvironmentItems m_environmentChanges;
};

} // namespace Internal
} // namespace Core
