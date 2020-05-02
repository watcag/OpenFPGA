/********************************************************************
 * Add commands to the OpenFPGA shell interface, 
 * in purpose of generate SDC files
 * - write_pnr_sdc : generate SDC to constrain the back-end flow for FPGA fabric
 * - write_analysis_sdc: TODO: generate SDC based on users' implementations
 *******************************************************************/
#include "openfpga_sdc.h"
#include "openfpga_sdc_command.h"

/* begin namespace openfpga */
namespace openfpga {

/********************************************************************
 * - Add a command to Shell environment: generate PnR SDC 
 * - Add associated options 
 * - Add command dependency
 *******************************************************************/
static 
ShellCommandId add_openfpga_write_pnr_sdc_command(openfpga::Shell<OpenfpgaContext>& shell,
                                                  const ShellCommandClassId& cmd_class_id,
                                                  const std::vector<ShellCommandId>& dependent_cmds) {
  Command shell_cmd("write_pnr_sdc");

  /* Add an option '--file' in short '-f'*/
  CommandOptionId output_opt = shell_cmd.add_option("file", true, "Specify the output directory for SDC files");
  shell_cmd.set_option_short_name(output_opt, "f");
  shell_cmd.set_option_require_value(output_opt, openfpga::OPT_STRING);

  /* Add an option '--flatten_name' */
  shell_cmd.add_option("flatten_names", false, "Use flatten names (no wildcards) in SDC files");

  /* Add an option '--constrain_global_port' */
  shell_cmd.add_option("constrain_global_port", false, "Constrain all the global ports of FPGA fabric");

  /* Add an option '--constrain_non_clock_global_port' */
  shell_cmd.add_option("constrain_non_clock_global_port", false, "Constrain all the non-clock global ports as clock ports of FPGA fabric");

  /* Add an option '--constrain_grid' */
  shell_cmd.add_option("constrain_grid", false, "Constrain all the grids of FPGA fabric");

  /* Add an option '--constrain_sb' */
  shell_cmd.add_option("constrain_sb", false, "Constrain all the switch blocks of FPGA fabric");

  /* Add an option '--constrain_cb' */
  shell_cmd.add_option("constrain_cb", false, "Constrain all the connection blocks of FPGA fabric");

  /* Add an option '--constrain_configurable_memory_outputs' */
  shell_cmd.add_option("constrain_configurable_memory_outputs", false, "Constrain all the outputs of configurable memories of FPGA fabric");

  /* Add an option '--constrain_routing_multiplexer_outputs' */
  shell_cmd.add_option("constrain_routing_multiplexer_outputs", false, "Constrain all the outputs of routing multiplexer of FPGA fabric");

  /* Add an option '--constrain_switch_block_outputs' */
  shell_cmd.add_option("constrain_switch_block_outputs", false, "Constrain all the outputs of switch blocks of FPGA fabric");

  /* Add an option '--constrain_zero_delay_paths' */
  shell_cmd.add_option("constrain_zero_delay_paths", false, "Constrain zero-delay paths in FPGA fabric");

  /* Add an option '--verbose' */
  shell_cmd.add_option("verbose", false, "Enable verbose output");
  
  /* Add command 'write_fabric_verilog' to the Shell */
  ShellCommandId shell_cmd_id = shell.add_command(shell_cmd, "generate SDC files to constrain the backend flow for FPGA fabric");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, write_pnr_sdc);

  /* Add command dependency to the Shell */
  shell.set_command_dependency(shell_cmd_id, dependent_cmds);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: generate PnR SDC 
 * - Add associated options 
 * - Add command dependency
 *******************************************************************/
static 
ShellCommandId add_openfpga_write_analysis_sdc_command(openfpga::Shell<OpenfpgaContext>& shell,
                                                       const ShellCommandClassId& cmd_class_id,
                                                       const std::vector<ShellCommandId>& dependent_cmds) {
  Command shell_cmd("write_analysis_sdc");

  /* Add an option '--file' in short '-f'*/
  CommandOptionId output_opt = shell_cmd.add_option("file", true, "Specify the output directory for SDC files");
  shell_cmd.set_option_short_name(output_opt, "f");
  shell_cmd.set_option_require_value(output_opt, openfpga::OPT_STRING);

  /* Add an option '--verbose' */
  shell_cmd.add_option("verbose", false, "Enable verbose output");

  /* Add an option '--flatten_name' */
  shell_cmd.add_option("flatten_names", false, "Use flatten names (no wildcards) in SDC files");
  
  /* Add command 'write_fabric_verilog' to the Shell */
  ShellCommandId shell_cmd_id = shell.add_command(shell_cmd, "generate SDC files for timing analysis a PnRed FPGA fabric mapped by a benchmark");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, write_analysis_sdc);

  /* Add command dependency to the Shell */
  shell.set_command_dependency(shell_cmd_id, dependent_cmds);

  return shell_cmd_id;
}

void add_openfpga_sdc_commands(openfpga::Shell<OpenfpgaContext>& shell) {
  /* Get the unique id of 'build_fabric' command which is to be used in creating the dependency graph */
  const ShellCommandId& build_fabric_id = shell.command(std::string("build_fabric"));

  /* Add a new class of commands */
  ShellCommandClassId openfpga_sdc_cmd_class = shell.add_command_class("FPGA-SDC");

  /******************************** 
   * Command 'write_pnr_sdc' 
   */
  /* The 'write_pnr_sdc' command should NOT be executed before 'build_fabric' */
  std::vector<ShellCommandId> pnr_sdc_cmd_dependency;
  pnr_sdc_cmd_dependency.push_back(build_fabric_id);
  add_openfpga_write_pnr_sdc_command(shell,
                                     openfpga_sdc_cmd_class,
                                     pnr_sdc_cmd_dependency);

  /******************************** 
   * Command 'write_analysis_sdc' 
   */
  /* The 'write_analysis_sdc' command should NOT be executed before 'build_fabric' */
  std::vector<ShellCommandId> analysis_sdc_cmd_dependency;
  analysis_sdc_cmd_dependency.push_back(build_fabric_id);
  add_openfpga_write_analysis_sdc_command(shell,
                                          openfpga_sdc_cmd_class,
                                          analysis_sdc_cmd_dependency);

} 

} /* end namespace openfpga */
