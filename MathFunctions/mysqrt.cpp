
#include "MathFunctions.h"
#include "TutorialConfig.h"

double mysqrt(double value)
{
	double result;

	// if we have both log and exp then use them
	#if defined (HAVE_LOG) && defined (HAVE_EXP)
		result = exp(log(value)*0.5);
	#else // otherwise use an iterative approach
		result = sqrt(value);
	#endif;

	return result;
}