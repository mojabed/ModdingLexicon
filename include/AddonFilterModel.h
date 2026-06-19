#pragma once

#include <QSortFilterProxyModel>
#include <QtQml>

class AddonFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(AddonFilterModel)

public:
    explicit AddonFilterModel(QObject* parent = nullptr);

    Q_INVOKABLE void setShowInstalledOnly(bool installed);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    bool m_showInstalledOnly = false;
};