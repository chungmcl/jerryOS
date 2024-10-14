import lldb
import re
import struct
import subprocess
import os
  
def dt_blob_to_source(debugger: lldb.SBDebugger, command: str, result: lldb.SBCommandReturnObject, dict: dict):
  interpreter = debugger.GetCommandInterpreter()
  return_obj = lldb.SBCommandReturnObject()
  def execute(command: str) -> str:
    interpreter.HandleCommand(command, return_obj)
    if return_obj.Succeeded():
      return return_obj.GetOutput()
    else:
      error = return_obj.GetError()
      print(error)
      return error
  
  inputs = re.findall(r'"(.*?)"|(\S+)', command)
  inputs = [input[0] if input[0] else input[1] for input in inputs]
  
  if len(inputs) < 1:
    print('Please specify the name of the device tree address pointer.')
    return
  elif len(inputs) == 1:
    device_tree_ptr_name = inputs[0]
    output_file = None
  else:
    device_tree_ptr_name = inputs[0]
    output_file = inputs[1]
    
  first_four_bytes = execute(f'x/4 {device_tree_ptr_name}')
  match = re.match(r'.*: .* (.*) .* .*', first_four_bytes)
  device_tree_len = int(match.group(1), 16)
  device_tree_len = int(device_tree_len.to_bytes(4, byteorder='little').hex(), 16)
  
  dt_hex = execute(f'x/{device_tree_len} {device_tree_ptr_name} --force')
  u32s: list[int] = []
  for line in dt_hex.splitlines():
    match = re.match(r'.*: (.*) (.*) (.*) (.*)', line)
    u32s += [int(byte, 16) for byte in match.groups()]
  
  os.popen('mkdir -p .dtb_to_dts').read()
  with open('.dtb_to_dts/.dtb', 'wb') as dtb:
    for u32 in u32s:
      dtb.write(struct.pack('I', u32))
  
  os.popen('dtc -I dtb -O dts -o .dtb_to_dts/.dts .dtb_to_dts/.dtb').read()
  
  with open('.dtb_to_dts/.dts', 'r') as dts:
    dts = dts.read()
    if output_file is None:
      print(dts)
    else:
      with open(output_file, 'w') as f:
        f.write(dts)
  
  os.popen('rm -rf .dtb_to_dts').read()
    
def __lldb_init_module (debugger, dict):
  debugger.HandleCommand('command script add -f dtb_to_dts.dt_blob_to_source dt_blob_to_source')
