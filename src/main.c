#include "jerryTypes.h"
#include "libfdt.h"

int main() {
  // note that the device tree is in big endian while this CPU is lil endian
  u8* deviceTreeAddress;
  asm volatile (
    "mov %0, x1" // Get the device tree's address out of x1
    : "=r"(deviceTreeAddress) // Output operand
    : // No input operands
    : // No clobbered registers
  );

  if (fdt_check_header(deviceTreeAddress) != 0) {
    // panic
  }

  int offset = 0;  // Root node
  int depth = 0;
  const void* fdt = (const void*)deviceTreeAddress;

  do {
    offset = fdt_next_node(fdt, offset, &depth);  // Assign offset before the condition
    if (offset < 0) {
      break;  // Exit the loop if there are no more nodes
    }
    const char* name = fdt_get_name(fdt, offset, NULL);
    // printf("Node: %s (depth: %d)\n", name, depth);
    int a = 2 + 2;

  } while (offset >= 0);

  return 0;
}
