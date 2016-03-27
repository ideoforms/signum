# Change Log

## [HEAD](https://github.com/ideoforms/signal/tree/HEAD)

[Full Changelog](https://github.com/ideoforms/signal/compare/v0.1.1...HEAD)

**New features:**

- Node: Saw [\#25](https://github.com/ideoforms/signal/issues/25)
- Node: EQ [\#19](https://github.com/ideoforms/signal/issues/19)

**Fixed bugs:**

- Add node-constant operations where node is the RHS operand [\#30](https://github.com/ideoforms/signal/issues/30)
- Prevent a node from being stepped multiple times per tick [\#29](https://github.com/ideoforms/signal/issues/29)

## [v0.1.1](https://github.com/ideoforms/signal/tree/v0.1.1) (2016-03-23)
[Full Changelog](https://github.com/ideoforms/signal/compare/v0.1.0...v0.1.1)

**New features:**

- Incorporate JSON parser library [\#7](https://github.com/ideoforms/signal/issues/7)
- SynthRef [\#3](https://github.com/ideoforms/signal/issues/3)

**Fixed bugs:**

- Correct loading of multichannel buffers [\#18](https://github.com/ideoforms/signal/issues/18)

## v0.0.1 (2016-01-01)

**New features:**

- Basic framework with `Node` and `Graph` classes
- Automatic reference counting with `NodeRef` as a subclass of `std::shared_ptr`
- Synthesis nodes: `Sine`, `Square`, `Granulator`, `Wavetable`
- Control nodes: `Tick`, `Line`, `ASR`
- Effects nodes: `Delay`, `Gate`, `Pan`, `Resample`, `Width`
- Chance nodes: `Dust`, `Noise`
- `Buffer` object to store fixed buffers of sample data
- `RingBuffer` for circular buffering
- Global `Registry` to store and instantiate `Node` objects by identifier
- Fast Fourier Transform (`FFT`) and inverse FFT (`IFFT`) nodes with Accelerate optimisation
- Operator overloading for node addition, multiplication, addition and subtraction
- Node output monitoring
- Cross-platform I/O with `libsoundio` 
- Audio file I/O with `libsndfile`
- waf-based build system
- Raspberry Pi support


\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*