#pragma once

#include <QList>
#include <QString>

QStringList getAvailableTranslations();

class TextSource;
TextSource* getBibleTextSource(QString translation, QString book);

