from signalflow import AudioGraph, Buffer, Sine, Line, Constant
from . import process_tree, count_zero_crossings
import pytest
import numpy as np

def test_graph():
    graph = AudioGraph()
    assert graph
    del graph

def test_graph_sample_rate():
    graph = AudioGraph()
    assert graph.sample_rate > 0
    graph.sample_rate = 1000
    assert graph.sample_rate == 1000

    with pytest.raises(Exception):
        graph.sample_rate = 0

    buf = Buffer(1, 1000)
    sine = Sine(10)
    process_tree(sine, buf)
    assert count_zero_crossings(buf.data[0]) == 10

    del graph

def test_graph_cyclic():
    graph = AudioGraph()
    graph.sample_rate = 1000
    line = Line(0, 1, 1)
    m1 = line * 1
    m2 = line * 2
    add = m1 + m2
    graph.play(add)
    buf = Buffer(1, 1000)
    graph.render_to_buffer(buf)
    assert np.all(np.abs(buf.data[0] - np.linspace(0, 3, graph.sample_rate)) < 0.00001)
    del graph

def test_graph_render():
    graph = AudioGraph()
    sine = Sine(440)
    graph.play(sine)
    with pytest.raises(RuntimeError):
        graph.render(441000)
    del graph

def test_graph_add_remove_node():
    graph = AudioGraph()
    constant = Constant(123)
    graph.add_node(constant)
    buffer = Buffer(1, 1024)
    graph.render_to_buffer(buffer)
    assert np.all(buffer.data[0] == 0.0)
    assert np.all(constant.output_buffer[0] == 123)
    del graph

    graph = AudioGraph()
    constant = Constant(123)
    graph.add_node(constant)
    graph.remove_node(constant)
    graph.render(1024)
    assert np.all(constant.output_buffer[0] == 0)
    del graph