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

#include "cppeditor_global.h"

#include <texteditor/icodestylepreferencesfactory.h>

namespace CppEditor {

class CPPEDITOR_EXPORT CppCodeStylePreferencesFactory : public TextEditor::ICodeStylePreferencesFactory
{
public:
    CppCodeStylePreferencesFactory();

    Utils::Id languageId() override;
    QString displayName() override;
    TextEditor::ICodeStylePreferences *createCodeStyle() const override;
    QWidget *createEditor(TextEditor::ICodeStylePreferences *settings,
                          QWidget *parent) const override;
    TextEditor::Indenter *createIndenter(QTextDocument *doc) const override;
    QString snippetProviderGroupId() const override;
    QString previewText() const override;
};

} // namespace CppEditor
