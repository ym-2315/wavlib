#ifndef WAVLIB_H
#define WAVLIB_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <string>
#include <cmath>
#include <limits>
#include <type_traits>

namespace wav
{

  /*
    Canonical WAVE File Format (PCM):

    RIFF Header (first 12 bytes):
    ----------------------------------------------------------------------------
    Offset  Size  Name         Description
    ----------------------------------------------------------------------------
    0       4     ChunkID      Contains the letters "RIFF" (0x52494646 big-endian).
    4       4     ChunkSize    36 + Subchunk2Size, or more precisely:
                             4 + (8 + Subchunk1Size) + (8 + Subchunk2Size).
                             This is the size of the entire file in bytes minus 8.
    8       4     Format       Contains the letters "WAVE" (0x57415645 big-endian).

    "WAVE" Format:
    The file consists of two main subchunks: "fmt " and "data".

    "fmt " subchunk:
    ----------------------------------------------------------------------------
    Offset  Size  Name             Description
    ----------------------------------------------------------------------------
    12      4     Subchunk1ID      Contains the letters "fmt " (0x666d7420 big-endian).
    16      4     Subchunk1Size    16 for PCM. This is the size of the rest of the subchunk.
    20      2     AudioFormat      PCM = 1 (Linear quantization; others indicate compression).
    22      2     NumChannels      Mono = 1, Stereo = 2, etc.
    24      4     SampleRate       8000, 44100, etc.
    28      4     ByteRate         == SampleRate * NumChannels * BitsPerSample/8.
    32      2     BlockAlign       == NumChannels * BitsPerSample/8.
    34      2     BitsPerSample    8, 16, 24, or 32.
            (if PCM, no extra parameters follow)

    "data" subchunk:
    ----------------------------------------------------------------------------
    Offset  Size  Name             Description
    ----------------------------------------------------------------------------
    36      4     Subchunk2ID      Contains the letters "data" (0x64617461 big-endian).
    40      4     Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8.
    44      *     Data             The actual sound data (interleaved).
  */

  //------------------------------------------------------------------------------
  // WavFile: Represents a complete WAV file (header and interleaved raw audio data).
  //------------------------------------------------------------------------------
  struct WavFile
  {
    uint32_t chunk_size = 0;
    uint16_t num_channels = 0;
    uint32_t sample_rate = 0;
    uint16_t block_align = 0;
    uint16_t bits_per_sample = 0;
    uint32_t data_size = 0;
    uint32_t num_samples = 0; // per channel
    std::vector<char> raw_data;

    // Reads a WAV file from disk.
    bool read(const std::string &filePath)
    {
      std::ifstream file(filePath, std::ios::binary);
      if (!file.is_open())
      {
        std::cerr << "Couldn't open file: " << filePath << std::endl;
        return false;
      }
      // Read RIFF header.
      char chunkID[5] = {0};
      file.read(chunkID, 4);
      if (std::strncmp(chunkID, "RIFF", 4) != 0)
      {
        std::cerr << "ChunkID must be 'RIFF'" << std::endl;
        return false;
      }
      file.read(reinterpret_cast<char *>(&chunk_size), sizeof(chunk_size));
      char format[5] = {0};
      file.read(format, 4);
      if (std::strncmp(format, "WAVE", 4) != 0)
      {
        std::cerr << "Format must be 'WAVE'" << std::endl;
        return false;
      }
      // Read subchunks until both "fmt " and "data" are found.
      bool foundFmt = false, foundData = false;
      while (file && (!foundFmt || !foundData))
      {
        char subchunkID[5] = {0};
        file.read(subchunkID, 4);
        if (file.gcount() < 4)
          break;
        uint32_t subchunk_size = 0;
        file.read(reinterpret_cast<char *>(&subchunk_size), sizeof(subchunk_size));
        if (std::strncmp(subchunkID, "fmt ", 4) == 0)
        {
          foundFmt = true;
          uint16_t audioFormat = 0;
          file.read(reinterpret_cast<char *>(&audioFormat), sizeof(audioFormat));
          file.read(reinterpret_cast<char *>(&num_channels), sizeof(num_channels));
          file.read(reinterpret_cast<char *>(&sample_rate), sizeof(sample_rate));
          file.seekg(4, std::ios::cur); // skip ByteRate.
          file.read(reinterpret_cast<char *>(&block_align), sizeof(block_align));
          file.read(reinterpret_cast<char *>(&bits_per_sample), sizeof(bits_per_sample));
          if (subchunk_size > 16)
            file.seekg(subchunk_size - 16, std::ios::cur);
        }
        else if (std::strncmp(subchunkID, "data", 4) == 0)
        {
          foundData = true;
          data_size = subchunk_size;
          raw_data.resize(data_size);
          file.read(raw_data.data(), data_size);
        }
        else
        {
          file.seekg(subchunk_size, std::ios::cur);
        }
      }
      if (!foundFmt)
      {
        std::cerr << "Couldn't find 'fmt ' subchunk." << std::endl;
        return false;
      }
      if (!foundData)
      {
        std::cerr << "Couldn't find 'data' subchunk." << std::endl;
        return false;
      }
      num_samples = data_size / block_align;
      return true;
    }

    // Saves this WAV file to disk.
    bool save(const std::string &filePath) const
    {
      std::ofstream out(filePath, std::ios::binary);
      if (!out.is_open())
      {
        std::cerr << "Error opening output file: " << filePath << std::endl;
        return false;
      }
      out.write("RIFF", 4);
      out.write(reinterpret_cast<const char *>(&chunk_size), sizeof(chunk_size));
      out.write("WAVE", 4);
      out.write("fmt ", 4);
      uint32_t subchunk1Size = 16;
      out.write(reinterpret_cast<const char *>(&subchunk1Size), sizeof(subchunk1Size));
      uint16_t audioFormat = 1;
      out.write(reinterpret_cast<const char *>(&audioFormat), sizeof(audioFormat));
      out.write(reinterpret_cast<const char *>(&num_channels), sizeof(num_channels));
      out.write(reinterpret_cast<const char *>(&sample_rate), sizeof(sample_rate));
      uint16_t bytesPerSample = bits_per_sample / 8;
      uint16_t localBlockAlign = num_channels * bytesPerSample;
      uint32_t byteRate = sample_rate * localBlockAlign;
      out.write(reinterpret_cast<const char *>(&byteRate), sizeof(byteRate));
      out.write(reinterpret_cast<const char *>(&localBlockAlign), sizeof(localBlockAlign));
      out.write(reinterpret_cast<const char *>(&bits_per_sample), sizeof(bits_per_sample));
      out.write("data", 4);
      out.write(reinterpret_cast<const char *>(&data_size), sizeof(data_size));
      out.write(reinterpret_cast<const char *>(raw_data.data()), data_size);
      out.close();
      return true;
    }
  };

  //------------------------------------------------------------------------------
  // WavData<T>: Stores deinterleaved, typed audio data.
  //------------------------------------------------------------------------------
  template <typename T>
  struct WavData
  {
    uint32_t sample_rate = 0;
    uint16_t num_channels = 0;
    uint16_t bits_per_sample = 0;
    uint32_t num_samples = 0; // per channel
    std::vector<T> channel1;  // Left channel (or mono)
    std::vector<T> channel2;  // Right channel (if stereo)

    WavData() = default;

    // Constructs WavData from a WavFile by extracting each sample using block alignment.
    WavData(const WavFile &wf)
    {
      sample_rate = wf.sample_rate;
      num_channels = wf.num_channels;
      bits_per_sample = wf.bits_per_sample;
      num_samples = wf.num_samples;
      // Check that T matches file bit depth.
      if (bits_per_sample != sizeof(T) * 8)
      {
        std::cerr << "Bit depth mismatch: file has " << bits_per_sample
                  << " bits, but T is " << (sizeof(T) * 8) << " bits." << std::endl;
        return;
      }
      // Use block alignment: each sample block is wf.block_align bytes.
      channel1.resize(num_samples);
      if (num_channels == 2)
        channel2.resize(num_samples);
      uint32_t blockAlign = wf.block_align;
      for (uint32_t i = 0; i < num_samples; i++)
      {
        // Compute the starting offset for sample i.
        const char *samplePtr = wf.raw_data.data() + i * blockAlign;
        // For each channel, extract sizeof(T) bytes.
        // Left channel (channel 0).
        std::memcpy(&channel1[i], samplePtr, sizeof(T));
        // Right channel (if stereo).
        if (num_channels == 2)
        {
          std::memcpy(&channel2[i], samplePtr + sizeof(T), sizeof(T));
        }
      }
    }

    // Converts this WavData into a complete WavFile.
    WavFile toWavFile() const
    {
      WavFile wf;
      wf.sample_rate = sample_rate;
      wf.num_channels = num_channels;
      wf.bits_per_sample = bits_per_sample;
      wf.block_align = num_channels * (bits_per_sample / 8);
      wf.num_samples = num_samples;
      wf.data_size = num_samples * wf.block_align;
      wf.raw_data.resize(wf.data_size);
      // Interleave data: for each sample, write channel1 and (if stereo) channel2.
      for (uint32_t i = 0; i < num_samples; i++)
      {
        // Compute destination pointer.
        char *dest = wf.raw_data.data() + i * wf.block_align;
        // Copy left channel.
        std::memcpy(dest, &channel1[i], sizeof(T));
        // Copy right channel if needed.
        if (num_channels == 2)
        {
          std::memcpy(dest + sizeof(T), &channel2[i], sizeof(T));
        }
      }
      wf.chunk_size = 36 + wf.data_size;
      return wf;
    }

    // Saves this WavData to disk by converting to a WavFile and calling its save.
    bool save(const std::string &filePath) const
    {
      WavFile wf = toWavFile();
      return wf.save(filePath);
    }
  };

  //------------------------------------------------------------------------------
  // Resample: Resamples a WavData<T> to a new sample rate using linear interpolation.
  //------------------------------------------------------------------------------
  template <typename T>
  WavData<T> resample(const WavData<T> &input, uint32_t new_sample_rate)
  {
    WavData<T> output = input;
    output.sample_rate = new_sample_rate;
    double ratio = static_cast<double>(new_sample_rate) / input.sample_rate;
    uint32_t newNumSamples = static_cast<uint32_t>(input.num_samples * ratio);
    output.num_samples = newNumSamples;
    output.channel1.resize(newNumSamples);
    if (input.num_channels == 2)
      output.channel2.resize(newNumSamples);
    for (uint32_t i = 0; i < newNumSamples; i++)
    {
      double src_index = i / ratio;
      uint32_t index0 = static_cast<uint32_t>(std::floor(src_index));
      uint32_t index1 = (index0 + 1 < input.num_samples) ? index0 + 1 : index0;
      double frac = src_index - index0;
      double s0 = static_cast<double>(input.channel1[index0]);
      double s1 = static_cast<double>(input.channel1[index1]);
      double interp = (1.0 - frac) * s0 + frac * s1;
      output.channel1[i] = static_cast<T>(std::round(interp));
      if (input.num_channels == 2)
      {
        double t0 = static_cast<double>(input.channel2[index0]);
        double t1 = static_cast<double>(input.channel2[index1]);
        double interp2 = (1.0 - frac) * t0 + frac * t1;
        output.channel2[i] = static_cast<T>(std::round(interp2));
      }
    }
    return output;
  }

  //------------------------------------------------------------------------------
  // convertSample: Converts a sample from type From to type To (distinguishing signed/unsigned).
  //------------------------------------------------------------------------------
  template <typename From, typename To>
  To convertSample(From sample)
  {
    double fSample = static_cast<double>(sample);
    double fromMin, fromMax, toMin, toMax;
    if constexpr (std::is_signed<From>::value)
    {
      fromMin = static_cast<double>(std::numeric_limits<From>::min());
      fromMax = static_cast<double>(std::numeric_limits<From>::max());
    }
    else
    {
      fromMin = 0.0;
      fromMax = static_cast<double>(std::numeric_limits<From>::max());
    }
    if constexpr (std::is_signed<To>::value)
    {
      toMin = static_cast<double>(std::numeric_limits<To>::min());
      toMax = static_cast<double>(std::numeric_limits<To>::max());
    }
    else
    {
      toMin = 0.0;
      toMax = static_cast<double>(std::numeric_limits<To>::max());
    }
    double normalized = (fSample - fromMin) / (fromMax - fromMin);
    double result = normalized * (toMax - toMin) + toMin;
    return static_cast<To>(std::round(result));
  }

  //------------------------------------------------------------------------------
  // Reencode: Converts a WavData from one sample type to another.
  //------------------------------------------------------------------------------
  template <typename From, typename To>
  WavData<To> reencode(const WavData<From> &input)
  {
    WavData<To> output;
    output.sample_rate = input.sample_rate;
    output.num_channels = input.num_channels;
    output.num_samples = input.num_samples;
    output.bits_per_sample = sizeof(To) * 8;
    output.channel1.resize(input.num_samples);
    if (input.num_channels == 2)
      output.channel2.resize(input.num_samples);
    for (uint32_t i = 0; i < input.num_samples; i++)
    {
      output.channel1[i] = convertSample<From, To>(input.channel1[i]);
      if (input.num_channels == 2)
        output.channel2[i] = convertSample<From, To>(input.channel2[i]);
    }
    return output;
  }

} // namespace wav

#endif // WAVLIB_H
