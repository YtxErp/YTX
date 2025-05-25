#include "nodemodelutils.h"

#include <QApplication>
#include <QQueue>
#include <QtConcurrent>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "mainwindowutils.h"

void NodeModelUtils::UpdateBranchUnit(const Node* root, Node* node)
{
    if (!node || node->node_type != kTypeBranch || node->unit == root->unit)
        return;

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    double initial_total { 0.0 };
    const int unit { node->unit };
    const bool rule { node->rule };

    while (!queue.isEmpty()) {
        const auto* current { queue.dequeue() };

        switch (current->node_type) {
        case kTypeBranch: {
            for (const auto* child : current->children)
                queue.enqueue(child);
        } break;
        case kTypeLeaf: {
            if (current->unit == unit)
                initial_total += (current->rule == rule ? 1 : -1) * current->initial_total;
        } break;
        default:
            break;
        }
    }

    node->initial_total = initial_total;
}

void NodeModelUtils::UpdatePath(StringHash& leaf, StringHash& branch, StringHash& support, const Node* root, const Node* node, CString& separator)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        const auto* current { queue.dequeue() };
        const auto path { ConstructPath(root, current, separator) };

        switch (current->node_type) {
        case kTypeBranch:
            for (const auto* child : current->children)
                queue.enqueue(child);

            branch.insert(current->id, path);
            break;
        case kTypeLeaf:
            leaf.insert(current->id, path);
            break;
        case kTypeSupport:
            support.insert(current->id, path);
            break;
        default:
            break;
        }
    }
}

void NodeModelUtils::InitializeRoot(Node*& root, int default_unit)
{
    if (root == nullptr) {
        root = ResourcePool<Node>::Instance().Allocate();
        root->id = -1;
        root->node_type = kTypeBranch;
        root->unit = default_unit;
    }

    assert(root && "Root node should not be null after initialization");
}

Node* NodeModelUtils::GetNode(CNodeHash& hash, int node_id)
{
    if (auto it = hash.constFind(node_id); it != hash.constEnd())
        return it.value();

    return nullptr;
}

bool NodeModelUtils::IsDescendant(const Node* lhs, const Node* rhs)
{
    if (!lhs || !rhs || lhs == rhs)
        return false;

    while (lhs && lhs != rhs)
        lhs = lhs->parent;

    return lhs == rhs;
}

void NodeModelUtils::SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare)
{
    if (!node)
        return;

    QQueue<Node*> queue {};
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        auto* current { queue.dequeue() };

        if (current->children.isEmpty())
            continue;

        std::sort(current->children.begin(), current->children.end(), Compare);
        for (auto* child : std::as_const(current->children)) {
            queue.enqueue(child);
        }
    }
}

QString NodeModelUtils::ConstructPath(const Node* root, const Node* node, CString& separator)
{
    if (!node || node == root)
        return QString();

    QStringList tmp {};

    while (node && node != root) {
        tmp.prepend(node->name);
        node = node->parent;
    }

    return tmp.join(separator);
}

bool NodeModelUtils::IsInternalReferenced(Sql* sql, int node_id, CString& message)
{
    if (sql->InternalReference(node_id)) {
        MainWindowUtils::Message(
            QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it is internally referenced.").arg(message), kThreeThousand);

        return true;
    }

    return false;
}

bool NodeModelUtils::IsSupportReferenced(Sql* sql, int node_id, CString& message)
{
    if (sql->SupportReference(node_id)) {
        MainWindowUtils::Message(
            QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it is support referenced.").arg(message), kThreeThousand);
        return true;
    }

    return false;
}

bool NodeModelUtils::IsExternalReferenced(Sql* sql, int node_id, CString& message)
{
    if (sql->ExternalReference(node_id)) {
        MainWindowUtils::Message(
            QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it is external referenced.").arg(message), kThreeThousand);
        return true;
    }

    return false;
}

bool NodeModelUtils::HasChildren(Node* node, CString& message)
{
    if (!node->children.isEmpty()) {
        MainWindowUtils::Message(
            QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it has children nodes.").arg(message), kThreeThousand);
        return true;
    }

    return false;
}

bool NodeModelUtils::IsOpened(CTransWgtHash& hash, int node_id, CString& message)
{
    if (hash.contains(node_id)) {
        MainWindowUtils::Message(QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it is opened.").arg(message), kThreeThousand);
        return true;
    }

    return false;
}

void NodeModelUtils::UpdateComboModel(QStandardItemModel* model, const QVector<std::pair<int, QString>>& items)
{
    if (!model || items.isEmpty())
        return;

    model->clear();
    QSignalBlocker blocker(model);

    for (const auto& item : items) {
        AppendItem(model, item.first, item.second);
    }

    model->sort(0);
}

void NodeModelUtils::LeafPathBranchPathModel(CStringHash& leaf, CStringHash& branch, QStandardItemModel* model)
{
    if (!model || (leaf.isEmpty() && branch.isEmpty()))
        return;

    auto future = QtConcurrent::run([&]() {
        QVector<std::pair<int, QString>> items;
        items.reserve(leaf.size() + branch.size());

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            items.emplaceBack(id, path);
        }

        for (const auto& [id, path] : branch.asKeyValueRange()) {
            items.emplaceBack(id, path);
        }

        items.emplaceBack(0, QString());

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<int, QString>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<int, QString>>>::finished, watcher, [watcher, model]() {
        NodeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void NodeModelUtils::AppendItem(QStandardItemModel* model, int node_id, CString& path)
{
    if (!model)
        return;

    auto* item { new QStandardItem(path) };
    item->setData(node_id, Qt::UserRole);
    model->appendRow(item);
}

void NodeModelUtils::RemoveItem(QStandardItemModel* model, int node_id)
{
    if (!model)
        return;

    for (int row = 0; row != model->rowCount(); ++row) {
        QStandardItem* item { model->item(row) };
        if (item && item->data(Qt::UserRole).toInt() == node_id) {
            model->removeRow(row);
            return;
        }
    }
}

void NodeModelUtils::UpdateModel(CStringHash& leaf, QStandardItemModel* leaf_model, CStringHash& support, QStandardItemModel* support_model, const Node* node)
{
    if (!node)
        return;

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    QSet<int> support_range {};
    QSet<int> leaf_range {};

    while (!queue.isEmpty()) {
        const auto* current { queue.dequeue() };

        switch (current->node_type) {
        case kTypeBranch:
            for (const auto* child : current->children)
                queue.enqueue(child);

            break;
        case kTypeLeaf:
            leaf_range.insert(current->id);
            break;
        case kTypeSupport:
            support_range.insert(current->id);
            break;
        default:
            break;
        }
    }

    UpdateModelFunction(support_model, support_range, support);
    UpdateModelFunction(leaf_model, leaf_range, leaf);
}

void NodeModelUtils::UpdatePathSeparator(CString& old_separator, CString& new_separator, StringHash& source_path)
{
    if (old_separator == new_separator || new_separator.isEmpty() || source_path.isEmpty())
        return;

    for (auto& path : source_path)
        path.replace(old_separator, new_separator);
}

void NodeModelUtils::UpdateModelSeparator(QStandardItemModel* model, CStringHash& source_path)
{
    if (!model || source_path.isEmpty())
        return;

    for (int row = 0; row != model->rowCount(); ++row) {
        QStandardItem* item = model->item(row);
        if (!item)
            continue;

        int id = item->data(Qt::UserRole).toInt();
        item->setText(source_path.value(id, QString {}));
    }
}

void NodeModelUtils::UpdateModelFunction(QStandardItemModel* model, CIntSet& update_range, CStringHash& source_path)
{
    if (!model || update_range.isEmpty() || source_path.isEmpty())
        return;

    for (int row = 0; row != model->rowCount(); ++row) {
        QStandardItem* item = model->item(row);
        if (!item)
            continue;

        int id = item->data(Qt::UserRole).toInt();
        if (update_range.contains(id)) {
            item->setText(source_path.value(id, QString {}));
        }
    }
}
