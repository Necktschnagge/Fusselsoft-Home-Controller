/*
 * f_time.cpp
 *
 * Created: 04.05.2017 14:28:49
 *  Author: F-NET-ADMIN
 */ 

#include "f_time.h"

bool HumanTime::Date::is_valid(bool isLeapYear) const {
	if ((0 <= static_cast<uint8_t>(month)) && (static_cast<uint8_t>(month) <12)){
		return day < getMonthLength(month,isLeapYear);
	} else {
		return false;
	}
}

bool HumanTime::HDate::is_valid(bool isLeapYear) const {
	if ((month==0) || (month > 12)) return false;
	return ((day != 0) || (day <= getMonthLength(int_to_month(month),isLeapYear)));
}
/*
HumanTime::HumanTime(const ETime& etime) : second(etime.time.second), minute(etime.time.minute), hour(etime.time.hour), day(etime.time.day), year() {
	normalize();
}
*/


void HumanTime::normalize(){
	// clear seconds:
	minute += second / 60;
	second = second % 60;
	// clear minutes:
	hour += minute / 60;
	minute %= 60;
	// clear hour:
	day += hour / 24;
	hour %= 24;
	// clear day;
	if (day >= daysOfYear()) {
		day -= daysOfYear();
		++year;
		normalize();
	}
	if (day < 0){
		--year;
		day += daysOfYear();
		normalize();
	}
}

uint8_t HumanTime::getMonthLength(Month month, bool isLeapYear){ // ready, checked
	if (month == Month::FEB) return 28 + isLeapYear;
	return HumanTime::__monthLength__[static_cast<uint8_t>(month)];
}

HumanTime& HumanTime::operator++(){// ready, checked, change implementation!
	++second; // tick forward
	
	// check overflows:
	
	//old implementation // faster but needs more program memory
	if (second == 60){
		second = 0;
		++minute;
	}
	if (minute == 60){
		minute = 0;
		++hour;
	}
	if (hour == 24){
		hour = 0;
		++day;
	}
	if (day == daysOfYear()){
		day = 0;
		++year;
	}
	// normalize should be better because of case second == 61 a.s.o.
	
	// new implementation // needs more time and stack but less flash memory
	//normalize();
	
	return *this;
}


HumanTime::Date HumanTime::getDate() const { // ready, checked
	int16_t d {day};
	Month m {Month::JAN};
	while (d >= this->getMonthLength(m)){
		d -= this->getMonthLength(m);
		++m;
	}
	return Date(m, static_cast<uint8_t>(d), true);
}

bool HumanTime::setDate(const HumanTime::Date& date){
	if (   !date.is_valid( this->isLeapYear() )   ) return false;
	this->day = 0; // 01.01.
	for (Month m = Month::JAN; m != date.month; ++m){
		this->day += getMonthLength(m);
	}
	return true;
}

/*bool Time::setDate(uint8_t month, uint8_t day_of_month){
	//### do this
	return false;
}*/

Day HumanTime::getDayOfWeek() const { // ready, not to check but to be tested
	/* via extended version of Gauss' formula (Georg Glaeser) */
	Date cdate = getDate();
	int16_t cyear = this->year - (month_to_int(cdate.month) < 3);
	uint8_t cday = cdate.day;
	--cdate.month;
	--cdate.month;
	uint16_t shiftedMonth = month_to_int(cdate.month);
	uint8_t llyear = cyear % 100;
	int16_t uuyear = cyear / 100;
	
	return static_cast<Day>((cday + ((26*shiftedMonth-2) / 10 ) + llyear + (llyear / 4) + (uuyear / 4) -2*uuyear) % 7);
	//return Day::MON;
}

bool HumanTime::operator == (HumanTime& rop){
	/*do we have to normalize the time ??#####*/
	rop.normalize();
	/*Time self {*this};
	self.normalize();*/
	normalize();
	return (rop.second == second) && (rop.minute == minute) && (rop.hour == hour) && (rop.day == day) && (rop.year == year);
}

bool HumanTime::operator <(HumanTime& rop){
	rop.normalize();
	normalize();
	return (year < rop.year) || ((year == rop.year) && (
		(day < rop.day) || ((day == rop.day) && (
			(hour < rop.hour) || ((hour == rop.hour) && (
				(minute < rop.minute) || ((minute == rop.minute) && (
					second < rop.second
				))
			))
		))
	));
}

/*Time Time::operator + (Time& rop){
	rop.normalize();
	normalize();
	Time result;
	
	result.second = second + rop.second;
	result.minute = minute + rop.minute;
	result.hour = hour + rop.hour;
	result.day = day + rop.day;
	result.year = year + rop.year;// <<< overflow ignored.
	result.normalize();
	
	return result;
}

Time Time::operator - (Time& rop){
	rop.normalize();
	normalize();
	Time result;
	
	result.second = second - rop.second;
	result.minute = minute - rop.minute;
	result.hour = hour - rop.hour;
	result.day = day - rop.day; // this is not working this way. if we want a difference between two dates. (????)
	result.year = year;
	result.normalize();
	//### idea. we define a new "independent time": second since time zero and make convertion constructors to normal time.
	
	result.year -= rop.year; // overflow ignored;
	result.normalize();
	
	return result;
}*/

namespace {
	// 400 years in days:
	constexpr int64_t factor400 {400ll * 365 /*normal year*/ + 100 * 1 /*leap years*/ - 3 /*no leap years: 100, 200, 300*/ };
	// [++value]s for one day:
	constexpr int64_t ticksPerDay { 24LL * 60 * 60 * (1LL << 16) };
}

/************************************************************************/
/* ET class                                                             */
/************************************************************************/

#if false
ExtendedTime::ExtendedTime(const EMT& emt) /*: divisions_of_second( (time.operator int64_t()) & 0xFFFFLL)*/ {
	int64_t value = emt;
	divisions_of_second = value % 0x10000;
	value -= divisions_of_second;
	value = value >> 16;
	/*
	time.second = value % 60;
	value -= time.second;
	value /= 60;
	time.minute = value % 60;
	value -= time.minute;
	value /= 60;
	time.hour = value % 24;
	value -= time.hour;
	value /= 60;
	// now we have time remaining in days...
	// first check for applicable 400 years...
	time.year = 400 * (  ( value - (value % factor400) ) / factor400  );
	value -= time.year * factor400;
	// 0 <= value < factor400 assert <<<<<
	while (value >= time.daysOfYear()){
		value -= time.daysOfYear();
		++time.year;
	}
	time.day = value;
	*////####work around private
}
#endif
/************************************************************************/
/* EMT class                                                            */
/************************************************************************/

#if false
ExtendedMetricTime& ExtendedMetricTime::operator = (const ExtendedTime& time){	
	/*value = time.divisions_of_second;
	Time source = time.time; // cannot use element initializer {...} because of public member attributes (!)
	source.normalize();
	
	value += ticksPerDay * factor400 * ( (source.year - source.year % 400) / 400); // -600 -> -800 -> -2 instead of -600 -> -1
	source.year %= 400;
	while(source.year > 0){
		--source.year;
		value += ticksPerDay * source.daysOfYear();
	}
	value += (1LL<<16) * (60LL * (60LL * (24LL * (source.day) + source.hour) + source.minute) + source.second);
	*/ //work around made things private
	return *this;
}
#endif

