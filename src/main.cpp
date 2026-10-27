#include <QApplication>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
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
#include <QVBoxLayout>
#include <QWidget>

class MainWindow final : public QWidget {
public:
    MainWindow() {
        setWindowTitle(QStringLiteral("List All Tokens"));
        resize(1040, 650);
        setupDatabase();
        buildUi();
        refresh();
    }
private:
    QLineEdit *platformFilter{}, *platform{}, *endpoint{}, *note{};
    QDateEdit *dateFilter{}, *date{};
    QSpinBox *input{}, *output{};
    QTableWidget *table{};
    QLabel *summary{};
    QSqlDatabase db;

    void setupDatabase() {
        const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(dir + "/usage.sqlite");
        if (!db.open()) QMessageBox::critical(this, "数据库错误", db.lastError().text());
        QSqlQuery q;
        q.exec("CREATE TABLE IF NOT EXISTS usage (id INTEGER PRIMARY KEY AUTOINCREMENT, platform TEXT NOT NULL, endpoint TEXT, day TEXT NOT NULL, input_tokens INTEGER NOT NULL, output_tokens INTEGER NOT NULL, note TEXT)");
    }

    void buildUi() {
        auto *root = new QVBoxLayout(this);
        auto *filters = new QHBoxLayout;
        platformFilter = new QLineEdit; platformFilter->setPlaceholderText("平台筛选");
        dateFilter = new QDateEdit; dateFilter->setCalendarPopup(true); dateFilter->setMinimumDate(QDate(1900, 1, 1)); dateFilter->setSpecialValueText("全部日期"); dateFilter->setDate(QDate(1900, 1, 1));
        auto *clear = new QPushButton("清除筛选");
        auto *exportBtn = new QPushButton("导出 CSV");
        filters->addWidget(platformFilter); filters->addWidget(dateFilter); filters->addWidget(clear); filters->addStretch(); filters->addWidget(exportBtn);
        root->addLayout(filters);

        table = new QTableWidget; table->setColumnCount(7); table->setHorizontalHeaderLabels({"平台", "端点", "日期", "输入 Token", "输出 Token", "总 Token", "备注"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->horizontalHeader()->setStretchLastSection(true); table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        root->addWidget(table, 1);

        auto *form = new QFormLayout;
        platform = new QLineEdit; endpoint = new QLineEdit; date = new QDateEdit(QDate::currentDate()); date->setCalendarPopup(true);
        input = new QSpinBox; output = new QSpinBox; input->setRange(0, 2147483647); output->setRange(0, 2147483647); note = new QLineEdit;
        form->addRow("平台 *", platform); form->addRow("端点", endpoint); form->addRow("日期 *", date); form->addRow("输入 Token", input); form->addRow("输出 Token", output); form->addRow("备注", note);
        auto *add = new QPushButton("添加记录"); auto *remove = new QPushButton("删除选中"); auto *actions = new QHBoxLayout; actions->addLayout(form, 1); auto *buttons = new QVBoxLayout; buttons->addWidget(add); buttons->addWidget(remove); buttons->addStretch(); actions->addLayout(buttons); root->addLayout(actions);
        summary = new QLabel; root->addWidget(summary);

        connect(add, &QPushButton::clicked, this, [this] { addRecord(); });
        connect(remove, &QPushButton::clicked, this, [this] { removeRecord(); });
        connect(clear, &QPushButton::clicked, this, [this] { platformFilter->clear(); dateFilter->setDate(QDate()); refresh(); });
        connect(exportBtn, &QPushButton::clicked, this, [this] { exportCsv(); });
        connect(platformFilter, &QLineEdit::textChanged, this, [this] { refresh(); });
        connect(dateFilter, &QDateEdit::dateChanged, this, [this] { refresh(); });
    }

    QString whereClause() const {
        QString where = " WHERE 1=1";
        if (!platformFilter->text().trimmed().isEmpty()) where += " AND platform LIKE :platform";
        if (dateFilter->date() > QDate(1900, 1, 1)) where += " AND day = :day";
        return where;
    }
    void bindFilters(QSqlQuery &q) const {
        if (!platformFilter->text().trimmed().isEmpty()) q.bindValue(":platform", "%" + platformFilter->text().trimmed() + "%");
        if (dateFilter->date() > QDate(1900, 1, 1)) q.bindValue(":day", dateFilter->date().toString(Qt::ISODate));
    }
    void refresh() {
        table->setRowCount(0); QSqlQuery q(db); q.prepare("SELECT platform, endpoint, day, input_tokens, output_tokens, input_tokens + output_tokens, note FROM usage" + whereClause() + " ORDER BY day DESC, id DESC"); bindFilters(q); if (!q.exec()) return;
        qint64 totalIn = 0, totalOut = 0; while (q.next()) { const int r = table->rowCount(); table->insertRow(r); for (int c = 0; c < 7; ++c) table->setItem(r, c, new QTableWidgetItem(q.value(c).toString())); totalIn += q.value(3).toLongLong(); totalOut += q.value(4).toLongLong(); }
        summary->setText(QString("共 %1 条记录 | 输入 %2 | 输出 %3 | 总计 %4").arg(table->rowCount()).arg(totalIn).arg(totalOut).arg(totalIn + totalOut));
    }
    void addRecord() {
        if (platform->text().trimmed().isEmpty()) { QMessageBox::warning(this, "提示", "平台名称不能为空"); return; }
        QSqlQuery q(db); q.prepare("INSERT INTO usage(platform, endpoint, day, input_tokens, output_tokens, note) VALUES(?, ?, ?, ?, ?, ?)"); q.addBindValue(platform->text().trimmed()); q.addBindValue(endpoint->text().trimmed()); q.addBindValue(date->date().toString(Qt::ISODate)); q.addBindValue(input->value()); q.addBindValue(output->value()); q.addBindValue(note->text().trimmed());
        if (!q.exec()) QMessageBox::critical(this, "保存失败", q.lastError().text()); else { platform->clear(); endpoint->clear(); input->setValue(0); output->setValue(0); note->clear(); refresh(); }
    }
    void removeRecord() {
        const auto row = table->currentRow(); if (row < 0) return; const auto platformName = table->item(row, 0)->text(); const auto day = table->item(row, 2)->text();
        QSqlQuery q(db); q.prepare("DELETE FROM usage WHERE rowid IN (SELECT rowid FROM usage WHERE platform=? AND day=? ORDER BY id DESC LIMIT 1)"); q.addBindValue(platformName); q.addBindValue(day); if (q.exec()) refresh();
    }
    void exportCsv() {
        const auto path = QFileDialog::getSaveFileName(this, "导出 CSV", "token-usage.csv", "CSV 文件 (*.csv)"); if (path.isEmpty()) return; QFile file(path); if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return; QTextStream out(&file); out.setEncoding(QStringConverter::Utf8); out << "平台,端点,日期,输入 Token,输出 Token,总 Token,备注\n";
        QSqlQuery q(db); q.prepare("SELECT platform, endpoint, day, input_tokens, output_tokens, input_tokens + output_tokens, note FROM usage" + whereClause() + " ORDER BY day DESC"); bindFilters(q); if (!q.exec()) return; while (q.next()) { for (int i = 0; i < 7; ++i) { if (i) out << ','; QString value = q.value(i).toString(); value.replace('"', "\"\""); out << '"' << value << '"'; } out << '\n'; }
    }
};
int main(int argc, char *argv[]) { QApplication app(argc, argv); MainWindow w; w.show(); return app.exec(); }
