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
  const void* currNode = (const void*)deviceTreeAddress;

  do {
    offset = fdt_next_node(currNode, offset, &depth);
    if (offset < 0) {
      break; 
    }

    const char* currNodeName = fdt_get_name(currNode, offset, NULL);

    int propertyOffset;
    const struct fdt_property* property;
    fdt_for_each_property_offset(propertyOffset, currNode, offset) {
      property = fdt_get_property_by_offset(currNode, propertyOffset, NULL);
      if (property) {
        const char* prop_name = fdt_string(currNode, fdt32_to_cpu(property->nameoff));
      }
    }
  } while (offset >= 0);

  return 0;
}
