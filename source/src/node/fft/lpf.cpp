#include "signalflow/core/graph.h"
#include "signalflow/node/fft/lpf.h"

namespace signalflow
{

FFTLPF::FFTLPF(NodeRef input, NodeRef frequency)
    : FFTOpNode(input), frequency(frequency)
{
    this->name = "fft-lpf";
    this->create_input("frequency", this->frequency);
}

void FFTLPF::process(Buffer &out, int num_frames)
{
    FFTNode *fftnode = (FFTNode *) this->input.get();
    this->num_hops = fftnode->num_hops;

    /*------------------------------------------------------------------------
     * Calculate a normalised cutoff value [0, 1]
     *-----------------------------------------------------------------------*/
    float cutoff = this->frequency->out[0][0];
    float cutoff_norm = (float) cutoff / (this->graph->get_sample_rate() / 2.0);

    /*------------------------------------------------------------------------
     * Calculate the bin above which we want to set magnitude = 0
     *-----------------------------------------------------------------------*/
    int cutoff_bin = this->num_bins * cutoff_norm;

    for (int hop = 0; hop < this->num_hops; hop++)
    {
        /*------------------------------------------------------------------------
         * IMPORTANT: FFT nodes must process fft_size frames and ignore
         * num_frames (num_frames indicates how many audio frames have been
         * passed this block, but the FFT node buffers windows of `fft_size`
         * frames.
         *-----------------------------------------------------------------------*/
        for (int frame = 0; frame < this->fft_size; frame++)
        {
            if (frame < fft_size / 2)
            {
                if (frame > cutoff_bin)
                    out[hop][frame] = 0.0;
                else
                    out[hop][frame] = input->out[hop][frame];
            }
            else
            {
                out[hop][frame] = input->out[hop][frame];
            }
        }
    }
}

}
