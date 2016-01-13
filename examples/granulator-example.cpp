#include <signum/signum.h>

/*------------------------------------------------------------------------
 * All objects are in the signum:: namespace.
 * Import this namespace for code brevity.
 *-----------------------------------------------------------------------*/
using namespace signum;

int main()
{
	/*------------------------------------------------------------------------
	 * Instantiate a single Graph object for all global audio processing.
	 *-----------------------------------------------------------------------*/
	Graph *graph = new Graph();

	/*------------------------------------------------------------------------
	 * Load audio buffer
	 *-----------------------------------------------------------------------*/
	Buffer *buffer = new Buffer("audio/gliss.aif");
	UnitRef dust = new Dust(1000.0);
	UnitRef pos = new Noise(0.3, false, 0, buffer->duration);
	UnitRef len = new Noise(100, false, 0.1, 0.5);
	UnitRef pan = new Noise(100, false);
	UnitRef granulator = new Granulator(buffer, dust, pos, len);
	Buffer *env_buf = new EnvelopeBufferHanning();
	granulator->set_param("pan", pan);
	granulator->set_buffer("envelope", env_buf);
	granulator = granulator * 0.25;

	/*------------------------------------------------------------------------
	 * The Graph can have multiple inputs, summed to output.
	 *-----------------------------------------------------------------------*/
	graph->output->add_input(granulator);

	/*------------------------------------------------------------------------
	 * Graph actually begins processing as soon as inputs are added.
	 * run() really just loops indefinitely.
	 *-----------------------------------------------------------------------*/
	graph->run();
}

