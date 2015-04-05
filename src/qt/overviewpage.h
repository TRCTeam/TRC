#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>
#include <stdint.h>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

static const QString strBarTooltip = "Proof of Seeding - ";
typedef std::pair<QString, uint8_t> strengthlevel;

namespace Ui {
    class OverviewPage;
}
class WalletModel;
class TxViewDelegate;
class TransactionFilterProxy;

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setModel(WalletModel *model);
    void showOutOfSyncWarning(bool fShow);

public slots:
    void setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance);
    void updateStrength();
    void setStrength(double strength);
    void setWeight(double nWeight);
    void setNetworkWeight(double nWeight);
    void setInterestRate(qint64 interest);

signals:
    void transactionClicked(const QModelIndex &index);

private:
    double getNextLevelEstimate(double strength);
    Ui::OverviewPage *ui;
    WalletModel *model;
    qint64 currentBalance;
    qint64 currentStake;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;
    double weight;
    double networkWeight;
    double currentStrength;
    qint64 currentInterestRate;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

private slots:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
};

#endif // OVERVIEWPAGE_H
