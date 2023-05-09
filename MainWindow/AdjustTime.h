#ifndef __ADJUSTTIME_H
#define __ADJUSTTIME_H

#include <QDialog>
#include <QTime>
namespace Ui
{
    class CAdjustTime;
}

class CAdjustTime : public QDialog
{
    Q_OBJECT
public:
    CAdjustTime( const QTime &initTime, const std::pair< uint64_t, bool > &offset, QWidget *parent = nullptr );
    ~CAdjustTime();

    std::pair< uint64_t, bool > newOffset();

Q_SIGNALS:

public Q_SLOTS:
    void slotTimeAdjustChanged();
    void slotResultTimeChanged();

private:
    QTime fInitTime;
    std::unique_ptr< Ui::CAdjustTime > fImpl;
};

#endif
