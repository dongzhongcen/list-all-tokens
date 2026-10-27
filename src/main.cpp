#include <QApplication>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSpinBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

class MainWindow final : public QWidget {
public:
    MainWindow() : network(new QNetworkAccessManager(this)) {
        setWindowTitle(QStringLiteral("List All Tokens")); resize(1080, 700);
        setupDatabase(); buildUi(); refresh();
    }
private:
    QLineEdit *platformFilter{}, *platform{}, *endpoint{}, *apiKey{}, *note{};
    QDateEdit *dateFilter{}, *date{}; QSpinBox *input{}, *output{};
    QTableWidget *table{}; QLabel *summary{}, *syncStatus{}; QNetworkAccessManager *network{}; QSqlDatabase db;

    void setupDatabase() {
        const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation); QDir().mkpath(dir);
        db = QSqlDatabase::addDatabase("QSQLITE"); db.setDatabaseName(dir + "/usage.sqlite");
        if (!db.open()) QMessageBox::critical(this, "数据库错误", db.lastError().text());
        QSqlQuery q; q.exec("CREATE TABLE IF NOT EXISTS usage (id INTEGER PRIMARY KEY AUTOINCREMENT, platform TEXT NOT NULL, endpoint TEXT, day TEXT NOT NULL, input_tokens INTEGER NOT NULL, output_tokens INTEGER NOT NULL, note TEXT)");
    }
    void buildUi() {
        auto *root = new QVBoxLayout(this); auto *filters = new QHBoxLayout;
        platformFilter = new QLineEdit; platformFilter->setPlaceholderText("平台筛选");
        dateFilter = new QDateEdit; dateFilter->setCalendarPopup(true); dateFilter->setMinimumDate(QDate(1900,1,1)); dateFilter->setSpecialValueText("全部日期"); dateFilter->setDate(QDate(1900,1,1));
        auto *clear = new QPushButton("清除筛选"), *exportBtn = new QPushButton("导出 CSV");
        filters->addWidget(platformFilter); filters->addWidget(dateFilter); filters->addWidget(clear); filters->addStretch(); filters->addWidget(exportBtn); root->addLayout(filters);
        table = new QTableWidget; table->setColumnCount(7); table->setHorizontalHeaderLabels({"平台","接口","日期","输入 Token","输出 Token","总 Token","备注"}); table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->horizontalHeader()->setStretchLastSection(true); table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents); root->addWidget(table,1);
        auto *form = new QFormLayout; platform = new QLineEdit; endpoint = new QLineEdit; endpoint->setPlaceholderText("例如 https://host.example.com/v1/usage"); apiKey = new QLineEdit; apiKey->setEchoMode(QLineEdit::Password); apiKey->setPlaceholderText("Bearer API Token"); date = new QDateEdit(QDate::currentDate()); date->setCalendarPopup(true); input = new QSpinBox; output = new QSpinBox; input->setRange(0,2147483647); output->setRange(0,2147483647); note = new QLineEdit;
        form->addRow("平台 *",platform); form->addRow("用量接口地址 *",endpoint); form->addRow("API Token",apiKey); form->addRow("日期",date); form->addRow("输入 Token",input); form->addRow("输出 Token",output); form->addRow("备注",note);
        auto *sync = new QPushButton("自动获取用量"), *add = new QPushButton("手动添加"), *remove = new QPushButton("删除选中"); auto *actions = new QHBoxLayout; actions->addLayout(form,1); auto *buttons = new QVBoxLayout; buttons->addWidget(sync); buttons->addWidget(add); buttons->addWidget(remove); buttons->addStretch(); actions->addLayout(buttons); root->addLayout(actions);
        syncStatus = new QLabel("自动统计：填写接口地址后点击“自动获取用量”。"); summary = new QLabel; root->addWidget(syncStatus); root->addWidget(summary);
        connect(sync,&QPushButton::clicked,this,[this]{syncUsage();}); connect(add,&QPushButton::clicked,this,[this]{addRecord();}); connect(remove,&QPushButton::clicked,this,[this]{removeRecord();}); connect(clear,&QPushButton::clicked,this,[this]{platformFilter->clear(); dateFilter->setDate(QDate(1900,1,1)); refresh();}); connect(exportBtn,&QPushButton::clicked,this,[this]{exportCsv();}); connect(platformFilter,&QLineEdit::textChanged,this,[this]{refresh();}); connect(dateFilter,&QDateEdit::dateChanged,this,[this]{refresh();});
    }
    QString whereClause() const { QString w=" WHERE 1=1"; if(!platformFilter->text().trimmed().isEmpty()) w+=" AND platform LIKE :platform"; if(dateFilter->date()>QDate(1900,1,1)) w+=" AND day = :day"; return w; }
    void bindFilters(QSqlQuery &q) const { if(!platformFilter->text().trimmed().isEmpty()) q.bindValue(":platform","%"+platformFilter->text().trimmed()+"%"); if(dateFilter->date()>QDate(1900,1,1)) q.bindValue(":day",dateFilter->date().toString(Qt::ISODate)); }
    void refresh() { table->setRowCount(0); QSqlQuery q(db); q.prepare("SELECT platform,endpoint,day,input_tokens,output_tokens,input_tokens+output_tokens,note FROM usage"+whereClause()+" ORDER BY day DESC,id DESC"); bindFilters(q); if(!q.exec()) return; qint64 in=0,out=0; while(q.next()){int r=table->rowCount();table->insertRow(r);for(int c=0;c<7;c++)table->setItem(r,c,new QTableWidgetItem(q.value(c).toString()));in+=q.value(3).toLongLong();out+=q.value(4).toLongLong();} summary->setText(QString("共 %1 条记录 | 输入 %2 | 输出 %3 | 总计 %4").arg(table->rowCount()).arg(in).arg(out).arg(in+out)); }
    static void scanJson(const QJsonValue &v, qint64 &in, qint64 &out, qint64 &total) { if(v.isObject()){for(auto it=v.toObject().constBegin();it!=v.toObject().constEnd();++it){QString k=it.key().toLower();qint64 n=it.value().toVariant().toLongLong();if(k=="input_tokens"||k=="prompt_tokens"||k=="inputtokens"||k=="prompttokens")in+=n;else if(k=="output_tokens"||k=="completion_tokens"||k=="outputtokens"||k=="completiontokens")out+=n;else if(k=="total_tokens"||k=="totaltokens")total+=n;scanJson(it.value(),in,out,total);}} else if(v.isArray()) for(const auto &x:v.toArray()) scanJson(x,in,out,total); }
    void syncUsage() { if(platform->text().trimmed().isEmpty()||endpoint->text().trimmed().isEmpty()){QMessageBox::warning(this,"提示","请填写平台名称和用量接口地址");return;} QUrl url(endpoint->text().trimmed()); if(!url.isValid()||url.scheme().isEmpty()){QMessageBox::warning(this,"提示","接口地址无效");return;} QNetworkRequest req(url); req.setHeader(QNetworkRequest::UserAgentHeader,"ListAllTokens/1.0"); if(!apiKey->text().trimmed().isEmpty()) req.setRawHeader("Authorization",("Bearer "+apiKey->text().trimmed()).toUtf8()); syncStatus->setText("自动统计：正在请求接口..."); auto *reply=network->get(req); connect(reply,&QNetworkReply::finished,this,[this,reply]{reply->deleteLater(); if(reply->error()!=QNetworkReply::NoError){syncStatus->setText("自动统计失败："+reply->errorString());return;} QJsonParseError err; auto doc=QJsonDocument::fromJson(reply->readAll(),&err); if(err.error!=QJsonParseError::NoError){syncStatus->setText("自动统计失败：接口返回不是 JSON");return;} qint64 in=0,out=0,total=0;scanJson(doc.object().isEmpty()?QJsonValue(doc.array()):QJsonValue(doc.object()),in,out,total);if(in==0&&out==0&&total>0)in=total; if(in==0&&out==0){syncStatus->setText("自动统计失败：未找到 token 用量字段");return;} insertUsage(in,out,"接口自动同步"); syncStatus->setText(QString("自动统计成功：输入 %1，输出 %2，总计 %3").arg(in).arg(out).arg(in+out));}); }
    void insertUsage(qint64 in,qint64 out,const QString &remark){QSqlQuery q(db);q.prepare("INSERT INTO usage(platform,endpoint,day,input_tokens,output_tokens,note) VALUES(?,?,?,?,?,?)");q.addBindValue(platform->text().trimmed());q.addBindValue(endpoint->text().trimmed());q.addBindValue(QDate::currentDate().toString(Qt::ISODate));q.addBindValue(in);q.addBindValue(out);q.addBindValue(note->text().trimmed().isEmpty()?remark:note->text().trimmed());if(q.exec())refresh();else syncStatus->setText("保存失败："+q.lastError().text());}
    void addRecord(){if(platform->text().trimmed().isEmpty()){QMessageBox::warning(this,"提示","平台名称不能为空");return;}insertUsage(input->value(),output->value(),"手动录入");platform->clear();endpoint->clear();apiKey->clear();input->setValue(0);output->setValue(0);note->clear();}
    void removeRecord(){int r=table->currentRow();if(r<0)return;QSqlQuery q(db);q.prepare("DELETE FROM usage WHERE rowid IN (SELECT rowid FROM usage WHERE platform=? AND day=? ORDER BY id DESC LIMIT 1)");q.addBindValue(table->item(r,0)->text());q.addBindValue(table->item(r,2)->text());if(q.exec())refresh();}
    void exportCsv(){auto path=QFileDialog::getSaveFileName(this,"导出 CSV","token-usage.csv","CSV 文件 (*.csv)");if(path.isEmpty())return;QFile f(path);if(!f.open(QIODevice::WriteOnly|QIODevice::Text))return;QTextStream out(&f);out.setEncoding(QStringConverter::Utf8);out<<"平台,端点,日期,输入 Token,输出 Token,总 Token,备注\n";QSqlQuery q(db);q.prepare("SELECT platform,endpoint,day,input_tokens,output_tokens,input_tokens+output_tokens,note FROM usage"+whereClause()+" ORDER BY day DESC");bindFilters(q);if(!q.exec())return;while(q.next()){for(int i=0;i<7;i++){if(i)out<<',';QString s=q.value(i).toString();s.replace('"',"\"\"");out<<'"'<<s<<'"';}out<<'\n';}}
};
int main(int argc,char *argv[]){QApplication app(argc,argv);MainWindow w;w.show();return app.exec();}
