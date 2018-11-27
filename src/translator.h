#ifndef TRANSLATOR_H__
#define TRANSLATOR_H__

#include <QMap>
#include <QString>
#include <QNetworkAccessManager>

class TranslatorAPI;


class TranslatorCache : public QObject {
    Q_OBJECT

public:
    TranslatorCache(QObject* parent) : QObject(parent) {
        m_cacheFile = "";
    }
    ~TranslatorCache();

public:
    bool loadCache(const QString & cacheFile);
    bool cachedValue(const QString & key, QString & value);

signals:
    void foundCacheSig(QString s, QString t);
    void foundNoCacheSig(QString s, QString t);

public slots:
    void findCacheSlot(QString s, QString t);
    void addCacheSlot(QString s, QString t);

private:
    QMap<QString, QString> m_cache;
    QString m_cacheFile;
};

class Translator : public QObject {
    Q_OBJECT
  public:
    explicit Translator(QObject* parent);
    ~Translator();

  public slots:
    void translateSlot(QString s, QString t);
    void replySlot(QNetworkReply *replySlot);
    void updateTranslatorSlot(QString, QString, QString);
    void sslErrorsSlot(QNetworkReply*,QList<QSslError>&);

  signals:
    void translateFinishSig(QString s, QString t);
    void translateErrorSig(QString s, QString t);

  private:
    QNetworkAccessManager* netAccessMgr;

  private:
    TranslatorAPI* translatorAPI;\
    QMap<const QNetworkReply*, QString> m_query;
};

#endif
