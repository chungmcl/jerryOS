from lldb import *
from lldb.plugins.parsed_cmd import *

def n_bits(n):
    return (1 << (n + 1)) - 1

def get_bits(from_val, msb, lsb):
    return (from_val >> lsb) & n_bits(msb - lsb)

def lldb_get_bits(debugger: lldb.SBDebugger, command: str, result: lldb.SBExecutionContext, internal_dict: dict):
    args = [int(x.strip(), 0) for x in command.strip().split()]
    if len(args) != 3:
        print("Usage: get_bits <from> <msb> <lsb>")
        return
    val = get_bits(args[0], args[1], args[2])
    val_binary = f"{val:064b}"
    val_binary = "0b " + " ".join([val_binary[i:i+4] for i in range(0, len(val_binary), 4)])
    print(val_binary)
    print(f"{hex(val)}")
    print(val)

def __lldb_init_module (debugger: SBDebugger, dict: dict):
    debugger.HandleCommand('command script add -f bit_tools.lldb_n_bits n_bits')
    debugger.HandleCommand('command script add -f bit_tools.lldb_get_bits get_bits')
    print("LLDB bit utils loaded: n_bits, get_bits, set_bits")