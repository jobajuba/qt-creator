/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangautomationutils.h"

#include "../clangcompletionassistinterface.h"
#include "../clangcompletionassistprovider.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QScopedPointer>

namespace ClangCodeModel {
namespace Internal {

class WaitForAsyncCompletions
{
public:
    enum WaitResult { GotResults, GotInvalidResults, Timeout };

    WaitResult wait(TextEditor::IAssistProcessor *processor,
                    TextEditor::AssistInterface *assistInterface,
                    int timeoutInMs)
    {
        QTC_ASSERT(processor, return Timeout);
        QTC_ASSERT(assistInterface, return Timeout);

        bool gotResults = false;

        processor->setAsyncCompletionAvailableHandler(
                    [this, processor, &gotResults] (TextEditor::IAssistProposal *proposal) {
            QTC_ASSERT(proposal, return);
            QTC_CHECK(!processor->running());
            proposalModel = proposal->model();
            delete proposal;
            gotResults = true;
        });

        // Are there any immediate results?
        if (TextEditor::IAssistProposal *proposal = processor->perform(assistInterface)) {
            proposalModel = proposal->model();
            delete proposal;
            QTC_ASSERT(proposalModel, return GotInvalidResults);
            return GotResults;
        }

        // There are not any, so wait for async results.
        QElapsedTimer timer;
        timer.start();
        while (!gotResults) {
            if (timer.elapsed() >= timeoutInMs) {
                processor->cancel();
                return Timeout;
            }
            QCoreApplication::processEvents();
        }

        return proposalModel ? GotResults : GotInvalidResults;
    }

public:
    TextEditor::ProposalModelPtr proposalModel;
};

TextEditor::ProposalModelPtr completionResults(TextEditor::BaseTextEditor *textEditor,
                                               const QStringList &includePaths,
                                               int timeOutInMs)
{
    using namespace TextEditor;

    auto textEditorWidget = TextEditorWidget::fromEditor(textEditor);
    QTC_ASSERT(textEditorWidget, return TextEditor::ProposalModelPtr());
    AssistInterface *assistInterface = textEditorWidget->createAssistInterface(
                TextEditor::Completion, TextEditor::ExplicitlyInvoked);
    QTC_ASSERT(assistInterface, return TextEditor::ProposalModelPtr());
    if (!includePaths.isEmpty()) {
        auto clangAssistInterface = static_cast<ClangCompletionAssistInterface *>(assistInterface);
        clangAssistInterface->setHeaderPaths(ProjectExplorer::toUserHeaderPaths(includePaths));
    }

    CompletionAssistProvider *assistProvider
            = textEditor->textDocument()->completionAssistProvider();
    QTC_ASSERT(qobject_cast<ClangCompletionAssistProvider *>(assistProvider),
               return TextEditor::ProposalModelPtr());
    QTC_ASSERT(assistProvider, return TextEditor::ProposalModelPtr());
    QTC_ASSERT(assistProvider->runType() == IAssistProvider::Asynchronous,
               return TextEditor::ProposalModelPtr());

    QScopedPointer<IAssistProcessor> processor(assistProvider->createProcessor());
    QTC_ASSERT(processor, return TextEditor::ProposalModelPtr());

    WaitForAsyncCompletions waitForCompletions;
    const WaitForAsyncCompletions::WaitResult result = waitForCompletions.wait(processor.data(),
                                                                               assistInterface,
                                                                               timeOutInMs);
    QTC_ASSERT(result == WaitForAsyncCompletions::GotResults,
               return TextEditor::ProposalModelPtr());
    return waitForCompletions.proposalModel;
}

QString qrcPath(const QByteArray &relativeFilePath)
{
    return QLatin1String(":/unittests/ClangCodeModel/") + QString::fromUtf8(relativeFilePath);
}

} // namespace Internal
} // namespace ClangCodeModel
