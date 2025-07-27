# Real-Time Audio Streaming System with Opus Codec

## Overview
A complete TCP-based audio streaming system using Opus codec for real-time audio compression and transmission.

## System Architecture
- **Source Client**: Captures audio from microphone or WAV file, encodes with Opus, streams via TCP
- **Server**: Receives encoded audio streams and redistributes to clients
- **Playback Client**: Receives encoded audio from server, decodes, and plays through speakers

## Features
- Real-time audio streaming with low latency
- Opus codec for efficient compression (configurable bitrate)
- Circular buffer for jitter management
- Cross-platform support (macOS Intel/M1, Linux, Windows)
- Support for both live microphone and WAV file input

## Building
```bash
make all
# or use the provided build scripts:
./build3.sh  # for tcp_source_client
./build4.sh  # for tcp_client
```

## Usage
### Local Testing
1. Start the test server:
```bash
./tcp_test_c 128000 5002
```

2. Start the playback client:
```bash
./tcp_client localhost 5002
```

3. Start the source client:
```bash
./tcp_source_client 128000 localhost 5001 svega.wav
```

### Network Streaming
Using the instructor's server:
```bash
# Source client
./tcp_source_client 128000 arlabs.dyndns.org 5001 svega.wav

# Playback client
./tcp_client arlabs.dyndns.org 5002
```

## Dependencies
- PortAudio: Cross-platform audio I/O
- Opus: Audio codec
- libsndfile: WAV file reading
- pthread: Threading support