# WAVLib: A Header-Only C++ Library for WAV File Handling

## Overview
WAVLib is a lightweight, header-only C++ library for reading, writing, and processing WAV files. It supports both mono and stereo audio, handling various bit depths with a focus on maintaining data integrity.

## Features
- **Read & Write WAV Files:** Load and save standard PCM WAV files.
- **Support for Multiple Bit Depths:** Works with 8-bit, 16-bit, and 32-bit PCM audio (24-bit not supported).
- **Automatic Sample Extraction:** Converts interleaved audio data into separate left and right channels.
- **Resampling:** Linear interpolation-based sample rate conversion.
- **Reencoding:** Convert WAV files between different bit depths while preserving amplitude ratios.

## Requirements
- C++17 or later
- A compiler supporting `<fstream>`, `<vector>`, `<cstring>`, and `<cmath>`.

## Usage

### Including the Library
Since WAVLib is header-only, simply include `wavlib.h` in your project:
```cpp
#include "wavlib.h"
```

### Reading a WAV File
```cpp
wav::WavFile wavFile;
if (wavFile.read("input.wav")) {
    std::cout << "Successfully loaded WAV file!" << std::endl;
}
```

### Converting to Typed Audio Data
```cpp
wav::WavData<int16_t> wavData(wavFile);
```

### Resampling Audio
```cpp
wav::WavData<int16_t> resampled = wav::resample(wavData, 22050);
```

### Reencoding Audio to a Different Bit Depth
```cpp
wav::WavData<uint8_t> converted = wav::reencode<int16_t, uint8_t>(wavData);
```

### Saving to a New WAV File
```cpp
converted.save("output.wav");
```

## File Structure
- `wavlib.h`: The main header file containing all functionality.
- `test.cpp`: Example usage and testing code (not included by default).

## License
This project is licensed under the MIT License. See `LICENSE` for more details.

## Contact
For issues, suggestions, or contributions, please visit the project's repository or reach out via email at `support@wavlib.dev`.

