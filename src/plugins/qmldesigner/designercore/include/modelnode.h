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

#include "qmldesignercorelib_global.h"
#include <QPointer>
#include <QList>
#include <QVector>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {
    class InternalNode;
    class ModelPrivate;
    class InternalNode;
    class InternalProperty;

    using InternalNodePointer = QSharedPointer<InternalNode>;
    using InternalPropertyPointer = QSharedPointer<InternalProperty>;
}
class NodeMetaInfo;
class AbstractProperty;
class BindingProperty;
class VariantProperty;
class SignalHandlerProperty;
class Model;
class AbstractView;
class NodeListProperty;
class NodeProperty;
class NodeAbstractProperty;
class ModelNode;
class GlobalAnnotationStatus;
class Comment;
class Annotation;

QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);

using PropertyListType = QList<QPair<PropertyName, QVariant> >;

static const PropertyName lockedProperty = {("locked")};

class QMLDESIGNERCORE_EXPORT ModelNode
{
    friend QMLDESIGNERCORE_EXPORT bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT uint qHash(const ModelNode & node);
    friend QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
    friend QMLDESIGNERCORE_EXPORT bool operator <(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);
    friend Model;
    friend AbstractView;
    friend NodeListProperty;
    friend Internal::ModelPrivate;
    friend NodeAbstractProperty;
    friend NodeProperty;

public:
    enum NodeSourceType {
        NodeWithoutSource = 0,
        NodeWithCustomParserSource = 1,
        NodeWithComponentSource = 2
    };

    ModelNode();
    ModelNode(const Internal::InternalNodePointer &internalNode, Model *model, const AbstractView *view);
    ModelNode(const ModelNode &modelNode, AbstractView *view);
    ModelNode(const ModelNode &other);
    ModelNode(ModelNode &&other);
    ~ModelNode();

    ModelNode &operator=(const ModelNode &other);
    ModelNode &operator=(ModelNode &&other);

    TypeName type() const;
    QString simplifiedTypeName() const;
    QString displayName() const;
    int minorVersion() const;
    int majorVersion() const;

    bool isValid() const;
    bool isInHierarchy() const;


    NodeAbstractProperty parentProperty() const;
    void setParentProperty(NodeAbstractProperty parent);
    void changeType(const TypeName &typeName, int majorVersion, int minorVersion);
    void setParentProperty(const ModelNode &newParentNode, const PropertyName &propertyName);
    bool hasParentProperty() const;

    QList<ModelNode> directSubModelNodes() const;
    QList<ModelNode> directSubModelNodesOfType(const TypeName &typeName) const;
    QList<ModelNode> subModelNodesOfType(const TypeName &typeName) const;

    QList<ModelNode> allSubModelNodes() const;
    QList<ModelNode> allSubModelNodesAndThisNode() const;
    bool hasAnySubModelNodes() const;

    //###

    AbstractProperty property(const PropertyName &name) const;
    VariantProperty variantProperty(const PropertyName &name) const;
    BindingProperty bindingProperty(const PropertyName &name) const;
    SignalHandlerProperty signalHandlerProperty(const PropertyName &name) const;
    NodeListProperty nodeListProperty(const PropertyName &name) const;
    NodeProperty nodeProperty(const PropertyName &name) const;
    NodeAbstractProperty nodeAbstractProperty(const PropertyName &name) const;
    NodeAbstractProperty defaultNodeAbstractProperty() const;
    NodeListProperty defaultNodeListProperty() const;
    NodeProperty defaultNodeProperty() const;

    void removeProperty(const PropertyName &name) const; //### also implement in AbstractProperty
    QList<AbstractProperty> properties() const;
    QList<VariantProperty> variantProperties() const;
    QList<NodeAbstractProperty> nodeAbstractProperties() const;
    QList<NodeProperty> nodeProperties() const;
    QList<NodeListProperty> nodeListProperties() const;
    QList<BindingProperty> bindingProperties() const;
    QList<SignalHandlerProperty> signalProperties() const;
    PropertyNameList propertyNames() const;

    bool hasProperties() const;
    bool hasProperty(const PropertyName &name) const;
    bool hasVariantProperty(const PropertyName &name) const;
    bool hasBindingProperty(const PropertyName &name) const;
    bool hasNodeAbstractProperty(const PropertyName &name) const;
    bool hasDefaultNodeAbstractProperty() const;
    bool hasDefaultNodeListProperty() const;
    bool hasDefaultNodeProperty() const;
    bool hasNodeProperty(const PropertyName &name) const;
    bool hasNodeListProperty(const PropertyName &name) const;

    void setScriptFunctions(const QStringList &scriptFunctionList);
    QStringList scriptFunctions() const;

    //###
    void destroy();

    QString id() const;
    QString validId();
    void setIdWithRefactoring(const QString &id);
    void setIdWithoutRefactoring(const QString &id);
    static bool isValidId(const QString &id);
    bool hasId() const;

    Model *model() const;
    AbstractView *view() const;

    const NodeMetaInfo metaInfo() const;
    bool hasMetaInfo() const;

    bool isSelected() const;
    bool isRootNode() const;

    bool isAncestorOf(const ModelNode &node) const;
    void selectNode();
    void deselectNode();

    static int variantUserType();
    QVariant toVariant() const;

    QVariant auxiliaryData(const PropertyName &name) const;
    void setAuxiliaryData(const PropertyName &name, const QVariant &data) const;
    void removeAuxiliaryData(const PropertyName &name) const;
    bool hasAuxiliaryData(const PropertyName &name) const;
    const QHash<PropertyName, QVariant> &auxiliaryData() const;

    QString customId() const;
    bool hasCustomId() const;
    void setCustomId(const QString &str);
    void removeCustomId();

    QVector<Comment> comments() const;
    bool hasComments() const;
    void setComments(const QVector<Comment> &coms);
    void addComment(const Comment &com);
    bool updateComment(const Comment &com, int position);

    Annotation annotation() const;
    bool hasAnnotation() const;
    void setAnnotation(const Annotation &annotation);
    void removeAnnotation();

    Annotation globalAnnotation() const;
    bool hasGlobalAnnotation() const;
    void setGlobalAnnotation(const Annotation &annotation);
    void removeGlobalAnnotation();

    GlobalAnnotationStatus globalStatus() const;
    bool hasGlobalStatus() const;
    void setGlobalStatus(const GlobalAnnotationStatus &status);
    void removeGlobalStatus();

    bool locked() const;
    void setLocked(bool value);

    static bool isThisOrAncestorLocked(const ModelNode &node);

    qint32 internalId() const;

    void setNodeSource(const QString&);
    void setNodeSource(const QString &newNodeSource, NodeSourceType type);
    QString nodeSource() const;

    QString convertTypeToImportAlias() const;

    NodeSourceType nodeSourceType() const;

    bool isComponent() const;
    bool isSubclassOf(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1) const;
    QIcon typeIcon() const;

    friend void swap(ModelNode &first, ModelNode &second) noexcept
    {
        using std::swap;

        swap(first.m_internalNode, second.m_internalNode);
        swap(first.m_model, second.m_model);
        swap(first.m_view, second.m_view);
    }

private: // functions
    Internal::InternalNodePointer internalNode() const;

    bool hasLocked() const;

private: // variables
    Internal::InternalNodePointer m_internalNode;
    QPointer<Model> m_model;
    QPointer<AbstractView> m_view;
};

QMLDESIGNERCORE_EXPORT bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT uint qHash(const ModelNode & node);
QMLDESIGNERCORE_EXPORT bool operator <(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const ModelNode &modelNode);
}

Q_DECLARE_METATYPE(QmlDesigner::ModelNode)
