# PlugPerf Project Backlog

## Project Summary
PlugPerf is a performance benchmarking and optimization tool tailored for audio plugins, with an initial focus on JUCE and VST3 plugin formats. The goal is to identify performance bottlenecks, validate improvements, and provide actionable insights to plugin developers for creating more efficient and responsive audio software.

---

## Milestones

### Milestone A: Initial Setup and Baseline Benchmarking
- [x] Define project scope and objectives
- [x] Set up repository and development environment
- [x] Integrate JUCE and VST3 plugin hosts for testing
- [x] Develop baseline benchmarking tests for CPU, memory, and latency
- [x] Document initial test results and identify key performance metrics
- [x] Add statistical validation (std dev, coefficient of variation) to measurements
- [x] Switch to real-time processing mode (matches Plugin Doctor methodology)
- [x] Cross-validate measurements with Plugin Doctor using local plugins (95% match achieved)
- [x] Create synthetic test plugin with controllable CPU load for validation
- [x] Implement measurement consistency checks (repeatability, outlier detection)
- [x] Generate visualization charts (PNG) showing performance across buffer sizes
- [x] Add platform baseline measurement (passthrough/empty plugin overhead)
- [x] Add support for multiple bit depths (32f, 64f)
- [x] Add support for configurable sample rates

### Pre-Milestone Remediation
- [x] Clear MIDI buffers between warmup and timed iterations to avoid compounding plugin-generated events.
- [x] Negotiate plugin bus layouts so requested channel counts are honoured or rejected explicitly.
- [x] Validate double-precision support before running 64f benchmarks and warn/fallback when unsupported.
- [x] Make packaging script core-count detection portable across macOS, Linux, and Windows.
- [DEFERED  ] Add regression coverage using a MIDI-emitting plugin to ensure buffer-reset behaviour stays intact.
- [x] Research and implement proper real-time audio thread priority (reference Tracktion Engine's tracktion::graph)
  - Implemented: Process::setPriority(RealtimePriority) on main thread
  - Rationale: Simpler and more reliable for CLI app than separate thread
  - Research documented in docs/REALTIME_THREAD_RESEARCH.md
  - Result: Consistent measurements with elevated priority

### Milestone B: Performance Sweeper Core Implementation
- [x] Implement parameter setting via command line (--param name=value)
  - Created `plugparams` tool for parameter inspection and manipulation
  - Supports querying all parameters with type classification (Boolean/Discrete/Continuous)
  - Shows ranges, possible values, and current settings
  - Allows setting parameters by index, name, or ID
  - JSON output for programmatic use
  - Verbose mode for detailed parameter information
- [ ] Add support for loading plugin presets (VST3 .vstpreset files)
- [ ] Add ability to save/restore plugin state for reproducible tests
- [ ] Collect system metadata (CPU model, core count, RAM, OS version)
- [ ] Collect plugin metadata (name, version, vendor, format, unique ID)
- [ ] Capture test configuration metadata (sample rate, buffer sizes, iterations, etc.)
- [ ] Design and implement the performance sweeper engine
- [ ] Develop automated test runners for batch plugin evaluation
- [ ] Create detailed profiling and reporting tools
- [ ] Validate sweeper accuracy against known benchmarks
- [ ] Optimize test execution time without sacrificing accuracy

### Milestone C: User Interface and Usability Enhancements
- [ ] Design a clean, intuitive UI for configuring tests and viewing results
- [ ] Implement real-time monitoring and visualization of performance data
- [ ] Add support for exporting reports in multiple formats (CSV, JSON, PDF)
- [ ] Integrate plugin selection and management features
- [ ] Provide user documentation and tutorials

### Milestone D: Extended Plugin and Platform Support
- [ ] Expand support to additional plugin formats (e.g., AU, AAX)
- [ ] Add compatibility with macOS, Windows, and Linux platforms
- [ ] Implement cross-platform build and deployment pipelines
- [ ] Test and validate performance sweeper on a variety of host DAWs
- [ ] Collect user feedback and iterate on platform support issues

### Milestone E: Results Collection and Server Integration
- [ ] Design SQL database schema for storing benchmark results
- [ ] Implement JSON export format with complete metadata
- [ ] Add HTTP/REST API client for uploading results to server
- [ ] Include authentication and user identification for result submissions
- [ ] Implement result validation and duplicate detection on server side
- [ ] Create web interface for browsing and comparing benchmark results
- [ ] Add privacy controls for sharing results publicly vs. privately
- [ ] Implement result aggregation and statistical analysis across submissions

### Milestone F: Advanced Features and Future Enhancements
- [ ] Develop AI-driven performance optimization suggestions
- [ ] Integrate with continuous integration (CI) systems for automated testing
- [ ] Add support for real-time plugin parameter automation tracking
- [ ] Implement historical performance tracking and trend analysis
- [ ] Plan for community contributions and plugin developer integrations

---

## Notes and Future Considerations

- **Validation**: Establish rigorous validation procedures to ensure benchmarking accuracy across different hardware configurations.
- **Platform Support**: Prioritize stable support on major OS platforms before expanding to niche environments.
- **Plugin Ecosystem**: Monitor emerging plugin standards and update PlugPerf accordingly.
- **Performance Metrics**: Explore additional metrics such as power consumption and thread concurrency.
- **Community Engagement**: Develop forums or channels for user feedback, bug reports, and feature requests.
- **Security**: Ensure safe execution of third-party plugins to prevent crashes or security vulnerabilities during benchmarking.
