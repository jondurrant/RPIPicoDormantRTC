/*
 * RP2040 driver for DS3231 RTC module.
 *
 * Adapter from work by Victor Gabriel Costin.
 * Jon Durrant
 * Licensed under the MIT license.
 */

#include "DS3231.hpp"
#include "internal/ds3231.h"

#include "hardware/i2c.h"
#include <cstdio>

/*
 * Indexes of internal data array
 */
#define DATA_SEC            1
#define DATA_MIN            2
#define DATA_HOU            3
#define DATA_DOW            4
#define DATA_DAY            5
#define DATA_MON            6
#define DATA_YEAR           7

/*
 * Bit defines for the hour raw data
 */
#define HOU_TENS_1          4
#define HOU_TENS_2          5
#define HOU_AM_PM           5
#define HOU_FORMAT          6


/*
 * Alarm Registers
 */
#define DS3231_ALARM1_ADDR          0x07
#define DS3231_ALARM2_ADDR          0x0B
#define DS3231_CONTROL_ADDR         0x0E
#define DS3231_STATUS_ADDR          0x0F

// control register bits
#define DS3231_CONTROL_A1IE     0x1		/* Alarm 2 Interrupt Enable */
#define DS3231_CONTROL_A2IE     0x2		/* Alarm 2 Interrupt Enable */
#define DS3231_CONTROL_INTCN    0x4		/* Interrupt Control */
#define DS3231_CONTROL_RS1	    0x8		/* square-wave rate select 2 */
#define DS3231_CONTROL_RS2    	0x10	/* square-wave rate select 2 */
#define DS3231_CONTROL_CONV    	0x20	/* Convert Temperature */
#define DS3231_CONTROL_BBSQW    0x40	/* Battery-Backed Square-Wave Enable */
#define DS3231_CONTROL_EOSC	    0x80	/* not Enable Oscillator, 0 equal on */

// status register bits
#define DS3231_STATUS_A1F      0x01		/* Alarm 1 Flag */
#define DS3231_STATUS_A2F      0x02		/* Alarm 2 Flag */
#define DS3231_STATUS_BUSY     0x04		/* device is busy executing TCXO */
#define DS3231_STATUS_EN32KHZ  0x08		/* Enable 32KHz Output  */
#define DS3231_STATUS_OSF      0x80		/* Oscillator Stop Flag */



/*
 * Read macros
 */
#define READ_SEC_DATA()     _read_data_reg(DS3231_SEC_REG, 1)
#define READ_MIN_DATA()     _read_data_reg(DS3231_MIN_REG, 1);
#define READ_HOU_DATA()     _read_data_reg(DS3231_HOU_REG, 1);
#define READ_DAY_DATA()     _read_data_reg(DS3231_DAY_REG, 1);
#define READ_MON_DATA()     _read_data_reg(DS3231_MON_REG, 1);
#define READ_YEAR_DATA()    _read_data_reg(DS3231_YEAR_REG, 1);
#define READ_ALL_DATA()     _read_data_reg(DS3231_SEC_REG, DS3231_NO_DATA_REG);

/*
 * Set macros
 */
#define WRITE_SEC_DATA()    _write_data_reg(DS3231_SEC_REG, 1)
#define WRITE_MIN_DATA()    _write_data_reg(DS3231_MIN_REG, 1);
#define WRITE_HOU_DATA()    _write_data_reg(DS3231_HOU_REG, 1);
#define WRITE_TIME_DATA()   _write_data_reg(DS3231_SEC_REG, 3);
#define WRITE_DAY_DATA()    _write_data_reg(DS3231_DAY_REG, 1);
#define WRITE_MON_DATA()    _write_data_reg(DS3231_MON_REG, 1);
#define WRITE_YEAR_DATA()   _write_data_reg(DS3231_YEAR_REG, 1);
#define WRITE_DATE_DATA()   _write_data_reg(DS3231_DAY_REG, 3);

/*
 * Decode macros
 */
#define DECODE_SEC()        do { _sec = _decode_gen(_data_buffer[DATA_SEC]); } while (0);
#define DECODE_MIN()        do { _min = _decode_gen(_data_buffer[DATA_MIN]); } while (0);
#define DECODE_HOU()        do { _decode_hou(); } while (0);

#define DECODE_DAY()        do { _day = _decode_gen(_data_buffer[DATA_DAY]); } while (0);
#define DECODE_MON()        do { _mon = _decode_gen(_data_buffer[DATA_MON]); } while (0);
#define DECODE_YEAR()       do { _year = 2000 + _decode_gen(_data_buffer[DATA_YEAR]); } while (0);

/*
 * Encode macros
 */
#define ENCODE_SEC(sec)                     do { _data_buffer[DATA_SEC] = _encode_gen(sec); } while (0);
#define ENCODE_MIN(min)                     do { _data_buffer[DATA_MIN] = _encode_gen(min); } while (0);
#define ENCODE_HOU(hou, format, is_pm)      do { _data_buffer[DATA_HOU] = _encode_hou(hou, format, is_pm); } while (0);

#define ENCODE_DAY(day)                     do { _data_buffer[DATA_DAY] = _encode_gen(day); } while (0);
#define ENCODE_MON(mon)                     do { _data_buffer[DATA_MON] = _encode_gen(mon); } while (0);
#define ENCODE_YEAR(year)                   do { _data_buffer[DATA_YEAR] = _encode_gen(year); } while (0);

/*
 * Miscellaneous
 */
#define MASK_BIT(bit)       (1 << bit)
#define SET_BIT(x, bit)     (x |= (1 << bit))
#define UNSET_BIT(x, bit)   (x &= (~(1 << bit)))
#define LSB_HALF(x)         (x & 0xF)
#define MSB_HALF(x)         ((x & (0xF << 4)) >> 4)
#define COMBINE(x, y)       ((uint8_t)((x << 4) | (y)))

#define ASCII_OFFSET        48

DS3231::DS3231(i2c_inst_t *i2c, uint8_t sdaPin, uint8_t sclPin){
	init(i2c,  sdaPin,  sclPin);
}

DS3231::DS3231(){
	init(i2c_default, PICO_DEFAULT_I2C_SDA_PIN , PICO_DEFAULT_I2C_SCL_PIN );

}

void DS3231::init(i2c_inst_t *i2c, uint8_t sdaPin, uint8_t sclPin){
	_i2c = i2c;
    i2c_init(_i2c, 400000);
    gpio_set_function(sdaPin, GPIO_FUNC_I2C);
    gpio_set_function(sclPin, GPIO_FUNC_I2C);
    gpio_pull_up(sdaPin);
    gpio_pull_up(sclPin);
}

void DS3231::_read_data_reg(uint8_t reg, uint8_t n_regs)
{
    _data_buffer[0] = reg;
    i2c_write_blocking(_i2c, DS3231_ADDR, _data_buffer, 1, true);
    i2c_read_blocking(_i2c, DS3231_ADDR, _data_buffer + reg + 1, n_regs, false);
}

void DS3231::_write_data_reg(uint8_t reg, uint8_t n_regs)
{
    uint8_t buffer[8];

    buffer[0] = reg;
    for (int i = 1; i <= n_regs; i++) {
        buffer[i] = _data_buffer[reg + i];
    }

    i2c_write_blocking(_i2c, DS3231_ADDR, buffer, n_regs + 1, false);
}

inline void DS3231::_format_time_string()
{
    /*
     * Format time string
     */
    _time_str_buffer[2] = _time_str_buffer[5] = ':';
    _time_str_buffer[1] = LSB_HALF(_data_buffer[DATA_HOU]) + ASCII_OFFSET;
    _time_str_buffer[3] = MSB_HALF(_data_buffer[DATA_MIN]) + ASCII_OFFSET;
    _time_str_buffer[4] = LSB_HALF(_data_buffer[DATA_MIN]) + ASCII_OFFSET;
    _time_str_buffer[6] = MSB_HALF(_data_buffer[DATA_SEC]) + ASCII_OFFSET;
    _time_str_buffer[7] = LSB_HALF(_data_buffer[DATA_SEC]) + ASCII_OFFSET;

    if (_12_format) {
        _time_str_buffer[0] = MSB_HALF(_data_buffer[DATA_HOU] & MASK_BIT(HOU_TENS_1)) + ASCII_OFFSET;
        if (_is_pm)
            _time_str_buffer[9] = 'P';
        else
            _time_str_buffer[9] = 'A';
        _time_str_buffer[10] = 'M';
        _time_str_buffer[11] = '\0';
    } else {
        _time_str_buffer[0] = MSB_HALF((_data_buffer[DATA_HOU] & MASK_BIT(HOU_TENS_1)) |
                                (_data_buffer[DATA_HOU] & MASK_BIT(HOU_TENS_2))) + ASCII_OFFSET;
        _time_str_buffer[8] = '\0';
    }
}

inline void DS3231::_format_date_string()
{
    int year_aux = 0, aux = 0;

    _date_str_buffer[2] = _date_str_buffer[5] = '.';
    _date_str_buffer[10] = '\0';
    _date_str_buffer[0] = MSB_HALF(_data_buffer[DATA_DAY]) + ASCII_OFFSET;
    _date_str_buffer[1] = LSB_HALF(_data_buffer[DATA_DAY]) + ASCII_OFFSET;
    _date_str_buffer[3] = MSB_HALF(_data_buffer[DATA_MON]) + ASCII_OFFSET;
    _date_str_buffer[4] = LSB_HALF(_data_buffer[DATA_MON]) + ASCII_OFFSET;

    year_aux = _year;
    while (year_aux >= 1000) {
        aux++;
        year_aux -= 1000;
    }
    _date_str_buffer[6] = aux + ASCII_OFFSET;

    aux = 0;
    while (year_aux >= 100) {
        aux++;
        year_aux -= 100;
    }
    _date_str_buffer[7] = aux + ASCII_OFFSET;

    aux = 0;
    while (year_aux >= 10) {
        aux++;
        year_aux -= 10;
    }
    _date_str_buffer[8] = aux + ASCII_OFFSET;
    _date_str_buffer[9] = year_aux + ASCII_OFFSET;
}

inline uint8_t DS3231::_decode_gen(uint8_t raw_data)
{
    return MSB_HALF(raw_data) * 10 + LSB_HALF(raw_data);
}

inline void DS3231::_decode_hou()
{
    _12_format = _data_buffer[DATA_HOU] & MASK_BIT(HOU_FORMAT);
    if (_12_format) {
        _is_pm = _data_buffer[DATA_HOU] & MASK_BIT(HOU_AM_PM);
        _hou = MSB_HALF(_data_buffer[DATA_HOU] & MASK_BIT(HOU_TENS_1)) * 10 +
                LSB_HALF(_data_buffer[DATA_HOU]);
    } else {
        _hou = MSB_HALF((_data_buffer[DATA_HOU] & MASK_BIT(HOU_TENS_1)) |
                        (_data_buffer[DATA_HOU] & MASK_BIT(HOU_TENS_2))) * 10 +
                            LSB_HALF(_data_buffer[DATA_HOU]);
    }
}

inline uint8_t DS3231::_encode_gen(uint8_t data)
{
    uint8_t msbd = 0;

    while (data >= 10) {
        msbd++;
        data -= 10;
    }

    return COMBINE(msbd, data);
}

inline uint8_t DS3231::_encode_hou(uint8_t hou, bool am_pm_format, bool is_pm)
{
    uint8_t rv = 0;

    rv = _encode_gen(hou);

    if (am_pm_format) {
        SET_BIT(rv, HOU_FORMAT);

        if (is_pm)
            SET_BIT(rv, HOU_AM_PM);
    }

    return rv;
}

uint8_t DS3231::get_temp()
{
    uint8_t temp;
    uint8_t frac;

    _data_buffer[0] = DS3231_MSB_TMP_REG;
    i2c_write_blocking(_i2c, DS3231_ADDR, _data_buffer, 1, true);
    i2c_read_blocking(_i2c, DS3231_ADDR, &temp, 1, false);

    return temp;
}

float DS3231::get_temp_f(){
	uint8_t temp;
	uint8_t frac;

	_data_buffer[0] = DS3231_MSB_TMP_REG;
	i2c_write_blocking(_i2c, DS3231_ADDR, _data_buffer, 1, true);
	i2c_read_blocking(_i2c, DS3231_ADDR, &temp, 1, false);

	_data_buffer[0] = DS3231_LSB_TMP_REG;
	i2c_write_blocking(_i2c, DS3231_ADDR, _data_buffer, 1, true);
	i2c_read_blocking(_i2c, DS3231_ADDR, &frac, 1, false);

	frac = frac >> 6;

	return ((float)temp + (0.25 * (float)frac));

}

uint8_t DS3231::get_sec()
{
    READ_SEC_DATA();
    DECODE_SEC();

    return _sec;
}

uint8_t DS3231::get_min()
{
    READ_MIN_DATA();
    DECODE_MIN();

    return _min;
}

uint8_t DS3231::get_hou()
{
    READ_HOU_DATA();
    DECODE_HOU();

    return _hou;
}

const char* DS3231::get_time_str()
{
    READ_ALL_DATA();
    DECODE_SEC();
    DECODE_MIN();
    DECODE_HOU();
    _format_time_string();

    return _time_str_buffer;
}

void DS3231::set_hou(uint8_t hou, bool am_pm_format, bool is_pm)
{
    if (am_pm_format && (hou < 1 || hou > 12) ||
        hou > 23)
        return;

    ENCODE_HOU(hou, am_pm_format, is_pm);
    WRITE_HOU_DATA();
}

void DS3231::set_min(uint8_t min)
{
    if (min > 59)
        return;

    ENCODE_MIN(min);
    WRITE_MIN_DATA();
}

void DS3231::set_sec(uint8_t sec)
{
    if (sec > 59)
        return;

    ENCODE_SEC(sec);
    WRITE_SEC_DATA();
}

void DS3231::set_time(uint8_t hou, uint8_t min, uint8_t sec,
                            bool am_pm_format, bool is_pm)
{
    if (am_pm_format && (hou < 1 || hou > 12) ||
        hou > 23 || min > 59 || sec > 59)
        return;

    ENCODE_HOU(hou, am_pm_format, is_pm);
    ENCODE_MIN(min);
    ENCODE_SEC(sec);
    WRITE_TIME_DATA();
}

uint8_t DS3231::get_day()
{
    READ_DAY_DATA();
    DECODE_DAY();

    return _day;
}

uint8_t DS3231::get_mon()
{
    READ_MON_DATA();
    DECODE_MON();

    return _mon;
}

int DS3231::get_year()
{
    READ_YEAR_DATA();
    DECODE_YEAR();

    return _year;
}

const char* DS3231::get_date_str()
{
    READ_ALL_DATA();
    DECODE_DAY();
    DECODE_MON();
    DECODE_YEAR();
    _format_date_string();

    return _date_str_buffer;
}

void DS3231::set_day(uint8_t day)
{
    if (day < 1 || day > 31)
        return;

    ENCODE_DAY(day);
    WRITE_DAY_DATA();
}

void DS3231::set_mon(uint8_t mon)
{
    if (mon < 1 || mon > 12)
        return;

    ENCODE_MON(mon);
    WRITE_MON_DATA();
}

void DS3231::set_year(int year)
{
    if (year < 2000 || year > 2099)
        return;

    ENCODE_YEAR(year - 2000);
    WRITE_YEAR_DATA();
}

void DS3231::set_date(uint8_t day, uint8_t mon, int year)
{
    if (day < 1 || day > 31 ||
        mon < 1 || mon > 12 ||
        year < 2000 || year > 2099)
        return;

    ENCODE_DAY(day);
    ENCODE_MON(mon);
    ENCODE_YEAR(year - 2000);
    WRITE_DATE_DATA();
}

void DS3231::set_delay(uint sleep_mins)
{
	uint8_t wakeup_min;
	uint8_t reg;
	uint8_t send_t[3];

	send_t[0] = 0;
    reg = DS3231_STATUS_ADDR;
	write_bytes(reg, send_t, 1);

	send_t[0] = DS3231_CONTROL_INTCN;
    reg = DS3231_CONTROL_ADDR;
    write_bytes(reg, send_t, 1);


	wakeup_min = (get_min() / sleep_mins + 1) * sleep_mins;
	if (wakeup_min > 59) {
		wakeup_min -= 60;
	}


	 uint8_t t[3] = { wakeup_min, 0,  0 };
	 uint8_t i;
	 reg=DS3231_ALARM2_ADDR;
	 uint8_t flags[4] = { 0, 1, 1, 1 };

	  for (i = 0; i <= 2; i++) {
			if (i == 2) {
				send_t[i] = _encode_gen(t[2]) | (flags[2] << 7) | (flags[3] << 6);
			} else
				send_t[i] = _encode_gen(t[i]) | (flags[i] << 7);
		}

	 write_bytes(reg, send_t, 3);

	 send_t[0] = DS3231_CONTROL_INTCN | DS3231_CONTROL_A2IE | DS3231_CONTROL_BBSQW;
	 reg = DS3231_CONTROL_ADDR;
	 write_bytes(reg, send_t, 1);

}

void DS3231::clearAlarm(void){
    uint8_t reg_val;

    reg_val = getAddr(DS3231_STATUS_ADDR) & ~DS3231_STATUS_A2F;
    setAddr(DS3231_STATUS_ADDR,  reg_val);
}

uint8_t DS3231::getAddr(const uint8_t addr){
    uint8_t rv;

    i2c_write_blocking(_i2c, DS3231_ADDR, &addr, 1, true);

    i2c_read_blocking(_i2c, DS3231_ADDR, &rv, 1, false);

    return rv;
}

void DS3231::setAddr(const uint8_t addr, const uint8_t val){
    write_bytes(addr, (uint8_t*) &val, 1);
}


void DS3231::write_bytes(uint8_t reg, uint8_t *buf, int len) {
    uint8_t buffer[len + 1];

   //printf("Write %X, ", reg);
    buffer[0] = reg;
    for(int x = 0; x < len; x++) {
      buffer[x + 1] = buf[x];
      //printf("%X, ",  buffer[x + 1]);
    }
    //printf("\n");
    i2c_write_blocking(_i2c,DS3231_ADDR, buffer, len + 1, false);
};
