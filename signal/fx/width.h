#pragma once

#include "../node.h"

namespace libsignal
{
	class Width : public Node
	{
		public:
			Width(NodeRef input, NodeRef width) :
				input(input), width(width)
			{
				this->num_input_channels = 1;
				this->num_output_channels = 2;
			}

			NodeRef input;
			NodeRef width;

			virtual void next(sample **out, int num_frames);
	};
}
