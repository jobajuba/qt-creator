/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <vector>

namespace QmlDesigner {

template<auto Type, typename InternalIntergerType = long long>
class BasicId
{
public:
    using DatabaseType = InternalIntergerType;

    constexpr explicit BasicId() = default;

    BasicId(const char *) = delete;

    constexpr explicit BasicId(InternalIntergerType id)
        : id{id}
    {}

    constexpr friend bool operator==(BasicId first, BasicId second)
    {
        return first.id == second.id && first.isValid() && second.isValid();
    }

    constexpr friend bool operator!=(BasicId first, BasicId second) { return !(first == second); }

    constexpr friend bool operator<(BasicId first, BasicId second) { return first.id < second.id; }
    constexpr friend bool operator>(BasicId first, BasicId second) { return first.id > second.id; }
    constexpr friend bool operator<=(BasicId first, BasicId second)
    {
        return first.id <= second.id;
    }
    constexpr friend bool operator>=(BasicId first, BasicId second)
    {
        return first.id >= second.id;
    }

    constexpr bool isValid() const { return id >= 0; }

    explicit operator bool() const { return isValid(); }

    explicit operator std::size_t() const { return static_cast<std::size_t>(id); }

    InternalIntergerType operator&() const { return id; }

public:
    InternalIntergerType id = -1;
};

enum class BasicIdType {
    Type,
    PropertyType,
    PropertyDeclaration,
    SourceId,
    SourceContextId,
    StorageCacheIndex,
    FunctionDeclaration,
    SignalDeclaration,
    EnumerationDeclaration,
    Import,
    TypeName,
    ProjectPartId
};

using TypeId = BasicId<BasicIdType::Type>;
using TypeIds = std::vector<TypeId>;

using PropertyDeclarationId = BasicId<BasicIdType::PropertyDeclaration>;
using PropertyDeclarationIds = std::vector<PropertyDeclarationId>;

using FunctionDeclarationId = BasicId<BasicIdType::FunctionDeclaration>;
using FunctionDeclarationIds = std::vector<FunctionDeclarationId>;

using SignalDeclarationId = BasicId<BasicIdType::SignalDeclaration>;
using SignalDeclarationIds = std::vector<SignalDeclarationId>;

using EnumerationDeclarationId = BasicId<BasicIdType::EnumerationDeclaration>;
using EnumerationDeclarationIds = std::vector<EnumerationDeclarationId>;

using SourceContextId = BasicId<BasicIdType::SourceContextId, int>;
using SourceContextIds = std::vector<SourceContextId>;

using SourceId = BasicId<BasicIdType::SourceId, int>;
using SourceIds = std::vector<SourceId>;

using ImportId = BasicId<BasicIdType::Import>;
using ImportIds = std::vector<ImportId>;

using TypeNameId = BasicId<BasicIdType::TypeName>;
using TypeNameIds = std::vector<TypeNameId>;

using ProjectPartId = BasicId<BasicIdType::ProjectPartId>;
using ProjectPartIds = std::vector<ProjectPartId>;

} // namespace QmlDesigner
