
QString browseForPDB(QWidget* parent);
bool registerPDB(QString path, QString& error);
QString pdbPathToName(QString path);

QStringList getAllPDBNames();
bool isPDBTranslation(QString translation);

class TextSource;
TextSource* getPDBTextSource(QString name, QString bookName);
