#include "signalflow/core/core.h"
#include "signalflow/core/graph-monitor.h"
#include "signalflow/core/graph.h"
#include "signalflow/node/node.h"
#include "signalflow/node/oscillators/constant.h"

#include "signalflow/patch/patch.h"

#include "signalflow/node/io/output/abstract.h"
#include "signalflow/node/io/output/ios.h"
#include "signalflow/node/io/output/soundio.h"

#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef HAVE_SNDFILE
#include <sndfile.h>
#endif

namespace signalflow
{

AudioGraph *shared_graph = NULL;

AudioGraph::AudioGraph(SignalFlowConfig *config,
                       NodeRef output_device)
{
    signalflow_init();

    if (shared_graph)
    {
        throw graph_already_created_exception();
    }
    shared_graph = this;

    if (config)
    {
        this->config = *config;
    }

    if (output_device)
    {
        this->output = output_device;
    }
    else
    {
        this->output = new AudioOut(this->config.get_output_device_name(),
                                    this->config.get_sample_rate(),
                                    this->config.get_output_buffer_size());
        if (!this->output)
        {
            throw std::runtime_error("AudioGraph: Couldn't find audio output device");
        }
    }

    AudioOut_Abstract *audio_out = (AudioOut_Abstract *) this->output.get();

    if (audio_out->get_sample_rate() == 0)
    {
        throw std::runtime_error("AudioGraph: Audio output device has zero sample rate");
    }

    this->sample_rate = audio_out->get_sample_rate();
    this->node_count = 0;
    this->_node_count_tmp = 0;
    this->cpu_usage = 0.0;
    this->monitor = NULL;
}

void AudioGraph::start()
{
    AudioOut_Abstract *audioout = (AudioOut_Abstract *) this->output.get();
    audioout->start();
}

void AudioGraph::stop()
{
    AudioOut_Abstract *audioout = (AudioOut_Abstract *) this->output.get();
    audioout->stop();
}

void AudioGraph::clear()
{
    AudioOut_Abstract *audioout = (AudioOut_Abstract *) this->output.get();
    auto inputs = audioout->get_inputs();
    for (auto input : inputs)
    {
        audioout->remove_input(input);
    }
}

AudioGraph::~AudioGraph()
{
    AudioOut_Abstract *audioout = (AudioOut_Abstract *) this->output.get();
    audioout->destroy();
    shared_graph = NULL;
}

void AudioGraph::wait(float time)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double t0 = tv.tv_sec + tv.tv_usec / 1000000.0;

    while (true)
    {
        usleep(10000);
        if (time)
        {
            gettimeofday(&tv, NULL);
            double t1 = tv.tv_sec + tv.tv_usec / 1000000.0;
            double dt = t1 - t0;
            if (dt > time)
            {
                break;
            }
        }
    }
}

void AudioGraph::render_subgraph(const NodeRef &node, int num_frames)
{
    /*------------------------------------------------------------------------
     * If this node has already been processed this timestep, return.
     *-----------------------------------------------------------------------*/
    if (node->has_rendered)
    {
        return;
    }

    if (!(node->inputs.size() > 0 || node->name == "constant" || node->name == "audioout" || node->name == "audioin"))
    {
        signalflow_debug("Node %s has no registered inputs", node->name.c_str());
    }

    /*------------------------------------------------------------------------
     * Pull our inputs before we generate our own outputs.
     *-----------------------------------------------------------------------*/
    for (auto param : node->inputs)
    {
        NodeRef param_node = *(param.second);
        if (param_node)
        {
            this->render_subgraph(param_node, num_frames);

            /*------------------------------------------------------------------------
             * Automatic input upmix.
             *
             * If the input node produces less channels than demanded, automatically
             * up-mix its output by replicating the existing channels. This allows
             * operations between multi-channel and mono-channel inputs to work
             * seamlessly without any additional implementation within the node
             * itself (for example, Multiply(new Sine(440), new LinearPanner(2, ...)))
             *
             * A few nodes must prevent automatic input up-mixing from happening.
             * These include ChannelArray and AudioOut.
             *
             * Some partially-initialised nodes (e.g. BufferPlayer with a not-yet-
             * populated Buffer) will have num_output_channels == 0. Don't try to
             * upmix a void output.
             *-----------------------------------------------------------------------*/
            if (param_node->get_num_output_channels() < node->get_num_input_channels() && !node->no_input_upmix && param_node->get_num_output_channels() > 0)
            {
                signalflow_debug("Upmixing %s (%s wants %d channels, %s only produces %d)", param_node->name.c_str(),
                                 node->name.c_str(), node->get_num_input_channels(), param_node->name.c_str(), param_node->get_num_output_channels());

                /*------------------------------------------------------------------------
                 * If we generate 2 channels but have 6 channels demanded, repeat
                 * them: [ 0, 1, 0, 1, 0, 1 ]
                 *-----------------------------------------------------------------------*/
                for (int out_channel_index = param_node->get_num_output_channels();
                     out_channel_index < node->get_num_input_channels();
                     out_channel_index++)
                {
                    int in_channel_index = out_channel_index % param_node->get_num_output_channels();
                    memcpy(param_node->out[out_channel_index],
                           param_node->out[in_channel_index],
                           num_frames * sizeof(sample));
                }
            }
        }
    }

    node->_process(node->out, num_frames);

    if (node->name != "constant")
    {
        node->has_rendered = true;
        this->_node_count_tmp++;
    }
}

void AudioGraph::reset_graph()
{
    /*------------------------------------------------------------------------
     * Disconnect any nodes and patches that are scheduled to be removed.
     *-----------------------------------------------------------------------*/
    for (auto node : nodes_to_remove)
    {
        node->disconnect_outputs();
    }
    nodes_to_remove.clear();

    for (auto patch : patches_to_remove)
    {
        for (auto patchref : patches)
        {
            if (patchref.get() == patch)
            {
                patches.erase(patchref);
                break;
            }
        }
    }
    this->patches_to_remove.clear();

    /*------------------------------------------------------------------------
     * Clear the record of processed nodes.
     *-----------------------------------------------------------------------*/
    this->_node_count_tmp = 0;
    this->reset_subgraph(this->output);
    for (auto node : this->scheduled_nodes)
    {
        this->reset_subgraph(node);
    }
}

void AudioGraph::reset_subgraph(NodeRef node)
{
    node->has_rendered = false;
    for (auto input : node->inputs)
    {
        NodeRef input_node = *(input.second);
        if (input_node && input_node->has_rendered)
        {
            this->reset_subgraph(input_node);
        }
    }
}

void AudioGraph::render(int num_frames)
{
    /*------------------------------------------------------------------------
     * Timestamp the start of processing to measure CPU usage.
     *-----------------------------------------------------------------------*/
    double t0 = signalflow_timestamp();

    this->reset_graph();
    this->render_subgraph(this->output, num_frames);
    for (auto node : this->scheduled_nodes)
    {
        this->render_subgraph(node, num_frames);
    }
    this->node_count = this->_node_count_tmp;
    signalflow_debug("AudioGraph: pull %d frames, %d nodes", num_frames, this->node_count);

    /*------------------------------------------------------------------------
     * Calculate CPU usage (approximately) by measuring the % of time
     * within the audio I/O callback that was used for processing.
     *-----------------------------------------------------------------------*/
    double t1 = signalflow_timestamp();
    double dt = t1 - t0;
    double t_max = (double) num_frames / this->sample_rate;
    this->cpu_usage = dt / t_max;
}

void AudioGraph::render_to_buffer(BufferRef buffer, int block_size)
{
    // TODO get_num_output_channels()
    int channel_count = buffer->get_num_channels();
    if (channel_count > this->output->num_input_channels)
    {
        throw std::runtime_error("Buffer cannot have more channels than the audio graph (" + std::to_string(channel_count) + " != " + std::to_string(this->output->num_input_channels) + ")");
    }
    int block_count = ceilf((float) buffer->get_num_frames() / block_size);

    for (int block_index = 0; block_index < block_count; block_index++)
    {
        int block_frames = block_size;
        if (block_index == block_count - 1 && buffer->get_num_frames() % block_size > 0)
        {
            block_frames = buffer->get_num_frames() % block_size;
        }
        this->render(block_frames);
        for (int channel_index = 0; channel_index < channel_count; channel_index++)
        {
            memcpy(buffer->data[channel_index] + (block_index * block_size),
                   this->output->out[channel_index],
                   block_frames * sizeof(sample));
        }
    }
}

NodeRef AudioGraph::get_output()
{
    return this->output;
}

NodeRef AudioGraph::add_node(NodeRef node)
{
    this->scheduled_nodes.insert(node);
    return node;
}

void AudioGraph::remove_node(NodeRef node)
{
    this->scheduled_nodes.erase(node);
}

void AudioGraph::play(PatchRef patch)
{
    AudioOut_Abstract *output = (AudioOut_Abstract *) (this->output.get());
    output->add_input(patch->output);
    this->patches.insert(patch);
}

void AudioGraph::play(NodeRef node)
{
    AudioOut_Abstract *output = (AudioOut_Abstract *) this->output.get();
    output->add_input(node);
}

void AudioGraph::stop(PatchRef patch)
{
    this->stop(patch.get());
}

void AudioGraph::stop(Patch *patch)
{
    patches_to_remove.insert(patch);
    nodes_to_remove.insert(patch->output);
}

void AudioGraph::stop(NodeRef node)
{
    nodes_to_remove.insert(node);
}

void AudioGraph::start_recording(const std::string &filename)
{
#ifdef HAVE_SNDFILE

    SF_INFO info;
    memset(&info, 0, sizeof(SF_INFO));
    int num_frames = this->output->num_input_channels;
    info.frames = this->get_output_buffer_size();
    info.channels = this->output->get_num_input_channels();
    info.samplerate = (int) this->sample_rate;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE *sndfile = sf_open(filename.c_str(), SFM_WRITE, &info);

    if (!sndfile)
    {
        throw std::runtime_error(std::string("Failed to write soundfile (") + std::string(sf_strerror(NULL)) + ")");
    }
#else

    throw std::runtime_error("Cannot record AudioGraph because SignalFlow was compiled without libsndfile support");

#endif
}

void AudioGraph::stop_recording()
{
}

void AudioGraph::show_structure()
{
    std::cout << "AudioGraph" << std::endl;
    this->show_structure(this->output, 0);
}

void AudioGraph::show_structure(NodeRef &root, int depth)
{
    std::cout << std::string(depth * 2, ' ');
    std::cout << " * " << root->name << std::endl;
    for (auto pair : root->inputs)
    {
        std::cout << std::string((depth + 1) * 2 + 1, ' ');

        NodeRef param_node = *(pair.second);
        if (param_node->name == "constant")
        {
            Constant *constant = (Constant *) (param_node.get());
            std::cout << pair.first << ": " << constant->value << std::endl;
        }
        else
        {
            std::cout << pair.first << ":" << std::endl;
            this->show_structure(param_node, depth + 1);
        }
    }
}

void AudioGraph::show_status(float frequency)
{
    if (frequency > 0)
    {
        if (this->monitor)
        {
            throw std::runtime_error("AudioGraph is already polling state");
        }
        this->monitor = new AudioGraphMonitor(this, frequency);
        this->monitor->start();
    }
}

int AudioGraph::get_sample_rate()
{
    return this->sample_rate;
}

void AudioGraph::set_sample_rate(int sample_rate)
{
    if (sample_rate <= 0)
    {
        throw std::runtime_error("Sample rate cannot be <= 0");
    }
    this->sample_rate = sample_rate;
}

int AudioGraph::get_output_buffer_size()
{
    AudioOut_Abstract *output = (AudioOut_Abstract *) this->output.get();
    return output->get_buffer_size();
}

int AudioGraph::get_node_count()
{
    return this->node_count;
}

int AudioGraph::get_patch_count()
{
    return (int) this->patches.size();
}

float AudioGraph::get_cpu_usage()
{
    return this->cpu_usage;
}

SignalFlowConfig &AudioGraph::get_config()
{
    return this->config;
}

}
