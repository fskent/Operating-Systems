#include "date.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 	Steve Kent, fkent, CIS 415 P0
 *	This is my own work. This ADT implementation is derived from Professor
 *	Sventek's stack.c example from the class slides. The destroy and
 *	create methods follow closely to what the stack.c implementation
 *	achieves.
 */

typedef struct d_data{
	int day;
	int month;
	int year;


} D_Data;
/*
 * duplicate() creates a duplicate of `d'
 * returns pointer to new Date structure if successful,
 *         NULL if not (memory allocation failure)
 */
static const Date* d_duplicate(const Date *d){
	const Date *new = d;
	if (new == NULL){
		return NULL;
	}
	return new;
	
};

/*
 * compare() compares two dates, returning <0, 0, >0 if
 * date1<date2, date1==date2, date1>date2, respectively
 */
static int d_compare(const Date *date1, const Date *date2){
	D_Data *data1 = (D_Data *)date1->self;
	D_Data *data2 = (D_Data *)date2->self;
	int yearComp = data1->year - data2->year;
	int monthComp = data1->month - data2->month;
	int dayComp = data1->day - data2->day;
	if (yearComp == 0){
		if (monthComp == 0){
			if (dayComp == 0){
				return 0;
			} else {
				return dayComp;
			}
		} else {
			return monthComp;
		}
	} else {
		return yearComp;
	}
	return 0;
};

/*
 * destroy() returns any storage associated with `d' to the system
 */
static void d_destroy(const Date *d){
	if (d != NULL){
		D_Data *data = (D_Data *)d->self;
		if (data != NULL){
			free(data);
		}
		free((void *)d);
	}	
};

static Date template = {NULL, d_duplicate, d_compare, d_destroy};

const Date *Date_create(char *datestr){
	Date *D = (Date *)malloc(sizeof(Date));
	int _day, _month, _year;
	if (D != NULL){
		D_Data *data = (D_Data *)malloc(sizeof(D_Data));
		if (data != NULL){
			sscanf(datestr, "%d/%d/%d", &_day, &_month, &_year);
			data->day = _day;
			data->month = _month;
			data->year = _year;
			*D = template;
			D->self = data;
		} else {
			free(D);
			D = NULL;
		}
	} 	
	return D;
}
