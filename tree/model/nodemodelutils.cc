#include "nodemodelutils.h"

#include <QApplication>
#include <QQueue>
#include <QtConcurrent>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "mainwindowutils.h"

void NodeModelUtils::UpdateBranchUnitF(const Node* root, Node* node)
{
    if (!node || node->type != kTypeBranch || node->unit == root->unit)
        return;

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    double initial_total { 0.0 };
    const int unit { node->unit };
    const bool rule { node->rule };

    while (!queue.isEmpty()) {
        const auto* current { queue.dequeue() };

        switch (current->type) {
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

void NodeModelUtils::UpdatePathFPTS(StringHash& leaf, StringHash& branch, StringHash& support, const Node* root, const Node* node, CString& separator)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        const auto* current { queue.dequeue() };
        const auto path { ConstructPathFPTS(root, current, separator) };

        switch (current->type) {
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
        root->type = kTypeBranch;
        root->unit = default_unit;
    }

    assert(root != nullptr && "Root node should not be null after initialization");
}

Node* NodeModelUtils::GetNodeByID(CNodeHash& hash, int node_id)
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

QString NodeModelUtils::ConstructPathFPTS(const Node* root, const Node* node, CString& separator)
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

bool NodeModelUtils::IsInternalReferencedFPTS(Sqlite* sql, int node_id, CString& message)
{
    if (sql->InternalReference(node_id)) {
        MainWindowUtils::Message(
            QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it is internally referenced.").arg(message), kThreeThousand);

        return true;
    }

    return false;
}

bool NodeModelUtils::IsSupportReferencedFPTS(Sqlite* sql, int node_id, CString& message)
{
    if (sql->SupportReference(node_id)) {
        MainWindowUtils::Message(
            QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it is support referenced.").arg(message), kThreeThousand);
        return true;
    }

    return false;
}

bool NodeModelUtils::IsExternalReferencedPS(Sqlite* sql, int node_id, CString& message)
{
    if (sql->ExternalReference(node_id)) {
        MainWindowUtils::Message(
            QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it is external referenced.").arg(message), kThreeThousand);
        return true;
    }

    return false;
}

bool NodeModelUtils::HasChildrenFPTS(Node* node, CString& message)
{
    if (!node->children.isEmpty()) {
        MainWindowUtils::Message(
            QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it has children nodes.").arg(message), kThreeThousand);
        return true;
    }

    return false;
}

bool NodeModelUtils::IsOpenedFPTS(CTransWgtHash& hash, int node_id, CString& message)
{
    if (hash.contains(node_id)) {
        MainWindowUtils::Message(QMessageBox::Warning, QObject::tr("Operation Blocked"), QObject::tr("%1 because it is opened.").arg(message), kThreeThousand);
        return true;
    }

    return false;
}

void NodeModelUtils::UpdateComboModel(QStandardItemModel* model, const QVector<std::pair<QString, int>>& items)
{
    if (!model || items.isEmpty())
        return;

    model->clear();
    QSignalBlocker blocker(model);

    for (const auto& item : items) {
        AddItemToModel(model, item.first, item.second, false);
    }

    model->sort(0);
}

void NodeModelUtils::LeafPathBranchPathModelFPT(CStringHash& leaf, CStringHash& branch, QStandardItemModel* model)
{
    if (!model || (leaf.isEmpty() && branch.isEmpty()))
        return;

    auto future = QtConcurrent::run([&]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf.size() + branch.size());

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            items.emplaceBack(path, id);
        }

        for (const auto& [id, path] : branch.asKeyValueRange()) {
            items.emplaceBack(path, id);
        }

        items.emplaceBack(QString(), 0);

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        NodeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void NodeModelUtils::LeafPathModelFPT(CStringHash& leaf, QStandardItemModel* model)
{
    if (!model || leaf.isEmpty())
        return;

    auto future = QtConcurrent::run([&]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf.size());

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            items.emplaceBack(path, id);
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        NodeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void NodeModelUtils::LeafPathRangeModelP(CStringHash& leaf, CIntSet& range, QStandardItemModel* model)
{
    if (!model || leaf.isEmpty() || range.isEmpty())
        return;

    auto future = QtConcurrent::run([range, &leaf]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(range.size());

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            if (range.contains(id)) {
                items.emplaceBack(path, id);
            }
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        NodeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void NodeModelUtils::LeafPathRangeModelS(
    CStringHash& leaf, CIntSet& crange, QStandardItemModel* cmodel, CIntSet& vrange, QStandardItemModel* vmodel, CIntSet& erange, QStandardItemModel* emodel)
{
    auto future = QtConcurrent::run([&leaf, crange, vrange, erange]() {
        QVector<std::pair<QString, int>> citems;
        QVector<std::pair<QString, int>> vitems;
        QVector<std::pair<QString, int>> eitems;

        citems.reserve(crange.size());
        vitems.reserve(vrange.size());
        eitems.reserve(erange.size());

        eitems.emplaceBack(QString(), 0);

        // Single traversal of leaf
        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            if (crange.contains(id)) {
                citems.emplaceBack(path, id);
            }
            if (vrange.contains(id)) {
                vitems.emplaceBack(path, id);
            }
            if (erange.contains(id)) {
                eitems.emplaceBack(path, id);
            }
        }

        return std::make_tuple(std::move(citems), std::move(vitems), std::move(eitems));
    });

    auto* watcher = new QFutureWatcher<std::tuple<QVector<std::pair<QString, int>>, QVector<std::pair<QString, int>>, QVector<std::pair<QString, int>>>>(
        cmodel); // Use one model for ownership
    QObject::connect(watcher,
        &QFutureWatcher<std::tuple<QVector<std::pair<QString, int>>, QVector<std::pair<QString, int>>, QVector<std::pair<QString, int>>>>::finished, watcher,
        [watcher, cmodel, vmodel, emodel]() {
            auto [citems, vitems, eitems] = watcher->result();

            if (cmodel && !citems.isEmpty()) {
                NodeModelUtils::UpdateComboModel(cmodel, citems);
            }
            if (vmodel && !vitems.isEmpty()) {
                NodeModelUtils::UpdateComboModel(vmodel, vitems);
            }
            if (emodel && !eitems.isEmpty()) {
                NodeModelUtils::UpdateComboModel(emodel, eitems);
            }

            watcher->deleteLater();
        });

    watcher->setFuture(future);
}

void NodeModelUtils::LeafPathFilterModelFPTS(CNodeHash& hash, CStringHash& leaf, QStandardItemModel* model, int specific_unit, int exclude_node)
{
    if (!model || leaf.isEmpty())
        return;

    auto future = QtConcurrent::run([&, specific_unit, exclude_node]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(leaf.size());

        auto should_add = [specific_unit, exclude_node](const Node* node, int id) { return node->unit == specific_unit && id != exclude_node; };

        for (const auto& [id, path] : leaf.asKeyValueRange()) {
            auto it = hash.constFind(id);
            if (it != hash.constEnd() && should_add(it.value(), id)) {
                items.emplaceBack(path, id);
            }
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        NodeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void NodeModelUtils::SupportPathFilterModelFPTS(CStringHash& support, QStandardItemModel* model, int specific_node, Filter filter)
{
    if (!model)
        return;

    auto future = QtConcurrent::run([&, specific_node, filter]() {
        QVector<std::pair<QString, int>> items;
        items.reserve(support.size() + 1);

        auto should_add = [specific_node, filter](int support_id) {
            switch (filter) {
            case Filter::kIncludeAllWithNone:
                return true;
            case Filter::kExcludeSpecific:
                return support_id != specific_node;
            default:
                return false;
            }
        };

        if (filter == Filter::kIncludeAllWithNone) {
            items.emplaceBack(QString(), 0);
        }

        for (const auto& [id, path] : support.asKeyValueRange()) {
            if (should_add(id)) {
                items.emplaceBack(path, id);
            }
        }

        return items;
    });

    auto* watcher = new QFutureWatcher<QVector<std::pair<QString, int>>>(model);
    QObject::connect(watcher, &QFutureWatcher<QVector<std::pair<QString, int>>>::finished, watcher, [watcher, model]() {
        NodeModelUtils::UpdateComboModel(model, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void NodeModelUtils::AddItemToModel(QStandardItemModel* model, const QString& path, int node_id, bool should_sort)
{
    if (!model)
        return;

    auto* item { new QStandardItem(path) };
    item->setData(node_id, Qt::UserRole);

    model->appendRow(item);

    if (should_sort)
        model->sort(0);
}

void NodeModelUtils::RemoveItemFromModel(QStandardItemModel* model, int node_id)
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

        switch (current->type) {
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

void NodeModelUtils::UpdateUnitModel(CStringHash& leaf, QStandardItemModel* unit_model, const Node* node, int specific_unit, Filter filter)
{
    if (!node)
        return;

    QQueue<const Node*> queue {};
    QSet<int> range {};

    queue.enqueue(node);

    auto should_add = [filter, specific_unit](const Node* node) {
        switch (filter) {
        case Filter::kIncludeSpecific:
            return node->unit == specific_unit;
        case Filter::kExcludeSpecific:
            return node->unit != specific_unit;
        default:
            return false;
        }
    };

    while (!queue.isEmpty()) {
        const Node* current { queue.dequeue() };

        switch (current->type) {
        case kTypeBranch:
            for (const auto* child : current->children)
                queue.enqueue(child);

            break;
        case kTypeLeaf:
            if (should_add(current))
                range.insert(current->id);
            break;
        default:
            break;
        }
    }

    UpdateModelFunction(unit_model, range, leaf);
}

void NodeModelUtils::UpdatePathSeparatorFPTS(CString& old_separator, CString& new_separator, StringHash& source_path)
{
    if (old_separator == new_separator || new_separator.isEmpty() || source_path.isEmpty())
        return;

    for (auto& path : source_path)
        path.replace(old_separator, new_separator);
}

void NodeModelUtils::UpdateModelSeparatorFPTS(QStandardItemModel* model, CStringHash& source_path)
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
