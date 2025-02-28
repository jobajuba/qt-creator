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

#include "publickeydeploymentdialog.h"

#include "sshkeydeployer.h"

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <utils/theme/theme.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {
class PublicKeyDeploymentDialogPrivate
{
public:
    SshKeyDeployer keyDeployer;
    bool done;
};
} // namespace Internal;

using namespace Internal;

PublicKeyDeploymentDialog *PublicKeyDeploymentDialog::createDialog(
        const IDevice::ConstPtr &deviceConfig, QWidget *parent)
{
    const FilePath dir = FilePath::fromString(deviceConfig->sshParameters().privateKeyFile).parentDir();
    const FilePath publicKeyFileName = FileUtils::getOpenFilePath(nullptr,
        tr("Choose Public Key File"), dir,
        tr("Public Key Files (*.pub);;All Files (*)"));
    if (publicKeyFileName.isEmpty())
        return nullptr;
    return new PublicKeyDeploymentDialog(deviceConfig, publicKeyFileName.toString(), parent);
}

PublicKeyDeploymentDialog::PublicKeyDeploymentDialog(const IDevice::ConstPtr &deviceConfig,
        const QString &publicKeyFileName, QWidget *parent)
    : QProgressDialog(parent), d(new PublicKeyDeploymentDialogPrivate)
{
    setAutoReset(false);
    setAutoClose(false);
    setMinimumDuration(0);
    setMaximum(1);

    d->done = false;
    setLabelText(tr("Deploying..."));
    setValue(0);
    connect(this, &PublicKeyDeploymentDialog::canceled,
            this, &PublicKeyDeploymentDialog::handleCanceled);
    connect(&d->keyDeployer, &SshKeyDeployer::error,
            this, &PublicKeyDeploymentDialog::handleDeploymentError);
    connect(&d->keyDeployer, &SshKeyDeployer::finishedSuccessfully,
            this, &PublicKeyDeploymentDialog::handleDeploymentSuccess);
    d->keyDeployer.deployPublicKey(deviceConfig->sshParameters(), publicKeyFileName);
}

PublicKeyDeploymentDialog::~PublicKeyDeploymentDialog()
{
    delete d;
}

void PublicKeyDeploymentDialog::handleDeploymentSuccess()
{
    handleDeploymentFinished(QString());
    setValue(1);
    d->done = true;
}

void PublicKeyDeploymentDialog::handleDeploymentError(const QString &errorMsg)
{
    handleDeploymentFinished(errorMsg);
}

void PublicKeyDeploymentDialog::handleDeploymentFinished(const QString &errorMsg)
{
    QString buttonText;
    QString textColor;
    if (errorMsg.isEmpty()) {
        buttonText = tr("Deployment finished successfully.");
        textColor = Utils::creatorTheme()->color(Utils::Theme::TextColorNormal).name();
    } else {
        buttonText = errorMsg;
        textColor = Utils::creatorTheme()->color(Utils::Theme::TextColorError).name();
    }
    setLabelText(QString::fromLatin1("<font color=\"%1\">%2</font>")
            .arg(textColor)
            .arg(buttonText.replace("\n", "<br/>")));
    setCancelButtonText(tr("Close"));
}

void PublicKeyDeploymentDialog::handleCanceled()
{
    disconnect(&d->keyDeployer, nullptr, this, nullptr);
    d->keyDeployer.stopDeployment();
    if (d->done)
        accept();
    else
        reject();
}

} // namespace RemoteLinux
