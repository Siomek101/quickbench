# quickbench
A benchmark tool that benchmarks:
- CPU (integer, float, single/multithread)
- RAM (speed, latency)
and shows important info about the OS.

## usage:
Show output in terminal:
```bash
./bench
```

Show output in markdown text (to copy):
```bash
./bench --md
// or to a file
./bench --md > output.md
```

Show output in json:
```bash
./bench --json
// or to a file:
./bench --json > output.json
```

Stress test:
```bash
./bench --stress [duration in full seconds]
// example
./bench --stress 600 // stress test for 10 minutes
```
