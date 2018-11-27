#include <translator.h>
#include <QNetworkRequest>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QFile>
#include <QDataStream>


class TranslatorAPI {
  public:
    TranslatorAPI(const QString & s, const QString & t):
        from(s), to(t)
    {}

    virtual QNetworkRequest translateRequest(const QString & qword) = 0;
    virtual QString translateResult(const QByteArray & data) = 0;
    virtual ~TranslatorAPI() {}

  protected:
    QString from;
    QString to;
};

class BaiduTranslatorAPI : public TranslatorAPI {
  public:
    BaiduTranslatorAPI(const QString & s, const QString & t) :
        TranslatorAPI(s, t),
        appid("20181124000238616"),
        key("BWIrrd3ARnhIVX7IJEjk")
    {}

    QNetworkRequest translateRequest(const QString & qword) {
        QString queryStr(qword.toUtf8().data());
        QString salt = QString::asprintf("%u", qrand() % 100000000U);
        QString strSign = appid + queryStr + salt + key;
        QByteArray bytes(strSign.toStdString().data(), strSign.size());
        QString sign = QString().fromStdString(QCryptographicHash::hash(bytes, QCryptographicHash::Algorithm::Md5).toHex().toStdString());
        QString query = "&from=" + from + "&to=" + to + "&appid=" + appid + "&salt=" + salt + "&sign=" + sign + "&q=" + queryStr;
        QUrl url("http://fanyi-api.baidu.com/api/trans/vip/translate?" + query);
        return QNetworkRequest(url);
    }

    QString translateResult(const QByteArray & data) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        qDebug() << "Baidu: " << doc;
        QJsonObject obj = doc.object();
        QJsonValue val = obj.value("trans_result");
        if( !val.isArray()){
            return "";
        }
        QJsonArray array = val.toArray();
        obj = array[0].toObject();
        return obj.value("dst").toString();
    }

  private:
    const QString appid;
    const QString key;
};

class YandexTranslatorAPI : public TranslatorAPI {
  public:
    YandexTranslatorAPI(const QString & s, const QString & t) :
        TranslatorAPI(s, t),
        key("trnsl.1.1.20181124T160248Z.ee4557c60a06fbac.05af9931631627667f6d431932bb4de2a4ad8e8a")
    {}

    QNetworkRequest translateRequest(const QString & qword) {
        QString urlStr = "https://translate.yandex.net/api/v1.5/tr.json/translate?";
        urlStr = urlStr + "lang=" + from + "-" + to;
        urlStr = urlStr + "&key=" + key;
        urlStr = urlStr + "&text=" + qword.toUtf8().data();
        return QNetworkRequest(QUrl(QUrl(urlStr).toEncoded().data()));
    }

    QString translateResult(const QByteArray & data) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        qDebug() << "Yandex: " << doc;
        return doc.object().value("text").toArray()[0].toString();
    }

  private:
    const QString key;
};

Translator::Translator(QObject* parent) : QObject(parent) {
    translatorAPI = nullptr;
    netAccessMgr = new QNetworkAccessManager(this);
    connect(netAccessMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(replySlot(QNetworkReply*)));
    connect(netAccessMgr, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>&)),
            this, SLOT(sslErrorsSlot(QNetworkReply*,QList<QSslError>&)));
}

Translator::~Translator()
{
    delete translatorAPI;
}

void Translator::translateSlot(QString qword, QString t) {
    qDebug() << "Translate Sig Got";
    if( !translatorAPI ) {
        translatorAPI = new BaiduTranslatorAPI("en", "zh");
    }

    QNetworkReply* reply = netAccessMgr->get(translatorAPI->translateRequest(qword));
    m_query[reply] = qword;
    printf("reply:0x%.16lX, t: %s\n", reply, qword.toStdString().data());
}

void Translator::replySlot(QNetworkReply *reply) {
    QString s = m_query.value(reply, "");
    if( s.size() > 0 ){
        if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 200 ) {
            emit translateErrorSig(s, reply->readAll().toStdString().data());
        }else{
            emit translateFinishSig(s, translatorAPI->translateResult(reply->readAll()));
        }
        m_query.remove(reply);
    }
}

void Translator::updateTranslatorSlot(QString site, QString source, QString target) {
    if( translatorAPI ) {
        delete translatorAPI;
    }
    qDebug() << "Update Translator API" << endl;
    if( site.indexOf("Baidu", 0, Qt::CaseInsensitive) >= 0 ) {
        translatorAPI = new BaiduTranslatorAPI(source, target);
    } else {
        translatorAPI = new YandexTranslatorAPI(source, target);
    }
}

void Translator::sslErrorsSlot(QNetworkReply * reply, QList<QSslError> & errors)
{
    QString s = m_query.value(reply, "");
    if( s.size() > 0 ){
        m_query.remove(reply);
        emit translateErrorSig(s, "SSL Error");
    }
}

TranslatorCache::~TranslatorCache()
{
    QFile file(m_cacheFile);
    file.open(QFile::OpenModeFlag::Truncate|QFile::OpenModeFlag::WriteOnly);
    if( file.isOpen() ){
        QDataStream out(&file);
        out << m_cache;
        file.close();
    }
}

bool TranslatorCache::loadCache(const QString &cacheFile)
{
    qDebug() << "Loading Cache File";
    m_cacheFile = cacheFile;
    QFile file(cacheFile);
    file.open(QFile::OpenModeFlag::ReadOnly);
    if( file.isOpen() ){
        QDataStream in(&file);
        in >> m_cache;
        file.close();
        return true;
    }

    return false;
}

bool TranslatorCache::cachedValue(const QString &key, QString &value)
{
    QString val = m_cache.value(key, "");
    if( val.size() == 0 ){
        return false;
    }else{
        value = val;
        return true;
    }
}

void TranslatorCache::findCacheSlot(QString s, QString t)
{
    QString value;
    if( cachedValue(s, value) ){
        emit foundCacheSig(s, value);
    }else{
        emit foundNoCacheSig(s, t);
    }
}

void TranslatorCache::addCacheSlot(QString s, QString t)
{
    m_cache[s] = t;
    qDebug() << s << " = " << t << "Cached" ;
}
