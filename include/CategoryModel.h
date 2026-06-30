#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QtQml>
#include <QString>

struct CategoryInfo {
    QString categoryId;
    QString categoryName;
    QString iconSource;
    int addonCount = 0;
};

class CategoryModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(CategoryModel)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum CategoryRoles {
        CategoryIdRole = Qt::UserRole + 1,
        CategoryNameRole,
        IconSourceRole,
        AddonCountRole
    };
    Q_ENUM(CategoryRoles)

     explicit CategoryModel(QObject* parent = nullptr);
    ~CategoryModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    Q_INVOKABLE void setCategories(const QList<CategoryInfo>& categories);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void updateCounts(const QMap<QString, int>& counts);

signals:
    void countChanged();

private:
    QList<CategoryInfo> m_categories;
};