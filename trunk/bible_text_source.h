#pragma once

#include <QList>
#include <QString>

void getAvailableTranslations(QStringList& names, QStringList& descs);

class TextSource;
TextSource* getBibleTextSource(QString translation, QString book);

