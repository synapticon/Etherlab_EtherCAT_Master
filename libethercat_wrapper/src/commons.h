/*
 * common.h
 *
 * Collection of functions to use in different timing programs.
 *
 * 2016-12-11 Frank Jeschke <frank@fjes.de>
 */


#ifndef _COMMONS_H
#define _COMMONS_H

#include <stddef.h>
#include <sys/time.h>

/*
 * Calculate the difference between start and end timespec with respect of
 * the seconds advance.
 *
 * This function originates from the gist:
 * https://gist.github.com/diabloneo/9619917:w
 */
void timespec_diff(struct timespec *start,
		   struct timespec *stop,
		   struct timespec *result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	} else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}

/*
 * Calculate the mean for every new value so the current mean is always up to
 * date.
 *
 * @param mean     to current mean value
 * @param current  the current measurement
 * @param count    count of this value (starts with 0)
 * @return the new mean value
 */

double calc_mean(double mean, double current, size_t count)
{
	double K = 1.0 / (count + 1);
	return (mean + K * (current - mean));
}

#endif /* _COMMONS_H */
