# Real-Time Audio Thread Research

## Goal
Implement DAW-equivalent real-time audio thread priority for accurate plugin performance measurements.

## Current Implementation

```cpp
Process::setPriority(Process::RealtimePriority);  // Process-level priority
// Measurements run on main thread
```

**Limitations:**
- Sets priority for entire process, not specific thread
- Runs on main thread, not dedicated audio thread
- No time-constraint scheduling (macOS/iOS)
- Doesn't match DAW audio callback behavior

## DAW Behavior (Research Findings)

### 1. JUCE AudioDeviceManager Approach

From JUCE documentation:
- `AudioIODevice` calls `audioDeviceIOCallbackWithContext()` on **its own high-priority audio thread**
- Thread priority is set automatically by the audio device
- Uses platform-specific real-time scheduling

### 2. JUCE Thread::RealtimeOptions (Modern JUCE API)

JUCE provides `Thread::RealtimeOptions` for creating real-time threads:

```cpp
Thread::RealtimeOptions options;
options.withPriority(10)  // 0-10, where 10 is highest (POSIX)
       .withProcessingTimeMs(expectedMs)  // macOS/iOS only
       .withMaximumProcessingTimeMs(maxMs)  // macOS/iOS only
       .withPeriodMs(periodMs);  // macOS/iOS only

// Or for audio processing:
options.withApproximateAudioProcessingTime(sampleRate, blockSize);

thread.startRealtimeThread(options);
```

**Platform-specific behavior:**
- **macOS/iOS**: Uses Mach `THREAD_TIME_CONSTRAINT_POLICY`
  - Sets time constraints for guaranteed CPU time
  - Specifies: processing time, max processing time, period
- **Linux**: Uses POSIX `SCHED_FIFO` with priority 0-10
- **Windows**: Uses high-priority thread scheduling
- **Android**: Uses OpenSL/Oboe super high priority threads

### 3. macOS Time-Constraint Policy

From Apple's Kernel Programming Guide:

```cpp
thread_time_constraint_policy_data_t policy;
policy.period      = HZ / callbackHz;  // Time between callbacks
policy.computation = HZ / 3300;        // Expected processing time
policy.constraint  = HZ / 2200;        // Maximum processing time
policy.preemptible = TRUE;             // Can be interrupted

thread_policy_set(mach_thread_self(),
                  THREAD_TIME_CONSTRAINT_POLICY,
                  (thread_policy_t)&policy,
                  THREAD_TIME_CONSTRAINT_POLICY_COUNT);
```

**Key parameters:**
- `period`: Time between thread wake-ups (e.g., buffer period)
- `computation`: Expected processing time per wake-up
- `constraint`: Maximum allowed processing time (must be > computation)
- `preemptible`: Whether thread can be interrupted

**Example for 48kHz, 512 samples:**
- Period: 512/48000 = 10.67ms
- Computation: ~40% of period = 4.27ms
- Constraint: ~60% of period = 6.4ms

### 4. Tracktion Engine Approach

From tracktion_engine documentation:

**tracktion::graph module:**
- Lock-free multi-threaded audio processing
- Dedicated real-time threads for audio graph processing
- No mutexes or allocations in audio thread
- Uses JUCE's thread priority mechanisms

**Key principles:**
- Separate audio thread from UI/main thread
- Real-time priority set on audio callback thread
- Lock-free communication between threads
- Time-constraint scheduling on macOS

## Recommended Implementation

### Phase 1: Dedicated Audio Thread

Create a dedicated thread for measurements using JUCE's Thread class:

```cpp
class BenchmarkThread : public Thread
{
public:
    BenchmarkThread() : Thread("PlugPerf Benchmark") {}
    
    void run() override
    {
        // Run measurements here
        // This runs on dedicated thread with real-time priority
    }
};

// Usage:
BenchmarkThread benchThread;
Thread::RealtimeOptions rtOptions;
rtOptions.withPriority(10)
         .withApproximateAudioProcessingTime(sampleRate, blockSize);
benchThread.startRealtimeThread(rtOptions);
benchThread.waitForThreadToExit(-1);
```

### Phase 2: Time-Constraint Scheduling (macOS/iOS)

For macOS, use `withApproximateAudioProcessingTime()`:

```cpp
Thread::RealtimeOptions rtOptions;
rtOptions.withPriority(10)
         .withApproximateAudioProcessingTime(48000.0, 512)
         .withPeriodMs(512.0 / 48000.0 * 1000.0);  // ~10.67ms
```

This automatically calculates appropriate time constraints.

### Phase 3: Verify Thread Priority

Add diagnostic output to verify thread is running with correct priority:

```cpp
#if JUCE_MAC || JUCE_IOS
    // Check if time-constraint policy is active
    thread_time_constraint_policy_data_t policy;
    mach_msg_type_number_t count = THREAD_TIME_CONSTRAINT_POLICY_COUNT;
    boolean_t get_default = FALSE;
    
    if (thread_policy_get(mach_thread_self(),
                          THREAD_TIME_CONSTRAINT_POLICY,
                          (thread_policy_t)&policy,
                          &count,
                          &get_default) == KERN_SUCCESS)
    {
        std::cerr << "Real-time thread active with time constraints\n";
    }
#endif
```

## Implementation Plan

### Step 1: Refactor Measurement Loop
- Move `measureOne()` logic into dedicated thread
- Pass parameters via thread-safe structure
- Return results via thread-safe mechanism

### Step 2: Create BenchmarkThread Class
- Inherit from `juce::Thread`
- Implement `run()` method with measurement loop
- Handle warmup and timed iterations on RT thread

### Step 3: Configure Real-Time Options
- Calculate appropriate time constraints based on buffer size
- Set priority to maximum (10)
- Use `withApproximateAudioProcessingTime()` for macOS

### Step 4: Validate Improvements
- Compare measurements before/after RT thread
- Check CV (coefficient of variation) improvement
- Verify thread priority with platform tools

### Step 5: Cross-Platform Testing
- Test on macOS (time-constraint policy)
- Test on Linux (SCHED_FIFO)
- Test on Windows (high-priority threads)

## Expected Benefits

1. **More accurate measurements** - Matches DAW audio callback behavior
2. **Lower variance** - Real-time scheduling reduces jitter
3. **Better cache locality** - Dedicated thread improves cache behavior
4. **Guaranteed CPU time** - Time constraints prevent preemption
5. **Industry standard** - Matches how all professional DAWs work

## References

- [JUCE AudioIODeviceCallback](https://docs.juce.com/master/classAudioIODeviceCallback.html)
- [JUCE Thread::RealtimeOptions](https://docs.juce.com/master/structThread_1_1RealtimeOptions.html)
- [Apple Mach Scheduling](https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/scheduler/scheduler.html)
- [Tracktion Engine](https://github.com/Tracktion/tracktion_engine)
- [JUCE Thread Class](https://docs.juce.com/master/classThread.html)

## Next Steps

1. Create prototype implementation
2. Benchmark against current approach
3. Validate with Plugin Doctor cross-check
4. Document performance improvements
5. Update README with technical details
