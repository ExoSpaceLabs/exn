#include <CCSDSPacket.h>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: validate_ccsdspack_interfaces <cfg> [cfg...]\n";
    return 2;
  }

  int failures = 0;
  for (int i = 1; i < argc; ++i) {
    const std::string path = argv[i];
    ccsds::Packet packet;

    const auto loaded = packet.loadFromConfigFile(path);
    if (!loaded) {
      ++failures;
      std::cerr << "FAIL " << path << ": " << loaded.error().message() << "\n";
      continue;
    }

    const auto serialized = packet.serialize();
    if (!serialized) {
      ++failures;
      std::cerr << "FAIL " << path << ": " << serialized.error().message() << "\n";
      continue;
    }

    if (packet.getPrimaryHeader().getVersionNumber() != 0U) {
      ++failures;
      std::cerr << "FAIL " << path << ": non-zero CCSDS version\n";
      continue;
    }
    if (!packet.getSecondaryHeaderFlag() || !packet.getSecondaryHeader()) {
      ++failures;
      std::cerr << "FAIL " << path << ": missing secondary header\n";
      continue;
    }

    std::cout << "OK   " << path
              << " selector=" << packet.getSecondaryHeader()->getType()
              << " bytes=" << serialized.value().size() << "\n";
  }

  if (failures != 0) {
    std::cerr << failures << " interface configuration(s) failed\n";
    return 1;
  }
  return 0;
}
