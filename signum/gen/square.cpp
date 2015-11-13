#include "square.h"

#include <math.h>
#include <stdio.h>

namespace signum::gen
{

Square::Square(float frequency, float width) : Unit()
{
	this->frequency = frequency;
	this->width = width;
	this->phase = 0.0;
}

sample Square::next()
{
	float s = (this->phase < this->width) ? 1 : -1;

	this->phase += 1.0 / (44100.0 / this->frequency);
	if (this->phase >= 1.0)
		this->phase -= 1.0;

	return s;
}

}
