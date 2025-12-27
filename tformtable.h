#ifndef TFORMTABLE_H
#define TFORMTABLE_H

#include <QMainWindow>
#include <QtSql>

namespace Ui {
class TFormTable;
}

class TFormTable : public QMainWindow
{
    Q_OBJECT

private:
    QSqlDatabase DB;
    QSqlRelationalTableModel *tabModel;
    QItemSelectionModel *selModel;
    void openTable();

public:
    explicit TFormTable(QWidget *parent = nullptr);
    ~TFormTable();

private slots:
    void on_act_OpenDB_triggered();

    void on_act_RecAppend_triggered();

    void on_act_RecInsert_triggered();

    void on_act_RecDelete_triggered();

    void on_act_Submit_triggered();

    void do_currentChanged(const QModelIndex &current, const QModelIndex &previous);

    void on_act_Revert_triggered();

    void on_act_Fields_triggered();

private:
    Ui::TFormTable *ui;
};

#endif // TFORMTABLE_H
