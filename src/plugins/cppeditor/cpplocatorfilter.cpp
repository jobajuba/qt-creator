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

#include "cpplocatorfilter.h"

#include "cppeditorconstants.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>

#include <QRegularExpression>

#include <algorithm>
#include <numeric>

namespace CppEditor {

CppLocatorFilter::CppLocatorFilter(CppLocatorData *locatorData)
    : m_data(locatorData)
{
    setId(Constants::LOCATOR_FILTER_ID);
    setDisplayName(Constants::LOCATOR_FILTER_DISPLAY_NAME);
    setDefaultShortcutString(":");
    setDefaultIncludedByDefault(false);
}

CppLocatorFilter::~CppLocatorFilter() = default;

Core::LocatorFilterEntry CppLocatorFilter::filterEntryFromIndexItem(IndexItem::Ptr info)
{
    const QVariant id = QVariant::fromValue(info);
    Core::LocatorFilterEntry filterEntry(this, info->scopedSymbolName(), id, info->icon());
    if (info->type() == IndexItem::Class || info->type() == IndexItem::Enum)
        filterEntry.extraInfo = info->shortNativeFilePath();
    else
        filterEntry.extraInfo = info->symbolType();

    return filterEntry;
}

QList<Core::LocatorFilterEntry> CppLocatorFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    QList<Core::LocatorFilterEntry> entries[int(MatchLevel::Count)];
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);
    const IndexItem::ItemType wanted = matchTypes();

    const QRegularExpression regexp = createRegExp(entry);
    if (!regexp.isValid())
        return {};
    const bool hasColonColon = entry.contains("::");
    const QRegularExpression shortRegexp =
            hasColonColon ? createRegExp(entry.mid(entry.lastIndexOf("::") + 2)) : regexp;

    m_data->filterAllFiles([&](const IndexItem::Ptr &info) -> IndexItem::VisitorResult {
        if (future.isCanceled())
            return IndexItem::Break;
        const IndexItem::ItemType type = info->type();
        if (type & wanted) {
            const QString symbolName = info->symbolName();
            QString matchString = hasColonColon ? info->scopedSymbolName() : symbolName;
            int matchOffset = hasColonColon ? matchString.size() - symbolName.size() : 0;
            QRegularExpressionMatch match = regexp.match(matchString);
            bool matchInParameterList = false;
            if (!match.hasMatch() && (type == IndexItem::Function)) {
                matchString += info->symbolType();
                match = regexp.match(matchString);
                matchInParameterList = true;
            }

            if (match.hasMatch()) {
                Core::LocatorFilterEntry filterEntry = filterEntryFromIndexItem(info);

                // Highlight the matched characters, therefore it may be necessary
                // to update the match if the displayName is different from matchString
                if (QStringView(matchString).mid(matchOffset) != filterEntry.displayName) {
                    match = shortRegexp.match(filterEntry.displayName);
                    matchOffset = 0;
                }
                filterEntry.highlightInfo = highlightInfo(match);
                if (matchInParameterList && filterEntry.highlightInfo.starts.isEmpty()) {
                    match = regexp.match(filterEntry.extraInfo);
                    filterEntry.highlightInfo = highlightInfo(match);
                    filterEntry.highlightInfo.dataType =
                            Core::LocatorFilterEntry::HighlightInfo::ExtraInfo;
                } else if (matchOffset > 0) {
                    for (int &start : filterEntry.highlightInfo.starts)
                        start -= matchOffset;
                }

                if (matchInParameterList)
                    entries[int(MatchLevel::Normal)].append(filterEntry);
                else if (filterEntry.displayName.startsWith(entry, caseSensitivityForPrefix))
                    entries[int(MatchLevel::Best)].append(filterEntry);
                else if (filterEntry.displayName.contains(entry, caseSensitivityForPrefix))
                    entries[int(MatchLevel::Better)].append(filterEntry);
                else
                    entries[int(MatchLevel::Good)].append(filterEntry);
            }
        }

        if (info->type() & IndexItem::Enum)
            return IndexItem::Continue;
        else
            return IndexItem::Recurse;
    });

    for (auto &entry : entries) {
        if (entry.size() < 1000)
            Utils::sort(entry, Core::LocatorFilterEntry::compareLexigraphically);
    }

    return std::accumulate(std::begin(entries), std::end(entries), QList<Core::LocatorFilterEntry>());
}

void CppLocatorFilter::accept(Core::LocatorFilterEntry selection,
                              QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    IndexItem::Ptr info = qvariant_cast<IndexItem::Ptr>(selection.internalData);
    Core::EditorManager::openEditorAt(info->fileName(), info->line(), info->column());
}

CppClassesFilter::CppClassesFilter(CppLocatorData *locatorData)
    : CppLocatorFilter(locatorData)
{
    setId(Constants::CLASSES_FILTER_ID);
    setDisplayName(Constants::CLASSES_FILTER_DISPLAY_NAME);
    setDefaultShortcutString("c");
    setDefaultIncludedByDefault(false);
}

CppClassesFilter::~CppClassesFilter() = default;

Core::LocatorFilterEntry CppClassesFilter::filterEntryFromIndexItem(IndexItem::Ptr info)
{
    const QVariant id = QVariant::fromValue(info);
    Core::LocatorFilterEntry filterEntry(this, info->symbolName(), id, info->icon());
    filterEntry.extraInfo = info->symbolScope().isEmpty()
        ? info->shortNativeFilePath()
        : info->symbolScope();
    filterEntry.filePath = Utils::FilePath::fromString(info->fileName());
    return filterEntry;
}

CppFunctionsFilter::CppFunctionsFilter(CppLocatorData *locatorData)
    : CppLocatorFilter(locatorData)
{
    setId(Constants::FUNCTIONS_FILTER_ID);
    setDisplayName(Constants::FUNCTIONS_FILTER_DISPLAY_NAME);
    setDefaultShortcutString("m");
    setDefaultIncludedByDefault(false);
}

CppFunctionsFilter::~CppFunctionsFilter() = default;

Core::LocatorFilterEntry CppFunctionsFilter::filterEntryFromIndexItem(IndexItem::Ptr info)
{
    const QVariant id = QVariant::fromValue(info);

    QString name = info->symbolName();
    QString extraInfo = info->symbolScope();
    info->unqualifiedNameAndScope(name, &name, &extraInfo);
    if (extraInfo.isEmpty()) {
        extraInfo = info->shortNativeFilePath();
    } else {
        extraInfo.append(" (" + Utils::FilePath::fromString(info->fileName()).fileName() + ')');
    }

    Core::LocatorFilterEntry filterEntry(this, name + info->symbolType(), id, info->icon());
    filterEntry.extraInfo = extraInfo;

    return filterEntry;
}

} // namespace CppEditor
