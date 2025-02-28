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

#include "modelnode.h"
#include <abstractproperty.h>
#include <abstractview.h>
#include <model.h>
#include <nodemetainfo.h>
#include "internalnode_p.h"
#include "invalidargumentexception.h"
#include "invalididexception.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "model_p.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "signalhandlerproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include <rewriterview.h>
#include "annotation.h"

#include <utils/algorithm.h>

#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

namespace QmlDesigner {
using namespace QmlDesigner::Internal;

/*!
\class QmlDesigner::ModelNode
\ingroup CoreModel
\brief The central class to access the node which can represent a widget, layout
            or other items. A Node is a part of a tree and has properties.

Conceptually ModelNode is an opaque handle to the internal data structures.

There is always a root model node in every QmlDesigner::Model:
\code
QmlDesigner::Model *model = QmlDesigner::Model::create();
QmlDesigner::ModelNode rootNode = model->rootNode();
\endcode

You can add a property to a node:
\code
childNode.addProperty("pos", QPoint(2, 12));
\endcode

All the manipulation functions are generating undo commands internally.
*/



/*! \brief internal constructor

*/
ModelNode::ModelNode(const InternalNodePointer &internalNode, Model *model, const AbstractView *view):
        m_internalNode(internalNode),
        m_model(model),
        m_view(const_cast<AbstractView*>(view))
{
    Q_ASSERT(!m_model || m_view);
}

ModelNode::ModelNode(const ModelNode &modelNode, AbstractView *view)
    : m_internalNode(modelNode.m_internalNode),
      m_model(modelNode.model()),
      m_view(view)
{
}

ModelNode::ModelNode(ModelNode &&other)
    : m_internalNode(std::move(other.m_internalNode))
    , m_model(std::move(other.m_model))
    , m_view(std::move(other.m_view))
{
    other.m_model = {};
    other.m_view = {};
}

ModelNode &ModelNode::operator=(ModelNode &&other)
{
    ModelNode newNode = std::move(other);

    swap(*this, newNode);

    return *this;
}

/*! \brief contructs a invalid model node
\return invalid node
\see invalid
*/
ModelNode::ModelNode():
        m_internalNode(new InternalNode)
{
}

ModelNode::ModelNode(const ModelNode &other) = default;

ModelNode& ModelNode::operator=(const ModelNode &other) = default;

/*! \brief does nothing
*/
ModelNode::~ModelNode() = default;

/*! \brief returns the name of node which is a short cut to a property like objectName
\return name of the node
*/
QString ModelNode::id() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return m_internalNode->id();
}

QString ModelNode::validId()
{
    if (id().isEmpty())
        setIdWithRefactoring(view()->generateNewId(simplifiedTypeName()));

    return id();
}

static bool idIsQmlKeyWord(const QString& id)
{
    static const QSet<QString> keywords = {"as",         "break",    "case",    "catch",
                                           "continue",   "debugger", "default", "delete",
                                           "do",         "else",     "finally", "for",
                                           "function",   "if",       "import",  "in",
                                           "instanceof", "new",      "print",   "return",
                                           "switch",     "this",     "throw",   "try",
                                           "typeof",     "var",      "void",    "while",
                                           "with"};

    return keywords.contains(id);
}

static bool isIdToAvoid(const QString& id)
{
    static const QSet<QString> ids = {
        "top",
        "bottom",
        "left",
        "right",
        "width",
        "height",
        "x",
        "y",
        "opacity",
        "parent",
        "item",
        "flow",
        "color",
        "margin",
        "padding",
        "border",
        "font",
        "text",
        "source",
        "state",
        "visible",
        "focus",
        "data",
        "clip",
        "layer",
        "scale",
        "enabled",
        "anchors",
        "texture",
        "shaderInfo"
    };

    return ids.contains(id);
}

static bool idContainsWrongLetter(const QString& id)
{
    static QRegularExpression idExpr(QStringLiteral("^[a-z_][a-zA-Z0-9_]*$"));
    return !id.contains(idExpr);
}

bool ModelNode::isValidId(const QString &id)
{
    return id.isEmpty() || (!idContainsWrongLetter(id) && !idIsQmlKeyWord(id) && !isIdToAvoid(id));
}

bool ModelNode::hasId() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return m_internalNode->hasId();
}

void ModelNode::setIdWithRefactoring(const QString& id)
{
    if (model()->rewriterView()
            && !id.isEmpty()
            && !m_internalNode->id().isEmpty()) { // refactor the id if they are not empty
        model()->rewriterView()->renameId(m_internalNode->id(), id);
    } else {
        setIdWithoutRefactoring(id);
    }
}

void ModelNode::setIdWithoutRefactoring(const QString &id)
{
    Internal::WriteLocker locker(m_model.data());
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (!isValidId(id))
        throw InvalidIdException(__LINE__, __FUNCTION__, __FILE__, id.toUtf8(), InvalidIdException::InvalidCharacters);

    if (id == m_internalNode->id())
        return;

    if (view()->hasId(id))
        throw InvalidIdException(__LINE__, __FUNCTION__, __FILE__, id.toUtf8(), InvalidIdException::DuplicateId);

    m_model.data()->d->changeNodeId(internalNode(), id);
}

/*! \brief the fully-qualified type name of the node is represented as string
\return type of the node as a string
*/
TypeName ModelNode::type() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return m_internalNode->type();
}

/*! \brief minor number of the QML type
\return minor number
*/
int ModelNode::minorVersion() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return m_internalNode->minorVersion();
}

/*! \brief major number of the QML type
\return major number
*/
int ModelNode::majorVersion() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return m_internalNode->majorVersion();
}

/*! \return the short-hand type name of the node. */
QString ModelNode::simplifiedTypeName() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return QString::fromUtf8(type().split('.').constLast());
}

QString ModelNode::displayName() const
{
    if (hasId())
        return id();
    return simplifiedTypeName();
}

/*! \brief Returns whether the node is valid

A node is valid if its model still exists, and contains this node.
Also, the current state must be a valid one.

A node might become invalid if e.g. it or one of its ancestors is deleted.

\return is a node valid(true) or invalid(false)
*/
bool ModelNode::isValid() const
{
    return !m_model.isNull() && !m_view.isNull() && m_internalNode && m_internalNode->isValid();
}

/*!
  \brief Returns whether the root node of the model is one of the anchestors of this node.

  Will return true also for the root node itself.
  */
bool ModelNode::isInHierarchy() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    if (isRootNode())
        return true;
    if (!hasParentProperty())
        return false;
    return parentProperty().parentModelNode().isInHierarchy();
}

/*!
  \brief Returns the property containing this node

  The NodeAbstractProperty is invalid if this ModelNode has no parent.
  NodeAbstractProperty can be a NodeProperty containing a single ModelNode, or
  a NodeListProperty.

  \return the property containing this ModelNode
  */
NodeAbstractProperty ModelNode::parentProperty() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (m_internalNode->parentProperty().isNull()) {
        Q_ASSERT_X(m_internalNode->parentProperty(), Q_FUNC_INFO, "parentProperty is invalid");
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "parent");
    }

    return NodeAbstractProperty(m_internalNode->parentProperty()->name(), m_internalNode->parentProperty()->propertyOwner(), m_model.data(), view());
}


/*! \brief the command id is used to compress the some commands together.
\param newParentNode parent of this node will be set to this node
\param commandId integer which is used to descripe commands which should compressed together to one command

For example:
\code
node.setParentNode(parentNode1);
node.setParentNode(parentNode2, 212);
node.setParentNode(parentNode3, 212);
model->undoStack()->undo();
ModelNode parentNode4 = node.parentProperty().parentModelNode();
parentNode4 == parentNode1; -> true
\endcode

\see parentNode childNodes hasChildNodes Model::undo

*/

void ModelNode::setParentProperty(NodeAbstractProperty parent)
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (!parent.parentModelNode().isValid())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "newParentNode");

    if (*this == parent.parentModelNode()) {
        Q_ASSERT_X(*this != parent.parentModelNode(), Q_FUNC_INFO, "cannot set parent to itself");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (hasParentProperty() && parent == parentProperty())
        return;

    parent.reparentHere(*this);
}

void ModelNode::changeType(const TypeName &typeName, int majorVersion, int minorVersion)
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    model()->d->changeNodeType(internalNode(), typeName, majorVersion, minorVersion);

}

void ModelNode::setParentProperty(const ModelNode &newParentNode, const PropertyName &propertyName)
{
    setParentProperty(newParentNode.nodeAbstractProperty(propertyName));
}

/*! \brief test if there is a parent for this node
\return true is this node has a parent
\see childNodes parentNode setParentNode hasChildNodes Model::undo
*/
bool ModelNode::hasParentProperty() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (m_internalNode->parentProperty().isNull())
        return false;

    return true;
}

/*!
  \brief Returns a BindingProperty

  Note that a valid BindingProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a BindingProperty.
  Assigning an expression to this BindingProperty will create the property.

  \return BindingProperty named name
  */

BindingProperty ModelNode::bindingProperty(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return BindingProperty(name, m_internalNode, model(), view());
}

SignalHandlerProperty ModelNode::signalHandlerProperty(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return SignalHandlerProperty(name, m_internalNode, model(), view());
}


/*!
  \brief Returns a NodeProperty

  Note that a valid NodeProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a NodeProperty.
  Assigning a ModelNode to this NodeProperty will create the property.

  \return NodeProperty named name
  */

NodeProperty ModelNode::nodeProperty(const PropertyName &name) const
{
      if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return NodeProperty(name, m_internalNode, model(), view());
}


/*!
  \brief Returns a NodeListProperty

  Note that a valid NodeListProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a NodeListProperty.
  Assigning a ModelNode to this NodeListProperty will create the property.

  \return NodeListProperty named name
  */

NodeListProperty ModelNode::nodeListProperty(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return NodeListProperty(name, m_internalNode, model(), view());
}

NodeAbstractProperty ModelNode::nodeAbstractProperty(const PropertyName &name) const
{
     if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

     return NodeAbstractProperty(name, m_internalNode, model(), view());
}

NodeAbstractProperty ModelNode::defaultNodeAbstractProperty() const
{
    return nodeAbstractProperty(metaInfo().defaultPropertyName());
}

NodeListProperty ModelNode::defaultNodeListProperty() const
{
    return nodeListProperty(metaInfo().defaultPropertyName());
}

NodeProperty ModelNode::defaultNodeProperty() const
{
    return nodeProperty(metaInfo().defaultPropertyName());
}

/*!
  \brief Returns a VariantProperty

  Note that a valid VariantProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a VariantProperty.
  Assigning a value to this VariantProperty will create the property.

  \return VariantProperty named name
  */

VariantProperty ModelNode::variantProperty(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return VariantProperty(name, m_internalNode, model(), view());
}

AbstractProperty ModelNode::property(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return AbstractProperty(name, m_internalNode, model(), view());
}

/*! \brief returns a property
\param name name of the property
\return returns a node property handle. If the property is not set yet, the node property is still valid (lazy reference).

It is searching only in the local Property.

\see addProperty changePropertyValue removeProperty properties hasProperties
*/

/*! \brief returns a list of all properties
\return list of all properties

The list of properties

*/
QList<AbstractProperty> ModelNode::properties() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QList<AbstractProperty> propertyList;

    foreach (const PropertyName &propertyName, internalNode()->propertyNameList()) {
        AbstractProperty property(propertyName, internalNode(), model(), view());
        propertyList.append(property);
    }

    return propertyList;
}


/*! \brief returns a list of all VariantProperties
\return list of all VariantProperties

The list of all properties containing just an atomic value.

*/
QList<VariantProperty> ModelNode::variantProperties() const
{
    QList<VariantProperty> propertyList;

    foreach (const AbstractProperty &abstractProperty, properties())
        if (abstractProperty.isVariantProperty())
            propertyList.append(abstractProperty.toVariantProperty());
    return propertyList;
}

QList<NodeAbstractProperty> ModelNode::nodeAbstractProperties() const
{
    QList<NodeAbstractProperty> propertyList;

    foreach (const AbstractProperty &nodeAbstractProperty, properties())
        if (nodeAbstractProperty.isNodeAbstractProperty())
            propertyList.append(nodeAbstractProperty.toNodeAbstractProperty());
    return propertyList;
}

QList<NodeProperty> ModelNode::nodeProperties() const
{
    QList<NodeProperty> propertyList;

    foreach (const AbstractProperty &nodeProperty, properties())
        if (nodeProperty.isNodeProperty())
            propertyList.append(nodeProperty.toNodeProperty());
    return propertyList;
}

QList<NodeListProperty> ModelNode::nodeListProperties() const
{
    QList<NodeListProperty> propertyList;

    foreach (const AbstractProperty &nodeListProperty, properties())
        if (nodeListProperty.isNodeListProperty())
            propertyList.append(nodeListProperty.toNodeListProperty());
    return propertyList;
}


/*! \brief returns a list of all BindingProperties
\return list of all BindingProperties

The list of all properties containing an expression.

*/
QList<BindingProperty> ModelNode::bindingProperties() const
{
    QList<BindingProperty> propertyList;

    foreach (const AbstractProperty &bindingProperty, properties())
        if (bindingProperty.isBindingProperty())
            propertyList.append(bindingProperty.toBindingProperty());
    return propertyList;
}

QList<SignalHandlerProperty> ModelNode::signalProperties() const
{
    QList<SignalHandlerProperty> propertyList;

    foreach (const AbstractProperty &property, properties())
        if (property.isSignalHandlerProperty())
            propertyList.append(property.toSignalHandlerProperty());
    return propertyList;
}

/*!
\brief removes a property from this node
\param name name of the property

Does nothing if the node state does not set this property.

\see addProperty property  properties hasProperties
*/
void ModelNode::removeProperty(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    model()->d->checkPropertyName(name);

    if (internalNode()->hasProperty(name))
        model()->d->removeProperty(internalNode()->property(name));
}

/*! \brief removes this node from the node tree
*/
static QList<ModelNode> descendantNodes(const ModelNode &node)
{
    const QList<ModelNode> children = node.directSubModelNodes();
    QList<ModelNode> descendants = children;
    for (const ModelNode &child : children)
        descendants += descendantNodes(child);

    return descendants;
}

static void removeModelNodeFromSelection(const ModelNode &node)
{
    // remove nodes from the active selection
    QList<ModelNode> selectedList = node.view()->selectedModelNodes();

    const QList<ModelNode> descendants = descendantNodes(node);
    for (const ModelNode &descendantNode : descendants)
        selectedList.removeAll(descendantNode);

    selectedList.removeAll(node);

    node.view()->setSelectedModelNodes(selectedList);
}

/*! \brief complete removes this ModelNode from the Model
*/
void ModelNode::destroy()
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (isRootNode())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "rootNode");

    removeModelNodeFromSelection(*this);
    model()->d->removeNode(internalNode());
}
//\}

/*! \name Property Manipulation
 *  This functions interact with properties.
 */


/*!
  \brief Returns if the two nodes reference the same entity in the same model
  */
bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode)
{
    return firstNode.internalId() == secondNode.internalId();
}

/*!
  \brief Returns if the two nodes do not reference the same entity in the same model
  */
bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode)
{
    return firstNode.internalId() != secondNode.internalId();
}

bool operator <(const ModelNode &firstNode, const ModelNode &secondNode)
{
    return firstNode.internalId() < secondNode.internalId();
}


Internal::InternalNodePointer ModelNode::internalNode() const
{
    return m_internalNode;
}


uint qHash(const ModelNode &node)
{
    return ::qHash(node.internalId());
}

/*!
\brief returns the model of the node
\return returns the model of the node
*/
Model *ModelNode::model() const
{
    return m_model.data();
}

/*!
\brief returns the view of the node
Each ModelNode belongs to one specific view.
\return view of the node
*/
AbstractView *ModelNode::view() const
{
    return m_view.data();
}


/*!
\brief returns all ModelNodes that are direct children of this ModelNode
The list contains every ModelNode that belongs to one of this ModelNodes
properties.
\return a list of all ModelNodes that are direct children
*/
QList<ModelNode> ModelNode::directSubModelNodes() const
{
    return toModelNodeList(internalNode()->allDirectSubNodes(), view());
}

QList<ModelNode> ModelNode::directSubModelNodesOfType(const TypeName &typeName) const
{
    return Utils::filtered(directSubModelNodes(), [typeName](const ModelNode &node){
        return node.metaInfo().isValid() && node.metaInfo().isSubclassOf(typeName);
    });
}

QList<ModelNode> ModelNode::subModelNodesOfType(const TypeName &typeName) const
{
    return Utils::filtered(allSubModelNodes(), [typeName](const ModelNode &node){
        return node.metaInfo().isValid() && node.metaInfo().isSubclassOf(typeName);
    });
}

/*!
\brief returns all ModelNodes that are direct or indirect children of this ModelNode
The list contains every ModelNode that is a direct or indirect child of this ModelNode.
All children in this list will be implicitly removed if this ModelNode is destroyed.
\return a list of all ModelNodes that are direct or indirect children
*/

QList<ModelNode> ModelNode::allSubModelNodes() const
{
    return toModelNodeList(internalNode()->allSubNodes(), view());
}

QList<ModelNode> ModelNode::allSubModelNodesAndThisNode() const
{
    QList<ModelNode> modelNodeList;
    modelNodeList.append(*this);
    modelNodeList.append(allSubModelNodes());

    return modelNodeList;
}

/*!
\brief returns if this ModelNode has any child ModelNodes.

\return if this ModelNode has any child ModelNodes
*/

bool ModelNode::hasAnySubModelNodes() const
{
    return !nodeAbstractProperties().isEmpty();
}

const NodeMetaInfo ModelNode::metaInfo() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return NodeMetaInfo(model()->metaInfoProxyModel(), type(), majorVersion(), minorVersion());
}

bool ModelNode::hasMetaInfo() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return model()->hasNodeMetaInfo(type(), majorVersion(), minorVersion());
}

/*! \brief has a node the selection of the model
\return true if the node his selection
*/
bool ModelNode::isSelected() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return view()->selectedModelNodes().contains(ModelNode(m_internalNode, m_model.data(), view()));
}

/*! \briefis this node the root node of the model
\return true if it is the root node
*/
bool ModelNode::isRootNode() const
{
    if (!isValid())
        return false;

    return view()->rootModelNode() == *this;
}

/*! \brief returns the list of all property names
\return list of all property names set in this state.

The list of properties set in this state.

\see addProperty property changePropertyValue removeProperty hasProperties
*/
PropertyNameList ModelNode::propertyNames() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    return internalNode()->propertyNameList();
}

/*! \brief test a if a property is set for this node
\return true if property a property ins this or a ancestor state exists
*/
bool ModelNode::hasProperty(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return internalNode()->hasProperty(name);
}

bool ModelNode::hasVariantProperty(const PropertyName &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isVariantProperty();
}

bool ModelNode::hasBindingProperty(const PropertyName &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isBindingProperty();
}

bool ModelNode::hasNodeAbstractProperty(const PropertyName &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isNodeAbstractProperty();
}

bool ModelNode::hasDefaultNodeAbstractProperty() const
{
    return hasProperty(metaInfo().defaultPropertyName()) && internalNode()->property(metaInfo().defaultPropertyName())->isNodeAbstractProperty();
}

bool ModelNode::hasDefaultNodeListProperty() const
{
    return hasProperty(metaInfo().defaultPropertyName()) && internalNode()->property(metaInfo().defaultPropertyName())->isNodeListProperty();
}

bool ModelNode::hasDefaultNodeProperty() const
{
    return hasProperty(metaInfo().defaultPropertyName()) && internalNode()->property(metaInfo().defaultPropertyName())->isNodeProperty();
}

bool ModelNode::hasNodeProperty(const PropertyName &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isNodeProperty();
}

bool ModelNode::hasNodeListProperty(const PropertyName &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isNodeListProperty();
}

static bool recursiveAncestor(const ModelNode &possibleAncestor, const ModelNode &node)
{
    if (!node.isValid())
        return false;

    if (node.hasParentProperty()) {
        if (node.parentProperty().parentModelNode() == possibleAncestor)
           return true;
        return recursiveAncestor(possibleAncestor, node.parentProperty().parentModelNode());
    }

    return false;
}

bool ModelNode::isAncestorOf(const ModelNode &node) const
{
    return recursiveAncestor(*this, node);
}

QDebug operator<<(QDebug debug, const ModelNode &modelNode)
{
    if (modelNode.isValid()) {
        debug.nospace() << "ModelNode("
                << modelNode.internalId() << ", "
                << modelNode.type() << ", "
                << modelNode.id() << ')';
    } else {
        debug.nospace() << "ModelNode(invalid)";
    }

    return debug.space();
}

QTextStream& operator<<(QTextStream &stream, const ModelNode &modelNode)
{
    if (modelNode.isValid()) {
        stream << "ModelNode("
                << "type: " << modelNode.type() << ", "
                << "id: " << modelNode.id() << ')';
    } else {
        stream << "ModelNode(invalid)";
    }

    return stream;
}

void ModelNode::selectNode()
{
    if (!isValid())
            throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QList<ModelNode> selectedNodeList;
    selectedNodeList.append(*this);

    view()->setSelectedModelNodes(selectedNodeList);
}

void ModelNode::deselectNode()
{
    if (!isValid())
            throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QList<ModelNode> selectedNodeList(view()->selectedModelNodes());
    selectedNodeList.removeAll(*this);

    view()->setSelectedModelNodes(selectedNodeList);
}

int ModelNode::variantUserType()
{
    return qMetaTypeId<ModelNode>();
}

QVariant ModelNode::toVariant() const
{
    return QVariant::fromValue(*this);
}

QVariant ModelNode::auxiliaryData(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return internalNode()->auxiliaryData(name);
}

void ModelNode::setAuxiliaryData(const PropertyName &name, const QVariant &data) const
{
    Internal::WriteLocker locker(m_model.data());
    m_model.data()->d->setAuxiliaryData(internalNode(), name, data);
}

void ModelNode::removeAuxiliaryData(const PropertyName &name) const
{
    Internal::WriteLocker locker(m_model.data());
    m_model.data()->d->removeAuxiliaryData(internalNode(), name);
}

bool ModelNode::hasAuxiliaryData(const PropertyName &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return internalNode()->hasAuxiliaryData(name);
}

const QHash<PropertyName, QVariant> &ModelNode::auxiliaryData() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return internalNode()->auxiliaryData();
}

QString ModelNode::customId() const
{
    QString result;
    if (hasCustomId())
        result = auxiliaryData(customIdProperty).value<QString>();

    return result;
}

bool ModelNode::hasCustomId() const
{
    return hasAuxiliaryData(customIdProperty);
}

void ModelNode::setCustomId(const QString &str)
{
    setAuxiliaryData(customIdProperty, QVariant::fromValue<QString>(str));
}

void ModelNode::removeCustomId()
{
    if (hasCustomId()) {
        removeAuxiliaryData(customIdProperty);
    }
}

QVector<Comment> ModelNode::comments() const
{
    return annotation().comments();
}

bool ModelNode::hasComments() const
{
    return annotation().hasComments();
}

void ModelNode::setComments(const QVector<Comment> &coms)
{
    Annotation anno = annotation();
    anno.setComments(coms);

    setAnnotation(anno);
}

void ModelNode::addComment(const Comment &com)
{
    Annotation anno = annotation();
    anno.addComment(com);

    setAnnotation(anno);
}

bool ModelNode::updateComment(const Comment &com, int position)
{
    bool result = false;
    if (hasAnnotation()) {
        Annotation anno = annotation();

        if (anno.updateComment(com, position)) {
            setAnnotation(anno);
            result = true;
        }
    }

    return result;
}

Annotation ModelNode::annotation() const
{
    Annotation result;

    if (hasAnnotation())
        result.fromQString(auxiliaryData(annotationProperty).value<QString>());

    return result;
}

bool ModelNode::hasAnnotation() const
{
    return hasAuxiliaryData(annotationProperty);
}

void ModelNode::setAnnotation(const Annotation &annotation)
{
    setAuxiliaryData(annotationProperty, QVariant::fromValue<QString>(annotation.toQString()));
}

void ModelNode::removeAnnotation()
{
    if (hasAnnotation()) {
        removeAuxiliaryData(annotationProperty);
    }
}

Annotation ModelNode::globalAnnotation() const
{
    Annotation result;
    ModelNode root = view()->rootModelNode();

    if (hasGlobalAnnotation())
        result.fromQString(root.auxiliaryData(globalAnnotationProperty).value<QString>());

    return result;
}

bool ModelNode::hasGlobalAnnotation() const
{
    return view()->rootModelNode().hasAuxiliaryData(globalAnnotationProperty);
}

void ModelNode::setGlobalAnnotation(const Annotation &annotation)
{
    view()->rootModelNode().setAuxiliaryData(globalAnnotationProperty,
                                             QVariant::fromValue<QString>(annotation.toQString()));
}

void ModelNode::removeGlobalAnnotation()
{
    if (hasGlobalAnnotation()) {
        view()->rootModelNode().removeAuxiliaryData(globalAnnotationProperty);
    }
}

GlobalAnnotationStatus ModelNode::globalStatus() const
{
    GlobalAnnotationStatus result;
    ModelNode root = view()->rootModelNode();

    if (hasGlobalAnnotation()) {
        result.fromQString(root.auxiliaryData(globalAnnotationStatus).value<QString>());
    }

    return result;
}

bool ModelNode::hasGlobalStatus() const
{
    return view()->rootModelNode().hasAuxiliaryData(globalAnnotationStatus);
}

void ModelNode::setGlobalStatus(const GlobalAnnotationStatus &status)
{
    view()->rootModelNode().setAuxiliaryData(globalAnnotationStatus,
                                             QVariant::fromValue<QString>(status.toQString()));
}

void ModelNode::removeGlobalStatus()
{
    if (hasGlobalStatus()) {
        view()->rootModelNode().removeAuxiliaryData(globalAnnotationStatus);
    }
}

bool ModelNode::locked() const
{
    if (hasLocked())
        return auxiliaryData(lockedProperty).toBool();

    return false;
}

bool ModelNode::hasLocked() const
{
    return hasAuxiliaryData(lockedProperty);
}

void ModelNode::setLocked(bool value)
{
    if (value) {
        setAuxiliaryData(lockedProperty, true);
        // Remove newly locked node and all its descendants from potential selection
        for (ModelNode node : allSubModelNodesAndThisNode()) {
            node.deselectNode();
            node.removeAuxiliaryData("timeline_expanded");
            node.removeAuxiliaryData("transition_expanded");
        }
    } else {
        removeAuxiliaryData(lockedProperty);
    }
}

bool ModelNode::isThisOrAncestorLocked(const ModelNode &node)
{
    if (!node.isValid())
        return false;

    if (node.locked())
        return true;

    if (node.isRootNode() || !node.hasParentProperty())
        return false;

    return isThisOrAncestorLocked(node.parentProperty().parentModelNode());
}

void  ModelNode::setScriptFunctions(const QStringList &scriptFunctionList)
{
    model()->d->setScriptFunctions(internalNode(), scriptFunctionList);
}

QStringList  ModelNode::scriptFunctions() const
{
    return internalNode()->scriptFunctions();
}

qint32 ModelNode::internalId() const
{
    if (m_internalNode.isNull())
        return -1;

    return m_internalNode->internalId();
}

void ModelNode::setNodeSource(const QString &newNodeSource)
{
    Internal::WriteLocker locker(m_model.data());

    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (internalNode()->nodeSource() == newNodeSource)
        return;

    m_model.data()->d->setNodeSource(internalNode(), newNodeSource);
}

void ModelNode::setNodeSource(const QString &newNodeSource, NodeSourceType type)
{
    Internal::WriteLocker locker(m_model.data());

    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (internalNode()->nodeSourceType() == type && internalNode()->nodeSource() == newNodeSource)
        return;

    internalNode()->setNodeSourceType(type); // Set type first as it doesn't trigger any notifies
    m_model.data()->d->setNodeSource(internalNode(), newNodeSource);
}

QString ModelNode::nodeSource() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return internalNode()->nodeSource();
}

QString ModelNode::convertTypeToImportAlias() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (model()->rewriterView())
        return model()->rewriterView()->convertTypeToImportAlias(QString::fromLatin1(type()));

    return QString::fromLatin1(type());
}

ModelNode::NodeSourceType ModelNode::nodeSourceType() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return static_cast<ModelNode::NodeSourceType>(internalNode()->nodeSourceType());

}

bool ModelNode::isComponent() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!metaInfo().isValid())
        return false;

    if (metaInfo().isFileComponent())
        return true;

    if (nodeSourceType() == ModelNode::NodeWithComponentSource)
        return true;

    if (metaInfo().isView() && hasNodeProperty("delegate")) {
        const ModelNode delegateNode = nodeProperty("delegate").modelNode();
        if (delegateNode.isValid()) {
            if (delegateNode.hasMetaInfo()) {
                const NodeMetaInfo delegateMetaInfo = delegateNode.metaInfo();
                if (delegateMetaInfo.isValid() && delegateMetaInfo.isFileComponent())
                    return true;
            }
            if (delegateNode.nodeSourceType() == ModelNode::NodeWithComponentSource)
                return true;
        }
    }

    if (metaInfo().isSubclassOf("QtQuick.Loader")) {

        if (hasNodeListProperty("component")) {

        /*
         * The component property should be a NodeProperty, but currently is a NodeListProperty, because
         * the default property is always implcitly a NodeListProperty. This is something that has to be fixed.
         */

            ModelNode componentNode = nodeListProperty("component").toModelNodeList().constFirst();
            if (componentNode.nodeSourceType() == ModelNode::NodeWithComponentSource)
                return true;
            if (componentNode.metaInfo().isFileComponent())
                return true;
        }

        if (hasNodeProperty("sourceComponent")) {
            if (nodeProperty("sourceComponent").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource)
                return true;
            if (nodeProperty("sourceComponent").modelNode().metaInfo().isFileComponent())
                return true;
        }

        if (hasVariantProperty("source"))
            return true;
    }

    return false;
}

bool ModelNode::isSubclassOf(const TypeName &typeName, int majorVersion, int minorVersion) const
{
    if (metaInfo().isValid())
        return metaInfo().isSubclassOf(typeName, majorVersion, minorVersion);

    return false;
}

QIcon ModelNode::typeIcon() const
{
    if (isValid()) {
        // if node has no own icon, search for it in the itemlibrary
        const ItemLibraryInfo *libraryInfo = model()->metaInfo().itemLibraryInfo();
        QList <ItemLibraryEntry> itemLibraryEntryList = libraryInfo->entriesForType(
                    type(), majorVersion(), minorVersion());
        if (!itemLibraryEntryList.isEmpty())
            return itemLibraryEntryList.constFirst().typeIcon();
        else if (metaInfo().isValid())
            return QIcon(QStringLiteral(":/ItemLibrary/images/item-default-icon.png"));
    }

    return QIcon(QStringLiteral(":/ItemLibrary/images/item-invalid-icon.png"));
}

}
