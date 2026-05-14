# CadGoose Profiling Tools

This folder contains scripts for profiling CadGoose performance.

## Usage

### CPU Profile (Time Profiler)
```bash
./profile_cpu.sh [PID] [DURATION_SECONDS]
```
Profiles CPU usage for specified duration (default: 30 seconds).
Uses `xctrace record` with Time Profiler template.

### Memory Profile (Allocations)
```bash
./profile_memory.sh [PID] [DURATION_SECONDS]
```
Profiles memory allocations for specified duration (default: 30 seconds).
Uses `xctrace record` with Allocations template.

### Soak Test with Profiling
```bash
./soak_profile.sh [DURATION_MINUTES]
```
Runs CadGoose for specified duration while continuously profiling.
Saves results to `soak_results_YYYYMMDD_HHMMSS.md`.

### Quick CPU Check
```bash
./quick_profile.sh [DURATION_SECONDS]
```
Simple 60-second CPU profile, saves to quick_profile.md

## Requirements
- Xcode command line tools (`xctrace`)
- CadGoose must be running
- Profile with PID of running CadGoose process