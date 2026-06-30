#include "CategoryModel.h"

#include <spdlog/spdlog.h>

CategoryModel::CategoryModel(QObject* parent)
    : QAbstractListModel(parent) {
}

CategoryModel::~CategoryModel() {}

int CategoryModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return m_categories.count();
}

QVariant CategoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_categories.count())
        return QVariant();

    const CategoryInfo& category = m_categories.at(index.row());

    switch (role) {
    case CategoryIdRole:
        return category.categoryId;
    case CategoryNameRole:
        return category.categoryName;
    case IconSourceRole:
        return category.iconSource;
    case AddonCountRole:
        return category.addonCount;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> CategoryModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[CategoryIdRole] = "categoryId";
    roles[CategoryNameRole] = "categoryName";
    roles[IconSourceRole] = "iconSource";
    roles[AddonCountRole] = "addonCount";
    return roles;
}

int CategoryModel::count() const {
    return m_categories.count();
}

void CategoryModel::setCategories(const QList<CategoryInfo>& categories) {
    beginResetModel();
    m_categories = categories;
    endResetModel();
    emit countChanged();
}

void CategoryModel::clear() {
    beginResetModel();
    m_categories.clear();
    endResetModel();
    emit countChanged();
}

void CategoryModel::updateCounts(const QMap<QString, int>& counts) {
    for (auto& cat : m_categories) {
        cat.addonCount = counts.value(cat.categoryId, 0);
    }
    if (!m_categories.isEmpty()) {
        emit dataChanged(index(0), index(m_categories.size() - 1), {AddonCountRole});
    }
}