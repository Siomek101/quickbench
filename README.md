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

## compiling:

### Debian-based (Debian, Ubuntu, Linux Mint etc.)
```bash
sudo apt update
sudo apt install g++ build-essential libncurses5-dev libncursesw5-dev
./compile.sh
```

### Fedora, RHEL, CentOS, Rocky Linux
```bash
sudo dnf install gcc-c++ ncurses-devel
./compile.sh
```

### Arch Linux, Arch Linux-based
```bash
sudo pacman -Syu base-devel ncurses
./compile.sh
```

### openSUSE
```bash
sudo zypper install gcc-c++ ncurses-devel
./compile.sh
```

### Alpine Linux
```bash
apk add gcc ncurses-dev
./compile.sh
```

# Thanks to the contributors:
- @ProfP30
