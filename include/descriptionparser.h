#pragma once

#include <QString>

QString extractDescription(const QString& html);

QString extractDivByClass(const QString& html, const QString& className);