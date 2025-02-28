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
#include "cpplocatordata.h"
#include "searchsymbols.h"

#include <coreplugin/locator/ilocatorfilter.h>

namespace CppEditor {

class CPPEDITOR_EXPORT CppLocatorFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    explicit CppLocatorFilter(CppLocatorData *locatorData);
    ~CppLocatorFilter() override;

    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(Core::LocatorFilterEntry selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;

protected:
    virtual IndexItem::ItemType matchTypes() const { return IndexItem::All; }
    virtual Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info);

protected:
    CppLocatorData *m_data = nullptr;
};

class CPPEDITOR_EXPORT CppClassesFilter : public CppLocatorFilter
{
    Q_OBJECT

public:
    explicit CppClassesFilter(CppLocatorData *locatorData);
    ~CppClassesFilter() override;

protected:
    IndexItem::ItemType matchTypes() const override { return IndexItem::Class; }
    Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info) override;
};

class CPPEDITOR_EXPORT CppFunctionsFilter : public CppLocatorFilter
{
    Q_OBJECT

public:
    explicit CppFunctionsFilter(CppLocatorData *locatorData);
    ~CppFunctionsFilter() override;

protected:
    IndexItem::ItemType matchTypes() const override { return IndexItem::Function; }
    Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info) override;
};

} // namespace CppEditor
