#pragma once

#include <QString>

// Extracts the addon description text from raw ESOUI page HTML.
QString extractDescription(const QString& html);
