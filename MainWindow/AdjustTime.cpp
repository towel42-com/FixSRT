#include "AdjustTime.h"
#include "ui_AdjustTime.h"

CAdjustTime::CAdjustTime( const QTime &initTime, const std::pair< uint64_t, bool > &offset, QWidget *parent ) :
    QDialog( parent ),
    fImpl( new Ui::CAdjustTime ),
    fInitTime( initTime )
{
    fImpl->setupUi( this );
    fImpl->initTime->setTime( initTime );
    fImpl->addBtn->setChecked( offset.second );
    fImpl->subBtn->setChecked( !offset.second );
    fImpl->timeOffset->setTime( QTime::fromMSecsSinceStartOfDay( offset.first ) );

    connect( fImpl->addBtn, &QRadioButton::clicked, this, &CAdjustTime::slotTimeAdjustChanged );
    connect( fImpl->subBtn, &QRadioButton::clicked, this, &CAdjustTime::slotTimeAdjustChanged );
    connect( fImpl->timeOffset, &QTimeEdit::timeChanged, this, &CAdjustTime::slotTimeAdjustChanged );

    connect( fImpl->resultTime, &QTimeEdit::timeChanged, this, &CAdjustTime::slotResultTimeChanged );
    slotTimeAdjustChanged();
}

CAdjustTime::~CAdjustTime()
{
}

std::pair< uint64_t, bool > CAdjustTime::newOffset()
{
    auto retVal = std::make_pair( fImpl->timeOffset->time().msecsSinceStartOfDay(), fImpl->addBtn->isChecked() );
    return retVal;
}

void CAdjustTime::slotTimeAdjustChanged()
{
    auto offset = newOffset();
    auto newValue = fInitTime.addMSecs( ( offset.second ? 1 : -1 ) * offset.first );
    disconnect( fImpl->resultTime, &QTimeEdit::timeChanged, this, &CAdjustTime::slotResultTimeChanged );
    fImpl->resultTime->setTime( newValue );
    connect( fImpl->resultTime, &QTimeEdit::timeChanged, this, &CAdjustTime::slotResultTimeChanged );
}

void CAdjustTime::slotResultTimeChanged()
{
    auto newOffset = fInitTime.msecsTo( fImpl->resultTime->time() );

    disconnect( fImpl->addBtn, &QRadioButton::clicked, this, &CAdjustTime::slotTimeAdjustChanged );
    disconnect( fImpl->subBtn, &QRadioButton::clicked, this, &CAdjustTime::slotTimeAdjustChanged );
    disconnect( fImpl->timeOffset, &QTimeEdit::timeChanged, this, &CAdjustTime::slotTimeAdjustChanged );

    fImpl->addBtn->setChecked( newOffset >= 0 );
    fImpl->subBtn->setChecked( newOffset < 0 );
    fImpl->timeOffset->setTime( QTime::fromMSecsSinceStartOfDay( std::abs( newOffset ) ) );

    connect( fImpl->addBtn, &QRadioButton::clicked, this, &CAdjustTime::slotTimeAdjustChanged );
    connect( fImpl->subBtn, &QRadioButton::clicked, this, &CAdjustTime::slotTimeAdjustChanged );
    connect( fImpl->timeOffset, &QTimeEdit::timeChanged, this, &CAdjustTime::slotTimeAdjustChanged );
}