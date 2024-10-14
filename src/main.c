// #include <stdint.h>

int main() {
  // TODO: parse the device tree with https://github.com/dgibson/dtc
  // note that the device tree is in big endian while this CPU is lil endian
  char* device_tree_address;
  asm volatile (
    "mov %0, x1" // Get the device tree's address out of x1
    : "=r"(device_tree_address) // Output operand
    : // No input operands
    : // No clobbered registers
  );

  return 0;
}
