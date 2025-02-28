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

#include "googletest.h"

#include "sqlitedatabasemock.h"

#include <modelnode.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/sourcepathcache.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace {

using QmlDesigner::FileStatus;
using QmlDesigner::FileStatuses;
using QmlDesigner::ImportId;
using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::TypeId;
using QmlDesigner::Cache::Source;
using QmlDesigner::Cache::SourceContext;
using QmlDesigner::Storage::TypeAccessSemantics;

namespace Storage = QmlDesigner::Storage;

MATCHER_P2(IsSourceContext,
           id,
           value,
           std::string(negation ? "isn't " : "is ") + PrintToString(SourceContext{value, id}))
{
    const SourceContext &sourceContext = arg;

    return sourceContext.id == id && sourceContext.value == value;
}

MATCHER_P2(IsSourceNameAndSourceContextId,
           name,
           id,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(QmlDesigner::Cache::SourceNameAndSourceContextId{name, id}))
{
    const QmlDesigner::Cache::SourceNameAndSourceContextId &sourceNameAndSourceContextId = arg;

    return sourceNameAndSourceContextId.sourceName == name
           && sourceNameAndSourceContextId.sourceContextId == id;
}

MATCHER_P5(IsStorageType,
           import,
           typeName,
           prototype,
           accessSemantics,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{import, typeName, prototype, accessSemantics, sourceId}))
{
    const Storage::Type &type = arg;

    return type.import == import && type.typeName == typeName
           && type.accessSemantics == accessSemantics && type.sourceId == sourceId
           && Utils::visit([&](auto &&v) -> bool { return v.name == prototype.name; }, type.prototype);
}

MATCHER_P4(IsStorageTypeWithInvalidSourceId,
           import,
           typeName,
           prototype,
           accessSemantics,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{import, typeName, prototype, accessSemantics, SourceId{}}))
{
    const Storage::Type &type = arg;

    return type.import == import && type.typeName == typeName && type.prototype == prototype
           && type.accessSemantics == accessSemantics && !type.sourceId.isValid();
}

MATCHER_P(IsExportedType,
          name,
          std::string(negation ? "isn't " : "is ") + PrintToString(Storage::ExportedType{name}))
{
    const Storage::ExportedType &type = arg;

    return type.name == name;
}

MATCHER_P3(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::PropertyDeclaration{name, typeName, traits}))
{
    const Storage::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Utils::visit([&](auto &&v) -> bool { return v.name == typeName.name; },
                         propertyDeclaration.typeName)
           && propertyDeclaration.traits == traits;
}

MATCHER_P4(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           aliasPropertyName,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::PropertyDeclaration(name, typeName, traits, aliasPropertyName)))
{
    const Storage::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Utils::visit([&](auto &&v) -> bool { return v.name == typeName.name; },
                           propertyDeclaration.typeName)
           && propertyDeclaration.aliasPropertyName == aliasPropertyName
           && propertyDeclaration.traits == traits;
}

MATCHER_P2(IsImport,
           name,
           version,
           std::string(negation ? "isn't " : "is ") + PrintToString(Storage::Import{name, version}))
{
    const Storage::Import &import = arg;

    return import.name == name && import.version == version;
}

MATCHER_P3(IsImportDependency,
           name,
           version,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ImportDependency{name, version, sourceId}))
{
    const Storage::ImportDependency &import = arg;

    return import.name == name && import.version == version && &import.sourceId == &sourceId;
}

class ProjectStorage : public testing::Test
{
public:
    ProjectStorage()
    {
        ON_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(A<Utils::SmallStringView>()))
            .WillByDefault(Return(SourceContextId()));
        ON_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(Utils::SmallStringView("")))
            .WillByDefault(Return(SourceContextId(0)));
        ON_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(Utils::SmallStringView("/path/to")))
            .WillByDefault(Return(SourceContextId(5)));
        ON_CALL(databaseMock, lastInsertedRowId()).WillByDefault(Return(12));
        ON_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnInt32(_, _))
            .WillByDefault(Return(Utils::optional<int>()));
        ON_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnInt32(0, Utils::SmallStringView("")))
            .WillByDefault(Return(Utils::optional<int>(0)));
        ON_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnInt32(5, Utils::SmallStringView("file.h")))
            .WillByDefault(Return(Utils::optional<int>(42)));
        ON_CALL(selectAllSourcesStatement, valuesReturnCacheSources(_))
            .WillByDefault(Return(std::vector<Source>{{"file.h", SourceContextId{1}, SourceId{1}},
                                                      {"file.cpp", SourceContextId{2}, SourceId{4}}}));
        ON_CALL(selectSourceContextPathFromSourceContextsBySourceContextIdStatement,
                valueReturnPathString(5))
            .WillByDefault(Return(Utils::optional<Utils::PathString>("/path/to")));
        ON_CALL(selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement,
                valueReturnCacheSourceNameAndSourceContextId(42))
            .WillByDefault(Return(QmlDesigner::Cache::SourceNameAndSourceContextId{"file.cpp", 5}));
        ON_CALL(selectSourceContextIdFromSourcesBySourceIdStatement,
                valueReturnInt32(TypedEq<int>(42)))
            .WillByDefault(Return(Utils::optional<int>(5)));
    }

protected:
    using ProjectStorageMock = QmlDesigner::ProjectStorage<SqliteDatabaseMock>;
    template<int ResultCount>
    using ReadStatement = ProjectStorageMock::ReadStatement<ResultCount>;
    using WriteStatement = ProjectStorageMock::WriteStatement;
    template<int ResultCount>
    using ReadWriteStatement = ProjectStorageMock::ReadWriteStatement<ResultCount>;

    NiceMock<SqliteDatabaseMock> databaseMock;
    ProjectStorageMock storage{databaseMock, true};
    ReadWriteStatement<1> &upsertTypeStatement = storage.upsertTypeStatement;
    ReadStatement<1> &selectTypeIdByExportedNameStatement = storage.selectTypeIdByExportedNameStatement;
    ReadStatement<1> &selectSourceContextIdFromSourceContextsBySourceContextPathStatement
        = storage.selectSourceContextIdFromSourceContextsBySourceContextPathStatement;
    ReadStatement<1> &selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment
        = storage.selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatement;
    ReadStatement<1> &selectSourceContextPathFromSourceContextsBySourceContextIdStatement
        = storage.selectSourceContextPathFromSourceContextsBySourceContextIdStatement;
    ReadStatement<2> &selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement
        = storage.selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement;
    ReadStatement<2> &selectAllSourceContextsStatement = storage.selectAllSourceContextsStatement;
    WriteStatement &insertIntoSourceContexts = storage.insertIntoSourceContextsStatement;
    WriteStatement &insertIntoSourcesStatement = storage.insertIntoSourcesStatement;
    ReadStatement<3> &selectAllSourcesStatement = storage.selectAllSourcesStatement;
    ReadStatement<1> &selectSourceContextIdFromSourcesBySourceIdStatement = storage.selectSourceContextIdFromSourcesBySourceIdStatement;
    ReadStatement<5> &selectTypeByTypeIdStatement = storage.selectTypeByTypeIdStatement;
    ReadStatement<1> &selectExportedTypesByTypeIdStatement = storage.selectExportedTypesByTypeIdStatement;
    ReadStatement<7> &selectTypesStatement = storage.selectTypesStatement;
};

TEST_F(ProjectStorage, SelectForFetchingSourceContextIdForKnownPathCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(TypedEq<Utils::SmallStringView>("/path/to")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceContextId("/path/to");
}

TEST_F(ProjectStorage, SelectForFetchingSourceIdForKnownPathCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnsSourceId(5, Eq("file.h")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceId(SourceContextId{5}, "file.h");
}

TEST_F(ProjectStorage, NotWriteForFetchingSourceContextIdForKnownPathCalls)
{
    EXPECT_CALL(insertIntoSourceContexts, write(An<Utils::SmallStringView>())).Times(0);

    storage.fetchSourceContextId("/path/to");
}

TEST_F(ProjectStorage, NotWriteForFetchingSoureIdForKnownEntryCalls)
{
    EXPECT_CALL(insertIntoSourcesStatement, write(An<uint>(), An<Utils::SmallStringView>())).Times(0);

    storage.fetchSourceId(SourceContextId{5}, "file.h");
}

TEST_F(ProjectStorage, SelectAndWriteForFetchingSourceContextIdForUnknownPathCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(
                    TypedEq<Utils::SmallStringView>("/some/not/known/path")));
    EXPECT_CALL(insertIntoSourceContexts,
                write(TypedEq<Utils::SmallStringView>("/some/not/known/path")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceContextId("/some/not/known/path");
}

TEST_F(ProjectStorage, SelectAndWriteForFetchingSourceIdForUnknownEntryCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnsSourceId(5, Eq("unknownfile.h")));
    EXPECT_CALL(insertIntoSourcesStatement,
                write(TypedEq<int>(5), TypedEq<Utils::SmallStringView>("unknownfile.h")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceId(SourceContextId{5}, "unknownfile.h");
}

TEST_F(ProjectStorage, ValueForFetchSourceContextForIdCalls)
{
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextPathFromSourceContextsBySourceContextIdStatement,
                valueReturnPathString(5));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceContextPath(SourceContextId{5});
}

TEST_F(ProjectStorage, FetchSourceContextForId)
{
    auto path = storage.fetchSourceContextPath(SourceContextId{5});

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(ProjectStorage, ThrowAsFetchingSourceContextPathForNonExistingId)
{
    ASSERT_THROW(storage.fetchSourceContextPath(SourceContextId{12}),
                 QmlDesigner::SourceContextIdDoesNotExists);
}

TEST_F(ProjectStorage, FetchSourceContextIdForUnknownSourceId)
{
    ASSERT_THROW(storage.fetchSourceContextId(SourceId{1111}), QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorage, FetchSourceContextIdThrows)
{
    ASSERT_THROW(storage.fetchSourceContextId(SourceId{41}), QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorage, GetTheSourceContextIdBackAfterFetchingANewEntryFromSourceContextsUnguarded)
{
    auto sourceContextId = storage.fetchSourceContextIdUnguarded("/some/not/known/path");

    ASSERT_THAT(sourceContextId, SourceContextId{12});
}

TEST_F(ProjectStorage, GetTheSourceIdBackAfterFetchingANewEntryFromSourcesUnguarded)
{
    auto sourceId = storage.fetchSourceIdUnguarded(SourceContextId{5}, "unknownfile.h");

    ASSERT_THAT(sourceId, SourceId{12});
}

TEST_F(ProjectStorage, SelectForFetchingSourceContextIdForKnownPathUnguardedCalls)
{
    InSequence s;

    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(TypedEq<Utils::SmallStringView>("/path/to")));

    storage.fetchSourceContextIdUnguarded("/path/to");
}

TEST_F(ProjectStorage, SelectForFetchingSourceIdForKnownPathUnguardedCalls)
{
    EXPECT_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnsSourceId(5, Eq("file.h")));

    storage.fetchSourceIdUnguarded(SourceContextId{5}, "file.h");
}

TEST_F(ProjectStorage, NotWriteForFetchingSourceContextIdForKnownPathUnguardedCalls)
{
    EXPECT_CALL(insertIntoSourceContexts, write(An<Utils::SmallStringView>())).Times(0);

    storage.fetchSourceContextIdUnguarded("/path/to");
}

TEST_F(ProjectStorage, NotWriteForFetchingSoureIdForKnownEntryUnguardedCalls)
{
    EXPECT_CALL(insertIntoSourcesStatement, write(An<uint>(), An<Utils::SmallStringView>())).Times(0);

    storage.fetchSourceIdUnguarded(SourceContextId{5}, "file.h");
}

TEST_F(ProjectStorage, SelectAndWriteForFetchingSourceContextIdForUnknownPathUnguardedCalls)
{
    InSequence s;

    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(
                    TypedEq<Utils::SmallStringView>("/some/not/known/path")));
    EXPECT_CALL(insertIntoSourceContexts,
                write(TypedEq<Utils::SmallStringView>("/some/not/known/path")));

    storage.fetchSourceContextIdUnguarded("/some/not/known/path");
}

TEST_F(ProjectStorage, SelectAndWriteForFetchingSourceIdForUnknownEntryUnguardedCalls)
{
    InSequence s;

    EXPECT_CALL(selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatment,
                valueReturnsSourceId(5, Eq("unknownfile.h")));
    EXPECT_CALL(insertIntoSourcesStatement,
                write(TypedEq<int>(5), TypedEq<Utils::SmallStringView>("unknownfile.h")));

    storage.fetchSourceIdUnguarded(SourceContextId{5}, "unknownfile.h");
}

TEST_F(ProjectStorage,
       SelectAndWriteForFetchingSourceContextIdTwoTimesIfTheIndexIsConstraintBecauseTheEntryExistsAlreadyCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(TypedEq<Utils::SmallStringView>("/other/unknow/path")));
    EXPECT_CALL(insertIntoSourceContexts, write(TypedEq<Utils::SmallStringView>("/other/unknow/path")))
        .WillOnce(Throw(Sqlite::ConstraintPreventsModification("busy")));
    EXPECT_CALL(databaseMock, rollback());
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSourceContextIdFromSourceContextsBySourceContextPathStatement,
                valueReturnsSourceContextId(TypedEq<Utils::SmallStringView>("/other/unknow/path")));
    EXPECT_CALL(insertIntoSourceContexts,
                write(TypedEq<Utils::SmallStringView>("/other/unknow/path")));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSourceContextId("/other/unknow/path");
}

TEST_F(ProjectStorage, FetchTypeByTypeIdCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectTypeByTypeIdStatement, valueReturnsStorageType(Eq(21)));
    EXPECT_CALL(selectExportedTypesByTypeIdStatement, valuesReturnsStorageExportedTypes(_, Eq(21)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchTypeByTypeId(TypeId{21});
}

TEST_F(ProjectStorage, FetchTypesCalls)
{
    InSequence s;
    Storage::Type type{Storage::Import{}, {}, {}, {}, SourceId{}, {}, {}, {}, {}, {}, TypeId{55}};
    Storage::Types types{type};

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectTypesStatement, valuesReturnsStorageTypes(_)).WillOnce(Return(types));
    EXPECT_CALL(selectExportedTypesByTypeIdStatement, valuesReturnsStorageExportedTypes(_, Eq(55)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchTypes();
}

class ProjectStorageSlowTest : public testing::Test
{
protected:
    template<typename Range>
    static auto toValues(Range &&range)
    {
        using Type = typename std::decay_t<Range>;

        return std::vector<typename Type::value_type>{range.begin(), range.end()};
    }

    void addSomeDummyData()
    {
        auto sourceContextId1 = storage.fetchSourceContextId("/path/dummy");
        auto sourceContextId2 = storage.fetchSourceContextId("/path/dummy2");
        auto sourceContextId3 = storage.fetchSourceContextId("/path/");

        storage.fetchSourceId(sourceContextId1, "foo");
        storage.fetchSourceId(sourceContextId1, "dummy");
        storage.fetchSourceId(sourceContextId2, "foo");
        storage.fetchSourceId(sourceContextId2, "bar");
        storage.fetchSourceId(sourceContextId3, "foo");
        storage.fetchSourceId(sourceContextId3, "bar");
        storage.fetchSourceId(sourceContextId1, "bar");
        storage.fetchSourceId(sourceContextId3, "bar");
    }

    auto createTypes()
    {
        setUpImportDependenciesAndDocuments();

        return Storage::Types{
            Storage::Type{
                Storage::Import{"QtQuick"},
                "QQuickItem",
                Storage::NativeType{"QObject"},
                TypeAccessSemantics::Reference,
                sourceId1,
                {Storage::ExportedType{"Item"}},
                {Storage::PropertyDeclaration{"data",
                                              Storage::NativeType{"QObject"},
                                              Storage::PropertyDeclarationTraits::IsList},
                 Storage::PropertyDeclaration{"children",
                                              Storage::ExportedType{"Item"},
                                              Storage::PropertyDeclarationTraits::IsList
                                                  | Storage::PropertyDeclarationTraits::IsReadOnly}},
                {Storage::FunctionDeclaration{"execute", "", {Storage::ParameterDeclaration{"arg", ""}}},
                 Storage::FunctionDeclaration{
                     "values",
                     "Vector3D",
                     {Storage::ParameterDeclaration{"arg1", "int"},
                      Storage::ParameterDeclaration{"arg2",
                                                    "QObject",
                                                    Storage::PropertyDeclarationTraits::IsPointer},
                      Storage::ParameterDeclaration{"arg3", "string"}}}},
                {Storage::SignalDeclaration{"execute", {Storage::ParameterDeclaration{"arg", ""}}},
                 Storage::SignalDeclaration{
                     "values",
                     {Storage::ParameterDeclaration{"arg1", "int"},
                      Storage::ParameterDeclaration{"arg2",
                                                    "QObject",
                                                    Storage::PropertyDeclarationTraits::IsPointer},
                      Storage::ParameterDeclaration{"arg3", "string"}}}},
                {Storage::EnumerationDeclaration{"Enum",
                                                 {Storage::EnumeratorDeclaration{"Foo"},
                                                  Storage::EnumeratorDeclaration{"Bar", 32}}},
                 Storage::EnumerationDeclaration{"Type",
                                                 {Storage::EnumeratorDeclaration{"Foo"},
                                                  Storage::EnumeratorDeclaration{"Poo", 12}}}}},
            Storage::Type{Storage::Import{"Qml", 2},
                          "QObject",
                          Storage::NativeType{},
                          TypeAccessSemantics::Reference,
                          sourceId2,
                          {Storage::ExportedType{"Object"}, Storage::ExportedType{"Obj"}}}};
    }

    auto createTypesWithExportedTypeNamesOnly()
    {
        auto types = createTypes();

        types[0].prototype = Storage::ExportedType{"Object"};
        types[0].propertyDeclarations[0].typeName = Storage::ExportedType{"Object"};

        return types;
    }

    auto createTypesWithAliases()
    {
        auto types = createTypes();

        types[1].propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});

        types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                      "QAliasItem",
                                      Storage::ExportedType{"Item"},
                                      TypeAccessSemantics::Reference,
                                      sourceId3,
                                      {Storage::ExportedType{"AliasItem"}}});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"data",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"items", Storage::ExportedType{"Item"}, "children"});
        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects", Storage::ExportedType{"Item"}, "objects"});

        types.push_back(
            Storage::Type{Storage::Import{"/path/to"},
                          "QObject2",
                          Storage::NativeType{},
                          TypeAccessSemantics::Reference,
                          sourceId4,
                          {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});
        types[3].propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList});

        return types;
    }

    auto createTypesWithRecursiveAliases()
    {
        auto types = createTypesWithAliases();
        types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                      "QAliasItem2",
                                      Storage::ExportedType{"Object"},
                                      TypeAccessSemantics::Reference,
                                      sourceId5,
                                      {Storage::ExportedType{"AliasItem2"}}});

        types.back().propertyDeclarations.push_back(
            Storage::PropertyDeclaration{"objects", Storage::ExportedType{"AliasItem"}, "objects"});

        return types;
    }

    auto createTypesWithAliases2()
    {
        auto types = createTypesWithAliases();
        types[2].prototype = Storage::NativeType{"QObject"};
        types[2].propertyDeclarations.erase(std::next(types[2].propertyDeclarations.begin()));

        return types;
    }

    auto createImportDependencies()
    {
        importSourceId1 = sourcePathCache.sourceId(importPath1);
        importSourceId2 = sourcePathCache.sourceId(importPath2);
        importSourceId3 = sourcePathCache.sourceId(importPath3);
        importSourceId5 = sourcePathCache.sourceId("/path/to/.");

        return Storage::ImportDependencies{
            Storage::ImportDependency{"Qml", Storage::VersionNumber{2}, importSourceId1, {}},
            Storage::ImportDependency{"QtQuick",
                                      Storage::VersionNumber{},
                                      importSourceId2,
                                      {Storage::Import{"Qml", Storage::VersionNumber{2}}}},
            Storage::ImportDependency{"/path/to",
                                      Storage::VersionNumber{},
                                      importSourceId5,
                                      {Storage::Import{"QtQuick"},
                                       Storage::Import{"Qml", Storage::VersionNumber{2}}}}};
    }

    Storage::Imports createImports()
    {
        return Storage::Imports{Storage::Import{"Qml", Storage::VersionNumber{2}},
                                Storage::Import{"QtQuick", Storage::VersionNumber{}},
                                Storage::Import{"/path/to", Storage::VersionNumber{}}};
    }

    auto createImportDependencies2()
    {
        importSourceId4 = sourcePathCache.sourceId(importPath4);

        auto newImportDependencies = createImportDependencies();
        newImportDependencies.push_back(
            Storage::ImportDependency{"Qml2", Storage::VersionNumber{3}, importSourceId4, {}});

        return newImportDependencies;
    }

    void setUpSourceIds()
    {
        sourceId1 = sourcePathCache.sourceId(path1);
        sourceId2 = sourcePathCache.sourceId(path2);
        sourceId3 = sourcePathCache.sourceId(path3);
        sourceId4 = sourcePathCache.sourceId(path4);
        sourceId5 = sourcePathCache.sourceId(path5);
    }

    void setUpImportDependenciesAndDocuments()
    {
        setUpImports();
        setUpSourceIds();

        importDependencies = createImportDependencies();

        documents = {Storage::Document{sourceId1, imports},
                     Storage::Document{sourceId2, imports},
                     Storage::Document{sourceId3, imports},
                     Storage::Document{sourceId4, imports},
                     Storage::Document{sourceId5, imports}};

        storage.synchronize(importDependencies,
                            documents,
                            {},
                            {sourceId1,
                             sourceId2,
                             sourceId3,
                             sourceId4,
                             sourceId5,
                             importSourceId1,
                             importSourceId2,
                             importSourceId5},
                            {});
        importIds = storage.fetchImportIds(imports);
        importId1 = importIds[0];
        importId2 = importIds[1];
        importId3 = importIds[2];
    }

    void setUpImports() { imports = createImports(); }

    template<typename Range>
    static FileStatuses convert(const Range &range)
    {
        return FileStatuses(range.begin(), range.end());
    }

protected:
    using ProjectStorage = QmlDesigner::ProjectStorage<Sqlite::Database>;
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    //Sqlite::Database database{TESTDATA_DIR "/alias7.db", Sqlite::JournalMode::Memory};
    ProjectStorage storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<ProjectStorage> sourcePathCache{storage};
    QmlDesigner::SourcePathView path1{"/path1/to"};
    QmlDesigner::SourcePathView path2{"/path2/to"};
    QmlDesigner::SourcePathView path3{"/path3/to"};
    QmlDesigner::SourcePathView path4{"/path4/to"};
    QmlDesigner::SourcePathView path5{"/path5/to"};
    SourceId sourceId1;
    SourceId sourceId2;
    SourceId sourceId3;
    SourceId sourceId4;
    SourceId sourceId5;
    QmlDesigner::SourcePathView importPath1{"/import/path1/to"};
    QmlDesigner::SourcePathView importPath2{"/import/path2/to"};
    QmlDesigner::SourcePathView importPath3{"/import/aaaa/to"};
    QmlDesigner::SourcePathView importPath4{"/import/ooo/to"};
    SourceId importSourceId1;
    SourceId importSourceId2;
    SourceId importSourceId3;
    SourceId importSourceId4;
    SourceId importSourceId5;
    Storage::Imports imports;
    ImportId importId1;
    ImportId importId2;
    ImportId importId3;
    Storage::ImportDependencies importDependencies;
    Storage::Documents documents;
    QmlDesigner::ImportIds importIds;
};

TEST_F(ProjectStorageSlowTest, FetchSourceContextIdReturnsAlwaysTheSameIdForTheSamePath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to");

    ASSERT_THAT(newSourceContextId, Eq(sourceContextId));
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextIdReturnsNotTheSameIdForDifferentPath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to2");

    ASSERT_THAT(newSourceContextId, Ne(sourceContextId));
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextPath)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto path = storage.fetchSourceContextPath(sourceContextId);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(ProjectStorageSlowTest, FetchUnknownSourceContextPathThrows)
{
    ASSERT_THROW(storage.fetchSourceContextPath(SourceContextId{323}),
                 QmlDesigner::SourceContextIdDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, FetchAllSourceContextsAreEmptyIfNoSourceContextsExists)
{
    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts), IsEmpty());
}

TEST_F(ProjectStorageSlowTest, FetchAllSourceContexts)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");

    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts),
                UnorderedElementsAre(IsSourceContext(sourceContextId, "/path/to"),
                                     IsSourceContext(sourceContextId2, "/path/to2")));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdFirstTime)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchExistingSourceId)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto createdSourceId = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdWithDifferentContextIdAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");
    auto sourceId2 = storage.fetchSourceId(sourceContextId2, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdWithDifferentNameAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceId2 = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceId(sourceContextId, "foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdWithNonExistingSourceContextIdThrows)
{
    ASSERT_THROW(storage.fetchSourceId(SourceContextId{42}, "foo"),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorageSlowTest, FetchSourceNameAndSourceContextIdForNonExistingSourceId)
{
    ASSERT_THROW(storage.fetchSourceNameAndSourceContextId(SourceId{212}),
                 QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, FetchSourceNameAndSourceContextIdForNonExistingEntry)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceId = storage.fetchSourceId(sourceContextId, "foo");

    auto sourceNameAndSourceContextId = storage.fetchSourceNameAndSourceContextId(sourceId);

    ASSERT_THAT(sourceNameAndSourceContextId, IsSourceNameAndSourceContextId("foo", sourceContextId));
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextIdForNonExistingSourceId)
{
    ASSERT_THROW(storage.fetchSourceContextId(SourceId{212}), QmlDesigner::SourceIdDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, FetchSourceContextIdForExistingSourceId)
{
    addSomeDummyData();
    auto originalSourceContextId = storage.fetchSourceContextId("/path/to3");
    auto sourceId = storage.fetchSourceId(originalSourceContextId, "foo");

    auto sourceContextId = storage.fetchSourceContextId(sourceId);

    ASSERT_THAT(sourceContextId, Eq(originalSourceContextId));
}

TEST_F(ProjectStorageSlowTest, FetchAllSources)
{
    auto sources = storage.fetchAllSources();

    ASSERT_THAT(toValues(sources), IsEmpty());
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdUnguardedFirstTime)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchExistingSourceIdUnguarded)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};
    auto createdSourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdUnguardedWithDifferentContextIdAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceIdUnguarded(sourceContextId2, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdUnguardedWithDifferentNameAreNotEqual)
{
    addSomeDummyData();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceIdUnguarded(sourceContextId, "foo");

    auto sourceId = storage.fetchSourceIdUnguarded(sourceContextId, "foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}

TEST_F(ProjectStorageSlowTest, FetchSourceIdUnguardedWithNonExistingSourceContextIdThrows)
{
    std::lock_guard lock{database};

    ASSERT_THROW(storage.fetchSourceIdUnguarded(SourceContextId{42}, "foo"),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsNewTypes)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsNewTypesWithExportedPrototypeName)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExportedType{"Object"};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsNewTypesThrowsWithWrongExportedPrototypeName)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExportedType{"Objec"};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsNewTypesWithMissingImportAndExportedPrototypeName)
{
    Storage::Types types{createTypes()};
    types.push_back(Storage::Type{Storage::Import{"/path/to"},
                                  "QObject2",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId4,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});
    storage.synchronize({}, {Storage::Document{sourceId1, {imports[0]}}}, {}, {sourceId1}, {});
    types[1].prototype = Storage::ExportedType{"Object2"};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsNewTypesWithMissingImport)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {Storage::Document{sourceId1, {imports[0]}}}, {}, {sourceId1}, {});

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsNewTypesReverseOrder)
{
    Storage::Types types{createTypes()};
    std::reverse(types.begin(), types.end());

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesOverwritesTypeAccessSemantics)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].accessSemantics = TypeAccessSemantics::Value;
    types[1].accessSemantics = TypeAccessSemantics::Value;

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Value,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Value,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesOverwritesSources)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].sourceId = sourceId3;
    types[1].sourceId = sourceId4;

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId4),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId3),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesInsertTypeIntoPrototypeChain)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].prototype = Storage::NativeType{"QQuickObject"};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickObject",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QQuickObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddExplicitPrototype)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"QtQuick"}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickObject",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QQuickObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesThrowsForMissingPrototype)
{
    setUpImportDependenciesAndDocuments();
    sourceId1 = sourcePathCache.sourceId(path1);
    Storage::Types types{Storage::Type{Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1,
                                       {Storage::ExportedType{"Item"}}}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesThrowsForMissingImport)
{
    sourceId1 = sourcePathCache.sourceId(path1);
    Storage::Types types{Storage::Type{Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1,
                                       {Storage::ExportedType{"Item"}}}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1}, {}),
                 QmlDesigner::ImportDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, TypeWithInvalidSourceIdThrows)
{
    Storage::Types types{Storage::Type{Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{""},
                                       TypeAccessSemantics::Reference,
                                       SourceId{},
                                       {Storage::ExportedType{"Item"}}}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {}, {}), QmlDesigner::TypeHasInvalidSourceId);
}

TEST_F(ProjectStorageSlowTest, DeleteTypeIfSourceIdIsSynchronized)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types.erase(types.begin());

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj"))))));
}

TEST_F(ProjectStorageSlowTest, DontDeleteTypeIfSourceIdIsNotSynchronized)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types.pop_back();

    storage.synchronize({}, {}, types, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, UpdateExportedTypesIfTypeNameChanges)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].typeName = "QQuickItem2";

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj")))),
                                     AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                         "QQuickItem2",
                                                         Storage::NativeType{"QObject"},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId1),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Item"))))));
}

TEST_F(ProjectStorageSlowTest, BreakingPrototypeChainByDeletingBaseComponentThrows)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddPropertyDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesAddPropertyDeclarationsWithMissingImportIdsForNativeTypes)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {Storage::Document{sourceId1, {}}}, {}, {sourceId1}, {});
    types[0].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesAddPropertyDeclarationsWithMissingImportIdsForExportedTypes)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {Storage::Document{sourceId1, {imports[0]}}}, {}, {sourceId1}, {});
    types[0].propertyDeclarations[0].typeName = Storage::ExportedType{"Obj"};

    ASSERT_THROW(storage.synchronize({}, {}, types, {}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddPropertyDeclarationExplicitType)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Import{
                                                                                  "QtQuick"}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId1,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QQuickObject"},
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesPropertyDeclarationType)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations[0].typeName = Storage::NativeType{"QQuickItem"};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QQuickItem"},
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesDeclarationTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QObject"},
                                                          Storage::PropertyDeclarationTraits::IsPointer),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesDeclarationTraitsAndType)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsPointer;
    types[0].propertyDeclarations[0].typeName = Storage::NativeType{"QQuickItem"};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QQuickItem"},
                                                          Storage::PropertyDeclarationTraits::IsPointer),
                                    IsPropertyDeclaration(
                                        "children",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovesAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::propertyDeclarations,
                                     UnorderedElementsAre(IsPropertyDeclaration(
                                         "data",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddsAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"object",
                                     Storage::NativeType{"QObject"},
                                     Storage::PropertyDeclarationTraits::IsPointer});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(AllOf(
            IsStorageType(Storage::Import{"QtQuick"},
                          "QQuickItem",
                          Storage::NativeType{"QObject"},
                          TypeAccessSemantics::Reference,
                          sourceId1),
            Field(&Storage::Type::propertyDeclarations,
                  UnorderedElementsAre(
                      IsPropertyDeclaration("object",
                                            Storage::NativeType{"QObject"},
                                            Storage::PropertyDeclarationTraits::IsPointer),
                      IsPropertyDeclaration("data",
                                            Storage::NativeType{"QObject"},
                                            Storage::PropertyDeclarationTraits::IsList),
                      IsPropertyDeclaration("children",
                                            Storage::NativeType{"QQuickItem"},
                                            Storage::PropertyDeclarationTraits::IsList
                                                | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRenameAPropertyDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].propertyDeclarations[1].name = "objects";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                        "QQuickItem",
                                        Storage::NativeType{"QObject"},
                                        TypeAccessSemantics::Reference,
                                        sourceId1),
                          Field(&Storage::Type::propertyDeclarations,
                                UnorderedElementsAre(
                                    IsPropertyDeclaration("data",
                                                          Storage::NativeType{"QObject"},
                                                          Storage::PropertyDeclarationTraits::IsList),
                                    IsPropertyDeclaration(
                                        "objects",
                                        Storage::NativeType{"QQuickItem"},
                                        Storage::PropertyDeclarationTraits::IsList
                                            | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, UsingNonExistingNativePropertyTypeThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, UsingNonExistingExportedPropertyTypeThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExportedType{"QObject2"};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, UsingNonExistingExplicitExportedPropertyTypeWithWrongNameThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"QObject2",
                                                                              Storage::Import{
                                                                                  "Qml"}};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, UsingNonExistingExplicitExportedPropertyTypeWithWrongImportThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"QObject",
                                                                              Storage::Import{
                                                                                  "QtQuick"}};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, BreakingPropertyDeclarationTypeDependencyByDeletingTypeThrows)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].prototype = Storage::NativeType{};
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddFunctionDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationReturnType)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].returnTypeName = "item";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].name = "name";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationPopParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationAppendParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters.push_back(Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationChangeParameterName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters[0].name = "other";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationChangeParameterTypeName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters[0].name = "long long";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesFunctionDeclarationChangeParameterTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations[1].parameters[0].traits = QmlDesigner::Storage::PropertyDeclarationTraits::IsList;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovesFunctionDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddFunctionDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].functionDeclarations.push_back(
        Storage::FunctionDeclaration{"name", "string", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::functionDeclarations,
                                     UnorderedElementsAre(Eq(types[0].functionDeclarations[0]),
                                                          Eq(types[0].functionDeclarations[1]),
                                                          Eq(types[0].functionDeclarations[2]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddSignalDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].name = "name";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationPopParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationAppendParameters)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters.push_back(Storage::ParameterDeclaration{"arg4", "int"});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationChangeParameterName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters[0].name = "other";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationChangeParameterTypeName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters[0].typeName = "long long";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesSignalDeclarationChangeParameterTraits)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations[1].parameters[0].traits = QmlDesigner::Storage::PropertyDeclarationTraits::IsList;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovesSignalDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddSignalDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].signalDeclarations.push_back(
        Storage::SignalDeclaration{"name", {Storage::ParameterDeclaration{"arg", "int"}}});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::signalDeclarations,
                                     UnorderedElementsAre(Eq(types[0].signalDeclarations[0]),
                                                          Eq(types[0].signalDeclarations[1]),
                                                          Eq(types[0].signalDeclarations[2]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddEnumerationDeclarations)
{
    Storage::Types types{createTypes()};

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesEnumerationDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].name = "Name";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesEnumerationDeclarationPopEnumeratorDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangesEnumerationDeclarationAppendEnumeratorDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations.push_back(
        Storage::EnumeratorDeclaration{"Haa", 54});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesChangesEnumerationDeclarationChangeEnumeratorDeclarationName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].name = "Hoo";

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesChangesEnumerationDeclarationChangeEnumeratorDeclarationValue)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[1].value = 11;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesChangesEnumerationDeclarationAddThatEnumeratorDeclarationHasValue)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].value = 11;
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = true;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest,
       SynchronizeTypesChangesEnumerationDeclarationRemoveThatEnumeratorDeclarationHasValue)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations[1].enumeratorDeclarations[0].hasValue = false;

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovesEnumerationDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations.pop_back();

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddEnumerationDeclaration)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {}, {});
    types[0].enumerationDeclarations.push_back(
        Storage::EnumerationDeclaration{"name", {Storage::EnumeratorDeclaration{"Foo", 98, true}}});

    storage.synchronize({}, {}, types, {}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::enumerationDeclarations,
                                     UnorderedElementsAre(Eq(types[0].enumerationDeclarations[0]),
                                                          Eq(types[0].enumerationDeclarations[1]),
                                                          Eq(types[0].enumerationDeclarations[2]))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsAddImports)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};

    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                    IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsAddImportsAgain)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});

    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                    IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsUpdateToMoreImports)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    importDependencies.push_back(
        Storage::ImportDependency{"QtQuick.Foo", Storage::VersionNumber{1}, importSourceId3});

    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId3, importSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                    IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                    IsImportDependency("QtQuick.Foo", Storage::VersionNumber{1}, importSourceId3)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsAddOneMoreImports)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    auto newImportDependency = Storage::ImportDependency{"QtQuick.Foo",
                                                         Storage::VersionNumber{1},
                                                         importSourceId3};

    storage.synchronize({newImportDependency}, {}, {}, {importSourceId3}, {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                    IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                    IsImportDependency("QtQuick.Foo", Storage::VersionNumber{1}, importSourceId3)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsAddSameImportNameButDifferentVersion)
{
    auto importSourceIdQml4 = sourcePathCache.sourceId("/path/Qml.4");
    auto importSourceIdQml3 = sourcePathCache.sourceId("/path/Qml.3");
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    importDependencies.push_back(
        Storage::ImportDependency{"Qml", Storage::VersionNumber{4}, importSourceIdQml4});
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    auto newImportDependency = Storage::ImportDependency{"Qml",
                                                         Storage::VersionNumber{3},
                                                         importSourceIdQml3};

    storage.synchronize({newImportDependency}, {}, {}, {importSourceIdQml4, importSourceIdQml3}, {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                    IsImportDependency("Qml", Storage::VersionNumber{3}, importSourceIdQml3),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                    IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsRemoveImport)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});

    storage.synchronize({}, {}, {}, {importSourceId5}, {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsChangeSourceId)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    importDependencies[1].sourceId = importSourceId3;

    storage.synchronize({importDependencies[1]}, {}, {}, {importSourceId2, importSourceId3}, {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId3),
                    IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsChangeName)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    importDependencies[0].name = "Qml2";
    importDependencies[1].dependencies[0].name = "Qml2";
    importDependencies[2].dependencies[1].name = "Qml2";

    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml2", Storage::VersionNumber{2}, importSourceId1),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                    IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsChangeVersion)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    importDependencies[0].version = Storage::VersionNumber{3};
    importDependencies[1].dependencies[0].version = Storage::VersionNumber{3};
    importDependencies[2].dependencies[1].version = Storage::VersionNumber{3};

    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    IsImportDependency("Qml", Storage::VersionNumber{3}, importSourceId1),
                    IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                    IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5)));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsAddImportDependecies)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};

    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                          Field(&Storage::ImportDependency::dependencies,
                                ElementsAre(IsImport("Qml", Storage::VersionNumber{2})))),
                    AllOf(IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                                     IsImport("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsAddImportDependeciesWhichDoesNotExitsThrows)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    importDependencies[1].dependencies.push_back(Storage::Import{"QmlBase", Storage::VersionNumber{2}});

    ASSERT_THROW(storage.synchronize(importDependencies,
                                     {},
                                     {},
                                     {importSourceId1, importSourceId2, importSourceId5},
                                     {}),
                 QmlDesigner::ImportDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsRemovesDependeciesForRemovedImports)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    auto last = importDependencies.back();
    importDependencies.pop_back();
    storage.synchronize({}, {}, {}, {importSourceId5}, {});
    last.dependencies.pop_back();
    importDependencies.push_back(last);

    storage.synchronize({importDependencies[2]}, {}, {}, {importSourceId5}, {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                          Field(&Storage::ImportDependency::dependencies,
                                ElementsAre(IsImport("Qml", Storage::VersionNumber{2})))),
                    AllOf(IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsAddMoreImportDependecies)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    auto importSourceIdQmlBase = sourcePathCache.sourceId("/path/QmlBase");
    importDependencies.push_back(
        Storage::ImportDependency{"QmlBase", Storage::VersionNumber{2}, importSourceIdQmlBase});
    importDependencies[1].dependencies.push_back(Storage::Import{"QmlBase", Storage::VersionNumber{2}});

    storage.synchronize({importDependencies[1], importDependencies[3]},
                        {},
                        {},
                        {importSourceId2, importSourceIdQmlBase},
                        {});

    ASSERT_THAT(
        storage.fetchAllImports(),
        UnorderedElementsAre(
            AllOf(IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                  Field(&Storage::ImportDependency::dependencies, IsEmpty())),
            AllOf(IsImportDependency("QmlBase", Storage::VersionNumber{2}, importSourceIdQmlBase),
                  Field(&Storage::ImportDependency::dependencies, IsEmpty())),
            AllOf(IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                  Field(&Storage::ImportDependency::dependencies,
                        UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                             IsImport("QmlBase", Storage::VersionNumber{2})))),
            AllOf(IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                  Field(&Storage::ImportDependency::dependencies,
                        UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                             IsImport("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsAddMoreImportDependeciesWithDifferentVersionNumber)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    auto importSourceIdQml3 = sourcePathCache.sourceId("/path/Qml.3");
    importDependencies.push_back(
        Storage::ImportDependency{"Qml", Storage::VersionNumber{3}, importSourceIdQml3, {}});
    importDependencies[1].dependencies.push_back(Storage::Import{"Qml", Storage::VersionNumber{3}});

    storage.synchronize({importDependencies[1], importDependencies[3]},
                        {},
                        {},
                        {importSourceId2, importSourceIdQml3},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{3}, importSourceIdQml3),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                                     IsImport("Qml", Storage::VersionNumber{3})))),
                    AllOf(IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                                     IsImport("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsDependencyGetsHighestVersionIfNoVersionIsSupplied)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    auto importSourceIdQml3 = sourcePathCache.sourceId("/path/Qml.3");
    importDependencies.push_back(
        Storage::ImportDependency{"Qml", Storage::VersionNumber{3}, importSourceIdQml3, {}});
    importDependencies[1].dependencies.push_back(Storage::Import{"Qml"});

    storage.synchronize({importDependencies[1], importDependencies[3]},
                        {},
                        {},
                        {importSourceId2, importSourceIdQml3},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{3}, importSourceIdQml3),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                                     IsImport("Qml", Storage::VersionNumber{3})))),
                    AllOf(IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                                     IsImport("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsDependencyGetsOnlyTheHighestDependency)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    auto importSourceIdQml1 = sourcePathCache.sourceId("/path/Qml.1");
    importDependencies.push_back(
        Storage::ImportDependency{"Qml", Storage::VersionNumber{1}, importSourceIdQml1, {}});
    importDependencies[1].dependencies.push_back(Storage::Import{"Qml"});

    storage.synchronize({importDependencies[1], importDependencies[3]},
                        {},
                        {},
                        {importSourceId2, importSourceIdQml1},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{1}, importSourceIdQml1),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2})))),
                    AllOf(IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                                     IsImport("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeImportsDependencyRemoveDuplicateDependencies)
{
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    storage.synchronize(importDependencies,
                        {},
                        {},
                        {importSourceId1, importSourceId2, importSourceId5},
                        {});
    auto importSourceIdQml3 = sourcePathCache.sourceId("/path/Qml.3");
    importDependencies.push_back(
        Storage::ImportDependency{"Qml", Storage::VersionNumber{3}, importSourceIdQml3});
    importDependencies[2].dependencies.push_back(Storage::Import{"Qml", Storage::VersionNumber{3}});
    importDependencies[2].dependencies.push_back(Storage::Import{"Qml", Storage::VersionNumber{2}});
    importDependencies[2].dependencies.push_back(Storage::Import{"Qml", Storage::VersionNumber{3}});
    importDependencies[2].dependencies.push_back(Storage::Import{"Qml", Storage::VersionNumber{2}});

    storage.synchronize({importDependencies[2], importDependencies[3]},
                        {},
                        {},
                        {importSourceId5, importSourceIdQml3},
                        {});

    ASSERT_THAT(storage.fetchAllImports(),
                UnorderedElementsAre(
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{2}, importSourceId1),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("Qml", Storage::VersionNumber{3}, importSourceIdQml3),
                          Field(&Storage::ImportDependency::dependencies, IsEmpty())),
                    AllOf(IsImportDependency("QtQuick", Storage::VersionNumber{}, importSourceId2),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2})))),
                    AllOf(IsImportDependency("/path/to", Storage::VersionNumber{}, importSourceId5),
                          Field(&Storage::ImportDependency::dependencies,
                                UnorderedElementsAre(IsImport("Qml", Storage::VersionNumber{2}),
                                                     IsImport("Qml", Storage::VersionNumber{3}),
                                                     IsImport("QtQuick", Storage::VersionNumber{}))))));
}

TEST_F(ProjectStorageSlowTest, RemovingImportRemovesDependentTypesToo)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    Storage::ImportDependencies importDependencies{createImportDependencies()};
    importDependencies.pop_back();
    importDependencies.pop_back();
    storage.synchronize({}, {}, {}, {importSourceId2, importSourceId5}, {});

    ASSERT_THAT(storage.fetchTypes(),
                UnorderedElementsAre(AllOf(IsStorageType(Storage::Import{"Qml", 2},
                                                         "QObject",
                                                         Storage::NativeType{},
                                                         TypeAccessSemantics::Reference,
                                                         sourceId2),
                                           Field(&Storage::Type::exportedTypes,
                                                 UnorderedElementsAre(IsExportedType("Object"),
                                                                      IsExportedType("Obj"))))));
}

TEST_F(ProjectStorageSlowTest, FetchTypeIdByImportIdAndName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByName(importId1, "QObject");

    ASSERT_THAT(storage.fetchTypeIdByExportedName("Object"), Eq(typeId));
}

TEST_F(ProjectStorageSlowTest, FetchTypeIdByExportedName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByExportedName("Object");

    ASSERT_THAT(storage.fetchTypeIdByName(importId1, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorageSlowTest, FetchTypeIdByImporIdsAndExportedName)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByImportIdsAndExportedName({importId1, importId2}, "Object");

    ASSERT_THAT(storage.fetchTypeIdByName(importId1, "QObject"), Eq(typeId));
}

TEST_F(ProjectStorageSlowTest, FetchInvalidTypeIdByImporIdsAndExportedNameIfImportIdsAreEmpty)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByImportIdsAndExportedName({}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchInvalidTypeIdByImporIdsAndExportedNameIfImportIdsAreInvalid)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByImportIdsAndExportedName({ImportId{}}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchInvalidTypeIdByImporIdsAndExportedNameIfNotInImport)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto typeId = storage.fetchTypeIdByImportIdsAndExportedName({importId2, importId3}, "Object");

    ASSERT_FALSE(typeId.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchImportDepencencyIds)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto importIds = storage.fetchImportDependencyIds({importId3});

    ASSERT_THAT(importIds, UnorderedElementsAre(importId1, importId2, importId3));
}

TEST_F(ProjectStorageSlowTest, FetchImportDepencencyIdsForRootDepencency)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    auto importIds = storage.fetchImportDependencyIds({importId1});

    ASSERT_THAT(importIds, ElementsAre(importId1));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddAliasDeclarations)
{
    Storage::Types types{createTypesWithAliases()};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddAliasDeclarationsAgain)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemoveAliasDeclarations)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddAliasDeclarationsThrowsForWrongTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[1].typeName = Storage::NativeType{"QQuickItemWrong"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[2]}, {sourceId4}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}
TEST_F(ProjectStorageSlowTest, SynchronizeTypesAddAliasDeclarationsThrowsForWrongPropertyName)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[1].aliasPropertyName = "childrenWrong";

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId4}, {}),
                 QmlDesigner::PropertyNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangeAliasDeclarationsTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations[2].typeName = Storage::ExportedType{"Obj2"};

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangeAliasDeclarationsPropertyName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations[2].aliasPropertyName = "children";

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangeAliasDeclarationsToPropertyDeclaration)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations.pop_back();
    types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangePropertyDeclarationsToAliasDeclaration)
{
    Storage::Types types{createTypesWithAliases()};
    auto typesChanged = types;
    typesChanged[2].propertyDeclarations.pop_back();
    typesChanged[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize({}, {}, typesChanged, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangeAliasTargetPropertyDeclarationTraits)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[1].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                              | Storage::PropertyDeclarationTraits::IsReadOnly;

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesChangeAliasTargetPropertyDeclarationTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[1].propertyDeclarations[0].typeName = Storage::ExportedType{"Item"};

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovePropertyDeclarationWithAnAliasThrows)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[1].propertyDeclarations.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemovePropertyDeclarationAndAlias)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[1].propertyDeclarations.pop_back();
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, {types[1], types[2]}, {sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemoveTypeWithAliasTargetPropertyDeclarationThrows)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[2].typeName = Storage::ExportedType{"Object2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId4}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, SynchronizeTypesRemoveTypeAndAliasPropertyDeclaration)
{
    Storage::Types types{createTypesWithAliases()};
    types[2].propertyDeclarations[2].typeName = Storage::ExportedType{"Object2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[2].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, {types[0], types[2]}, {sourceId1, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, UpdateAliasPropertyIfPropertyIsOverloaded)
{
    Storage::Types types{createTypesWithAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, AliasPropertyIsOverloaded)
{
    Storage::Types types{createTypesWithAliases()};
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(
        storage.fetchTypes(),
        Contains(
            AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                "QAliasItem",
                                Storage::NativeType{"QQuickItem"},
                                TypeAccessSemantics::Reference,
                                sourceId3),
                  Field(&Storage::Type::propertyDeclarations,
                        UnorderedElementsAre(
                            IsPropertyDeclaration("items",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("objects",
                                                  Storage::NativeType{"QQuickItem"},
                                                  Storage::PropertyDeclarationTraits::IsList
                                                      | Storage::PropertyDeclarationTraits::IsReadOnly),
                            IsPropertyDeclaration("data",
                                                  Storage::NativeType{"QObject"},
                                                  Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, UpdateAliasPropertyIfOverloadedPropertyIsRemoved)
{
    Storage::Types types{createTypesWithAliases()};
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::NativeType{"QQuickItem"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[0].propertyDeclarations.pop_back();

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, RelinkAliasProperty)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[3].import = Storage::Import{"QtQuick"};

    storage.synchronize({}, {}, {types[3]}, {sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject2"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, DoNotRelinkAliasPropertyForExplicitExportedTypeName)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object2",
                                                                              Storage::Import{
                                                                                  "/path/to"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[3].import = Storage::Import{"QtQuick"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[3]}, {sourceId4}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest,
       DoRelinkAliasPropertyForExplicitExportedTypeNameEvenIfAnOtherSimilarTimeNameExists)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object2",
                                                                              Storage::Import{
                                                                                  "/path/to"}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QObject2",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId5,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QQuickItem"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject2"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, RelinkAliasPropertyReactToTypeNameChange)
{
    Storage::Types types{createTypesWithAliases2()};
    types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"items", Storage::ExportedType{"Item"}, "children"});
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types[0].typeName = "QQuickItem2";

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(
                    IsStorageType(Storage::Import{"QtQuick"},
                                  "QAliasItem",
                                  Storage::NativeType{"QObject"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3),
                    Field(&Storage::Type::propertyDeclarations,
                          UnorderedElementsAre(
                              IsPropertyDeclaration("items",
                                                    Storage::NativeType{"QQuickItem2"},
                                                    Storage::PropertyDeclarationTraits::IsList
                                                        | Storage::PropertyDeclarationTraits::IsReadOnly),
                              IsPropertyDeclaration("objects",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList),
                              IsPropertyDeclaration("data",
                                                    Storage::NativeType{"QObject"},
                                                    Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, DoNotRelinkAliasPropertyForDeletedType)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.erase(std::next(types.begin(), 2));
    types[2].import = Storage::Import{"QtQuick"};

    storage.synchronize({}, {}, {types[2]}, {sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                 "QAliasItem",
                                                 Storage::NativeType{"QQuickItem"},
                                                 TypeAccessSemantics::Reference,
                                                 sourceId3)))));
}

TEST_F(ProjectStorageSlowTest, DoNotRelinkAliasPropertyForDeletedTypeAndPropertyType)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.pop_back();
    types.pop_back();
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject"};

    storage.synchronize({}, {}, {types[1]}, {sourceId2, sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(), SizeIs(2));
}

TEST_F(ProjectStorageSlowTest, DoNotRelinkAliasPropertyForDeletedTypeAndPropertyTypeNameChange)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.erase(std::next(types.begin(), 2));
    types[2].import = Storage::Import{"QtQuick"};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject"};

    storage.synchronize({}, {}, {types[2]}, {sourceId3, sourceId4}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Not(Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                 "QAliasItem",
                                                 Storage::NativeType{"QQuickItem"},
                                                 TypeAccessSemantics::Reference,
                                                 sourceId3)))));
}

TEST_F(ProjectStorageSlowTest, DoNotRelinkPropertyTypeDoesNotExists)
{
    Storage::Types types{createTypesWithAliases()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.pop_back();

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId4}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, DoNotRelinkAliasPropertyTypeDoesNotExists)
{
    Storage::Types types{createTypesWithAliases2()};
    types[1].propertyDeclarations[0].typeName = Storage::NativeType{"QObject2"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4}, {});
    types.erase(types.begin());

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId1}, {}), QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ChangePrototypeTypeName)
{
    Storage::Types types{createTypesWithExportedTypeNamesOnly()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].typeName = "QObject3";

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject3"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorageSlowTest, ChangePrototypeTypeImportId)
{
    Storage::Types types{createTypes()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].import = Storage::Import{"QtQuick"};

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorageSlowTest, ChangeExplicitPrototypeTypeImportIdThows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"Qml"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].import = Storage::Import{"QtQuick"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ChangeExplicitPrototypeTypeImportId)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"Qml"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].import = Storage::Import{"QtQuick"};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"QtQuick"}};

    storage.synchronize({}, {}, {types[0], types[1]}, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorageSlowTest, ChangePrototypeTypeNameAndImportId)
{
    Storage::Types types{createTypesWithExportedTypeNamesOnly()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].import = Storage::Import{"QtQuick"};
    types[1].typeName = "QObject3";

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject3"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorageSlowTest, ChangePrototypeTypeNameThrowsForWrongNativePrototupeTypeName)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExportedType{"Object"};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].typeName = "QObject3";

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ThrowForPrototypeChainCycles)
{
    Storage::Types types{createTypes()};
    types[1].prototype = Storage::ExportedType{"Object2"};
    types.push_back(Storage::Type{Storage::Import{"/path/to"},
                                  "QObject2",
                                  Storage::ExportedType{"Item"},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object2"}, Storage::ExportedType{"Obj2"}}});

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorageSlowTest, ThrowForTypeIdAndPrototypeIdAreTheSame)
{
    Storage::Types types{createTypes()};
    types[1].prototype = Storage::ExportedType{"Object"};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorageSlowTest, ThrowForTypeIdAndPrototypeIdAreTheSameForRelinking)
{
    Storage::Types types{createTypesWithExportedTypeNamesOnly()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].prototype = Storage::ExportedType{"Item"};
    types[1].typeName = "QObject2";

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::PrototypeChainCycle);
}

TEST_F(ProjectStorageSlowTest, RecursiveAliases)
{
    Storage::Types types{createTypesWithRecursiveAliases()};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QAliasItem2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId5),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, RecursiveAliasesChangePropertyType)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[1].propertyDeclarations[0].typeName = Storage::ExportedType{"Object2"};

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QAliasItem2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId5),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         Storage::NativeType{"QObject2"},
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, UpdateAliasesAfterInjectingProperty)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[0].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ExportedType{"Item"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, {types[0]}, {sourceId1}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QAliasItem2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId5),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         Storage::NativeType{"QQuickItem"},
                                         Storage::PropertyDeclarationTraits::IsList
                                             | Storage::PropertyDeclarationTraits::IsReadOnly))))));
}

TEST_F(ProjectStorageSlowTest, UpdateAliasesAfterChangeAliasToProperty)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[2].propertyDeclarations.clear();
    types[2].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects",
                                     Storage::ExportedType{"Item"},
                                     Storage::PropertyDeclarationTraits::IsList
                                         | Storage::PropertyDeclarationTraits::IsReadOnly});

    storage.synchronize({}, {}, {types[2]}, {sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                AllOf(Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                   "QAliasItem2",
                                                   Storage::NativeType{"QObject"},
                                                   TypeAccessSemantics::Reference,
                                                   sourceId5),
                                     Field(&Storage::Type::propertyDeclarations,
                                           ElementsAre(IsPropertyDeclaration(
                                               "objects",
                                               Storage::NativeType{"QQuickItem"},
                                               Storage::PropertyDeclarationTraits::IsList
                                                   | Storage::PropertyDeclarationTraits::IsReadOnly,
                                               "objects"))))),
                      Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                                   "QAliasItem",
                                                   Storage::NativeType{"QQuickItem"},
                                                   TypeAccessSemantics::Reference,
                                                   sourceId3),
                                     Field(&Storage::Type::propertyDeclarations,
                                           ElementsAre(IsPropertyDeclaration(
                                               "objects",
                                               Storage::NativeType{"QQuickItem"},
                                               Storage::PropertyDeclarationTraits::IsList
                                                   | Storage::PropertyDeclarationTraits::IsReadOnly,
                                               "")))))));
}

TEST_F(ProjectStorageSlowTest, UpdateAliasesAfterChangePropertyToAlias)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    types[3].propertyDeclarations[0].traits = Storage::PropertyDeclarationTraits::IsList
                                              | Storage::PropertyDeclarationTraits::IsReadOnly;
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[1].propertyDeclarations.clear();
    types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ExportedType{"Object2"}, "objects"});

    storage.synchronize({}, {}, {types[1]}, {sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QAliasItem2",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId5),
                               Field(&Storage::Type::propertyDeclarations,
                                     ElementsAre(IsPropertyDeclaration(
                                         "objects",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList
                                             | Storage::PropertyDeclarationTraits::IsReadOnly,
                                         "objects"))))));
}

TEST_F(ProjectStorageSlowTest, CheckForProtoTypeCycleThrows)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    types[1].propertyDeclarations.clear();
    types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ExportedType{"AliasItem2"}, "objects"});

    ASSERT_THROW(storage.synchronize(
                     {}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorageSlowTest, CheckForProtoTypeCycleAfterUpdateThrows)
{
    Storage::Types types{createTypesWithRecursiveAliases()};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3, sourceId4, sourceId5}, {});
    types[1].propertyDeclarations.clear();
    types[1].propertyDeclarations.push_back(
        Storage::PropertyDeclaration{"objects", Storage::ExportedType{"AliasItem2"}, "objects"});

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::AliasChainCycle);
}

TEST_F(ProjectStorageSlowTest, ExplicitPrototype)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"Qml"}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorageSlowTest, ExplicitPrototypeUpperDownTheImportChainThrows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"QtQuick"}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ExplicitPrototypeUpperInTheImportChain)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"QtQuick"}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QQuickObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorageSlowTest, ExplicitPrototypeWithWrongVersionThrows)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"Qml", 4}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ExplicitPrototypeWithVersion)
{
    Storage::Types types{createTypes()};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"Qml", 2}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Import{"QtQuick"},
                                       "QQuickItem",
                                       Storage::NativeType{"QObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorageSlowTest, ExplicitPrototypeWithVersionInTheProtoTypeChain)
{
    Storage::Types types{createTypes()};
    auto importDependencyQtQuick2 = Storage::ImportDependency{
        "QtQuick",
        Storage::VersionNumber{2},
        importSourceId2,
        {Storage::Import{"Qml", Storage::VersionNumber{2}}}};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"QtQuick", 2}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({importDependencyQtQuick2},
                        {},
                        types,
                        {sourceId1, sourceId2, sourceId3, importSourceId2},
                        {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(IsStorageType(Storage::Import{"QtQuick", 2},
                                       "QQuickItem",
                                       Storage::NativeType{"QQuickObject"},
                                       TypeAccessSemantics::Reference,
                                       sourceId1)));
}

TEST_F(ProjectStorageSlowTest, ExplicitPrototypeWithVersionDownTheProtoTypeChainThrows)
{
    Storage::Types types{createTypes()};
    auto importDependencyQtQuick2 = Storage::ImportDependency{
        "QtQuick",
        Storage::VersionNumber{2},
        importSourceId2,
        {Storage::Import{"Qml", Storage::VersionNumber{2}}}};
    types[0].prototype = Storage::ExplicitExportedType{"Object", Storage::Import{"QtQuick", 2}};

    ASSERT_THROW(storage.synchronize({importDependencyQtQuick2},
                                     {},
                                     types,
                                     {sourceId1, sourceId2, sourceId3, importSourceId2},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ExplicitPropertyDeclarationTypeName)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Import{
                                                                                  "Qml"}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorageSlowTest, ExplicitPropertyDeclarationTypeNameDownTheImportChainThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Import{
                                                                                  "QtQuick"}};

    ASSERT_THROW(storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ExplicitPropertyDeclarationTypeNameInTheImportChain)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Import{
                                                                                  "QtQuick"}};
    types.push_back(Storage::Type{Storage::Import{"QtQuick"},
                                  "QQuickObject",
                                  Storage::NativeType{},
                                  TypeAccessSemantics::Reference,
                                  sourceId3,
                                  {Storage::ExportedType{"Object"}}});

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QQuickObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorageSlowTest, ExplicitPropertyDeclarationTypeNameWithVersion)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Import{"Qml",
                                                                                              2}};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2, sourceId3}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(
                    Field(&Storage::Type::propertyDeclarations,
                          Contains(IsPropertyDeclaration("data",
                                                         Storage::NativeType{"QObject"},
                                                         Storage::PropertyDeclarationTraits::IsList)))));
}

TEST_F(ProjectStorageSlowTest, PrototypeWithVersionDownTheProtoTypeChainThrows)
{
    Storage::Types types{createTypes()};
    auto importDependencyQtQuick2 = Storage::ImportDependency{
        "QtQuick",
        Storage::VersionNumber{2},
        importSourceId2,
        {Storage::Import{"Qml", Storage::VersionNumber{2}}}};
    types[0].prototype = Storage::ExportedType{"Object"};
    auto document = Storage::Document{sourceId1, {Storage::Import{"QtQuick"}}};

    ASSERT_THROW(storage.synchronize({importDependencyQtQuick2},
                                     {document},
                                     types,
                                     {sourceId1, sourceId2, sourceId3, importSourceId2},
                                     {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ChangePropertyTypeImportIdWithExplicitTypeThrows)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Import{
                                                                                  "Qml"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[1].import = Storage::Import{"QtQuick"};

    ASSERT_THROW(storage.synchronize({}, {}, {types[1]}, {sourceId2}, {}),
                 QmlDesigner::TypeNameDoesNotExists);
}

TEST_F(ProjectStorageSlowTest, ChangePropertyTypeImportIdWithExplicitType)
{
    Storage::Types types{createTypes()};
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Import{
                                                                                  "Qml"}};
    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});
    types[0].propertyDeclarations[0].typeName = Storage::ExplicitExportedType{"Object",
                                                                              Storage::Import{
                                                                                  "QtQuick"}};
    types[1].import = Storage::Import{"QtQuick"};

    storage.synchronize({}, {}, types, {sourceId1, sourceId2}, {});

    ASSERT_THAT(storage.fetchTypes(),
                Contains(AllOf(IsStorageType(Storage::Import{"QtQuick"},
                                             "QQuickItem",
                                             Storage::NativeType{"QObject"},
                                             TypeAccessSemantics::Reference,
                                             sourceId1),
                               Field(&Storage::Type::propertyDeclarations,
                                     Contains(IsPropertyDeclaration(
                                         "data",
                                         Storage::NativeType{"QObject"},
                                         Storage::PropertyDeclarationTraits::IsList))))));
}

TEST_F(ProjectStorageSlowTest, AddFileStatuses)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};

    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorageSlowTest, RemoveFileStatus)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()), UnorderedElementsAre(fileStatus1));
}

TEST_F(ProjectStorageSlowTest, UpdateFileStatus)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    FileStatus fileStatus2b{sourceId2, 102, 102};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2b});

    ASSERT_THAT(convert(storage.fetchAllFileStatuses()),
                UnorderedElementsAre(fileStatus1, fileStatus2b));
}

TEST_F(ProjectStorageSlowTest, ThrowForInvalidSourceId)
{
    setUpSourceIds();
    FileStatus fileStatus1{SourceId{}, 100, 100};

    ASSERT_THROW(storage.synchronize({}, {}, {}, {sourceId1}, {fileStatus1}),
                 Sqlite::ConstraintPreventsModification);
}

TEST_F(ProjectStorageSlowTest, FetchAllFileStatuses)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus1, fileStatus2});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

TEST_F(ProjectStorageSlowTest, FetchAllFileStatusesReverse)
{
    setUpSourceIds();
    FileStatus fileStatus1{sourceId1, 100, 100};
    FileStatus fileStatus2{sourceId2, 101, 101};
    storage.synchronize({}, {}, {}, {sourceId1, sourceId2}, {fileStatus2, fileStatus1});

    auto fileStatuses = convert(storage.fetchAllFileStatuses());

    ASSERT_THAT(fileStatuses, ElementsAre(fileStatus1, fileStatus2));
}

} // namespace
