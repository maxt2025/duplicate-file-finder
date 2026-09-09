**# Duplicate File Finder**
A command-line utility written in **C** for finding duplicate files.

The program recursively scans selected user directories, detects files with identical content, and calculates how much disk space could potentially be recovered by removing duplicates.

## Features
- Recursive directory scanning
- File grouping by size
- Hash table for efficient candidate grouping
- Byte-by-byte duplicate verification
- Human-readable file sizes (B, KB, MB, GB, TB)
- Calculation of potential disk space savings
- Support multiple directories in a single run
- Symbolic links are not followed


## Requirements
- Linux or macOS


## Build

Using Make:
```bash
make
```
Or compile directly:
```bash
gcc -Wall -Wextra -pedantic main.c -o dupfinder
```

## Usage

```bash
./dupfinder "$HOME/Documents" "$HOME/Downloads"
```

Currently, the program accepts the following directories:
~/Documents
~/Downloads
~/Pictures

Example:
./dupfinder "$HOME/Documents" "$HOME/Downloads"

Example output:

Group #1:
Size: 6.25 MB

/home/user/Documents/video.mp4
/home/user/Downloads/video-copy.mp4

Potential saving: 6.25 MB

Total duplicate data: 6.25 MB

## Duplicate Detection

Files are not considered duplicates based only on their hash.
The program first compares file sizes, then calculates an FNV-1a hash, and finally performs a byte-by-byte comparison of candidate files.
This prevents hash collisions from causing files with different contents to be reported as duplicates.

## Current Limitations
- Windows is not currently supported.
- Only Documents, Downloads and Pictures can be scanned.
- The program reports duplicates but does not delete them automatically