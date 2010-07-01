
QString pdbPathToName(QString path);

void getAvailablePDBTranslations(QStringList& names, QStringList& descs);
bool isPDBTranslation(QString translation);

class TextSource;
TextSource* getPDBTextSource(QString name, QString bookName);
