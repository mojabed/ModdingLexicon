#pragma once

#include <QSortFilterProxyModel>
#include <QtQml>

class AddonFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(AddonFilterModel)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortModeChanged)

public:
    explicit AddonFilterModel(QObject* parent = nullptr);

    Q_INVOKABLE void setShowInstalledOnly(bool installed);
    Q_INVOKABLE void setCategoryFilter(const QString& categoryId);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setExcludeBelowApiVersion(int minApi);
    int excludeBelowApiVersion() const;
    Q_INVOKABLE void refreshFilter();

    Q_INVOKABLE void setSortMode(const QString& mode);
    Q_INVOKABLE void setSortOrder(Qt::SortOrder order);
    QString sortMode() const;
    Qt::SortOrder sortOrder() const;

    QString categoryFilter() const;
    QString searchText() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

signals:
    void categoryFilterChanged();
    void searchTextChanged();
    void sortModeChanged();

private:
    bool m_showInstalledOnly = false;
    QString m_categoryFilter;
    QString m_searchText;
    QString m_sortMode;
    Qt::SortOrder m_sortOrder = Qt::DescendingOrder;
    int m_excludeBelowApiVersion = 0;
};