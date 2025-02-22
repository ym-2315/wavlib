#include "wavlib.h"
#include <iostream>
#include <string>

int main()
{
  // Read the original WAV file.
  wav::WavFile wf;
  if (!wf.read("change/input/file.wav"))
  {
    std::cerr << "Error reading WAV file." << std::endl;
    return 1;
  }

  std::cout << "Channels: " << wf.num_channels
            << ", Sample Rate: " << wf.sample_rate
            << ", Bits per Sample: " << wf.bits_per_sample
            << ", Samples per Channel: " << wf.num_samples << std::endl;

  // Convert raw data into deinterleaved data.
  // Ensure the template type matches the file's bit depth.
  wav::WavData<int32_t> data(wf);

  // Convert deinterleaved data back into a complete WavFile.
  wav::WavFile generatedWav = data.toWavFile();

  // Save the generated WavFile to disk.
  if (generatedWav.save("change/output/file.wav"))
  {
    std::cout << "WAV file saved successfully." << std::endl;
  }
  else
  {
    std::cerr << "Failed to save WAV file." << std::endl;
  }

  // Example: Resample to 22050 Hz.
  auto resampledData = wav::resample(data, 22050);
  std::cout << "Resampled sample rate: " << resampledData.sample_rate << std::endl;

  // Example: Re-encode from int32_t to int16_t.
  auto reencodedData = wav::reencode<int32_t, int16_t>(data);
  std::cout << "Re-encoded bit depth: " << reencodedData.bits_per_sample << " bits." << std::endl;
  return 0;
}
