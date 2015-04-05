#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "util.h"
#include <QAbstractItemDelegate>
#include <QPainter>
#include <boost/assign/list_of.hpp>

#define DECORATION_SIZE 64
#define NUM_ITEMS 3

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(qVariantCanConvert<QColor>(value))
        {
            foreground = qvariant_cast<QColor>(value);
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    int unit = model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, stake));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + stake + unconfirmedBalance + immatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);
}

void OverviewPage::setModel(WalletModel *model)
{
    this->model = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

const std::vector<strengthlevel> levels =
    boost::assign::list_of<strengthlevel>
        ("Not Seeding", 0)
        ("Leecher", 10)
        ("Novice Seeder", 20)
        ("Junior Seeder", 25)
        ("Rising Seeder", 35)
        ("Medium Seeder", 40)
        ("Essential Seeder", 45)
        ("Hardcore Seeder", 55)
        ("Pro Seeder", 65)
        ("Elite Contributor", 75)
        ("Master Contributor", 100);

const double levelreq[] = {0, 0.00001, 0.0001, 0.001, 0.02, 0.05, 0.1, 0.15, 0.2, 0.25, 1.0};

int8_t getStrengthLevel(double strength)
{
    uint16_t level = 0;
    for(uint8_t i = 0; i < ARRAYLEN(levelreq); i++)
    {
        if(strength <= levelreq[i] && strength > 0)
        {
            level = i;
            break;
        }
    }
    
    return level;
}

double getNextLevelReq(double strength)
{
    return levelreq[getStrengthLevel(strength) + 1];
}

double GetStrength(double nWeight, double networkWeight)
{
    if (nWeight == 0 && networkWeight == 0)
        return 0;
    return nWeight / (static_cast<double>(nWeight) + networkWeight);
}

double OverviewPage::getNextLevelEstimate(double strength)
{
    if(networkWeight == 0 || networkWeight == weight)
        return 0;
    double nextLevel = getNextLevelReq(strength);
    if(nextLevel == 0.5) nextLevel += 0.0001;
    
    return (weight - nextLevel * (weight + networkWeight))/(2 * nextLevel -  1);
}

void OverviewPage::setStrength(double strength)
{
    int8_t levelIdx = getStrengthLevel(strength);
    QString name;
    QString tooltip = strBarTooltip;
    int8_t value;

    if(levelIdx < 0)
    {
        name = "Error!";
        value = 0;
    }
    else
    {
        strengthlevel level = levels[levelIdx];
        name = level.first;
        value = level.second;
        if (levelIdx < 10 && networkWeight > 0)
        {
            strengthlevel nextLevel = levels[levelIdx + 1];
            tooltip.append(QString(" Next level: %1, in about %2 coins.")
                           .arg(nextLevel.first)
                           .arg(QString::number(getNextLevelEstimate(strength), 'f', 0)));
        }
    }
    
    ui->strengthBar->setFormat(name);
    ui->strengthBar->setValue(value);
    ui->strengthBar->setTextVisible(true);
    ui->strengthBar->setToolTip(tooltip);
}

void OverviewPage::updateStrength()
{
    setStrength(GetStrength(weight, networkWeight));
}

void OverviewPage::setWeight(double nWeight)
{
    weight = nWeight;
}

void OverviewPage::setNetworkWeight(double nWeight)
{
    networkWeight = nWeight;
}

void OverviewPage::setInterestRate(qint64 interest)
{
    currentInterestRate = interest;
    
    // format as whole CENTs, this way we avoid conversion to double.
    // space before percent sign because the gui uses a space before most units
    QString formatStr = QString("%1 %").arg(BitcoinUnits::format(BitcoinUnits::BTC, interest));
    ui->interestValueLabel->setText(formatStr);
}

void OverviewPage::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, model->getStake(), currentUnconfirmedBalance, currentImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = model->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}
