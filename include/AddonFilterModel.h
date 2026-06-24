#pragma once

#include <QSortFilterProxyModel>
#include <QtQml>
#include <QSet>

class AddonFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(AddonFilterModel)

public:
    explicit AddonFilterModel(QObject* parent = nullptr);

    Q_INVOKABLE void setShowInstalledOnly(bool installed);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private slots:
    void onSourceModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

private:
    bool m_showInstalledOnly = false;
    mutable QSet<int> m_installedRowsCache;
    mutable bool m_cacheValid = false;
    
    void invalidateCache();
};