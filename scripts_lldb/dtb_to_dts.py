from lldb import *
from lldb.plugins.parsed_cmd import *
from os import popen

class DTBlobToSource(ParsedCommand):
  def setup_command_definition(self) -> None:
    parser: LLDBOptionValueParser = self.get_parser()
    parser.add_option(
      "i",
      "in-scope",
      help = "in_scope_only = True",
      value_type = lldb.eArgTypeBoolean,
      dest = "inscope",
      default = True,
    )
    
    parser.add_option(
      "a",
      "arguments",
      help = "arguments = True",
      value_type = lldb.eArgTypeBoolean,
      dest = "arguments",
      default = True
    )
    
    parser.add_option(
      "l",
      "locals",
      help = "locals = True",
      value_type = lldb.eArgTypeBoolean,
      dest = "locals",
      default = True
    )
    
    parser.add_option(
      "s",
      "statics",
      help = "statics = True",
      value_type = lldb.eArgTypeBoolean,
      dest = "statics",
      default = True,
    )
    
    parser.add_option(
      'p',
      'pointer',
      help='Pointer to the device tree blob.',
      value_type=eArgTypeAddressOrExpression,
      dest='pointer',
      default=None
    )
    
    parser.add_option(
      'o',
      'output-file',
      help=(
        'Path to file where parsed device tree blob should be written to. '
        'Defaults to "./parsedDeviceTree.dts".' 
        'Pass \'_\' if you do not wish to output to a file.'
      ),
      value_type=eArgTypeFilename,
      dest='output_file',
      default='./parsedDeviceTree.dts'
    )
    
    parser.add_option(
      't',
      'print-dts',
      help='Print the device tree source in LLDB session.',
      value_type=eArgTypeBoolean,
      dest='print_dts',
      default=False
    )
  
  def __call__(
    self, 
    debugger: SBDebugger, 
    args_array: SBStructuredData, # This is the list of arguments provided.
    exe_ctx: SBExecutionContext, # Gives the SBExecutionContext on which the command should operate.
    result: SBCommandReturnObject # Any results of the command should be written into this SBCommandReturnObject.
  ):
    target: SBTarget = debugger.GetSelectedTarget()
    process: SBProcess = target.GetProcess()
    frame: SBFrame = exe_ctx.GetFrame()
    parser: LLDBOptionValueParser = self.get_parser()
    variables_list: SBValueList = frame.GetVariables(
      parser.arguments, parser.locals, parser.statics, parser.inscope
    )
    
    for i in range(0, variables_list.GetSize()):
      variable: SBValue = variables_list.GetValueAtIndex(i)
      variable_type: SBType = variable.GetType()
      
      # Is there a way to evaluate parser.pointer, so it can handle
      # something like "deviceTreeAddress + 4"?
      if parser.pointer == variable.name:
        if not SBType.IsPointerType(variable_type):
          print(f'{parser.pointer} is not a pointer.')
          return
        
        device_tree_address = int(variable.value, 16)
        error = SBError()
        
        device_tree_magic_num = int.from_bytes(
          bytearray(
            process.ReadMemory(device_tree_address, 4, error)
          ), 
          byteorder='big'
        )
        if device_tree_magic_num != 0xd00dfeed:
          print(f'{parser.pointer} does not appear to be a valid device tree blob (magic number is incorrect).')
          return
        
        device_tree_len = int.from_bytes(
          bytearray(
            process.ReadMemory(device_tree_address + 4, 4, error)
          ), 
          byteorder='big'
        )
        
        device_tree_blob = process.ReadMemory(device_tree_address, device_tree_len, error)
        popen('mkdir -p .dtb_to_dts').read()
        with open('.dtb_to_dts/.dtb', 'wb') as dtb_file:
            dtb_file.write(device_tree_blob)

        dtc_out = popen('dtc -I dtb -O dts -o .dtb_to_dts/.dts .dtb_to_dts/.dtb').read()
        if dtc_out != '':
          print(dtc_out)

        output_file_path = parser.output_file
        if output_file_path != '_':
          out = popen(f'cp .dtb_to_dts/.dts {output_file_path}').read()
          if out != '':
            print(out)
        
        if parser.print_dts:
          with open('.dtb_to_dts/.dts', 'r') as dts_file:
            print(dts_file.read())

        popen('rm -rf .dtb_to_dts/').read()
        
        return
    
    print(f'There is no pointer named "{parser.pointer}" in this frame.')

    
def __lldb_init_module (debugger: SBDebugger, dict: dict):
  debugger.HandleCommand(f'command script add --parsed --class {__name__}.DTBlobToSource dt-blob-to-source')
