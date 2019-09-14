/***********************************************
 * This file includes functions to generate
 * Verilog submodules for multiplexers.
 * including both fundamental submodules
 * such as a branch in a multiplexer 
 * and the full multiplexer
 **********************************************/
#include <string>

#include "util.h"
#include "vtr_assert.h"

/* Device-level header files */
#include "mux_graph.h"
#include "module_manager.h"
#include "physical_types.h"
#include "vpr_types.h"

/* FPGA-X2P context header files */
#include "spice_types.h"
#include "fpga_x2p_naming.h"
#include "fpga_x2p_utils.h"

/* FPGA-Verilog context header files */
#include "verilog_global.h"
#include "verilog_writer_utils.h"
#include "verilog_mux.h"

/*********************************************************************
 * Generate structural Verilog codes (consist of transmission-gates or
 * pass-transistor) modeling an branch circuit 
 * for a multiplexer with the given size 
 *********************************************************************/
static 
void generate_verilog_cmos_mux_branch_body_structural(ModuleManager& module_manager,
                                                      const CircuitLibrary& circuit_lib, 
                                                      std::fstream& fp,
                                                      const CircuitModelId& tgate_model, 
                                                      const ModuleId& module_id, 
                                                      const BasicPort& input_port,
                                                      const BasicPort& output_port,
                                                      const BasicPort& mem_port,
                                                      const BasicPort& mem_inv_port,
                                                      const MuxGraph& mux_graph) {
  /* Make sure we have a valid file handler*/
  check_file_handler(fp);

  /* Get the module id of tgate in Module manager */
  ModuleId tgate_module_id = module_manager.find_module(circuit_lib.model_name(tgate_model));
  VTR_ASSERT(ModuleId::INVALID() != tgate_module_id);

  /* TODO: move to check_circuit_library? Get model ports of tgate */
  std::vector<CircuitPortId> tgate_input_ports = circuit_lib.model_ports_by_type(tgate_model, SPICE_MODEL_PORT_INPUT, true);
  std::vector<CircuitPortId> tgate_output_ports = circuit_lib.model_ports_by_type(tgate_model, SPICE_MODEL_PORT_OUTPUT, true);
  VTR_ASSERT(3 == tgate_input_ports.size());
  VTR_ASSERT(1 == tgate_output_ports.size());

  /* Verilog Behavior description for a MUX */
  print_verilog_comment(fp, std::string("---- Structure-level description -----"));

  /* Output the netlist following the connections in mux_graph */
  /* Iterate over the inputs */
  for (const auto& mux_input : mux_graph.inputs()) {
    BasicPort cur_input_port(input_port.get_name(), size_t(mux_graph.input_id(mux_input)), size_t(mux_graph.input_id(mux_input)));
    /* Iterate over the outputs */
    for (const auto& mux_output : mux_graph.outputs()) {
      BasicPort cur_output_port(output_port.get_name(), size_t(mux_graph.output_id(mux_output)), size_t(mux_graph.output_id(mux_output)));
      /* if there is a connection between the input and output, a tgate will be outputted */
      std::vector<MuxEdgeId> edges = mux_graph.find_edges(mux_input, mux_output);
      /* There should be only one edge or no edge*/
      VTR_ASSERT((1 == edges.size()) || (0 == edges.size()));
      /* No need to output tgates if there are no edges between two nodes */
      if (0 == edges.size()) {
        continue;
      }
      /* TODO: Output a tgate use a module manager */
      /* Create a port-to-port name map */
      std::map<std::string, BasicPort> port2port_name_map;
      /* input port */
      port2port_name_map[circuit_lib.port_lib_name(tgate_input_ports[0])] = cur_input_port;
      /* output port */
      port2port_name_map[circuit_lib.port_lib_name(tgate_output_ports[0])] = cur_output_port;
      /* Find the mem_id controlling the edge */
      MuxMemId mux_mem = mux_graph.find_edge_mem(edges[0]);
      BasicPort cur_mem_port(mem_port.get_name(), size_t(mux_mem), size_t(mux_mem));
      BasicPort cur_mem_inv_port(mem_inv_port.get_name(), size_t(mux_mem), size_t(mux_mem));
      /* mem port */
      if (false == mux_graph.is_edge_use_inv_mem(edges[0])) {
        /* wire mem to mem of module, and wire mem_inv to mem_inv of module */
        port2port_name_map[circuit_lib.port_lib_name(tgate_input_ports[1])] = cur_mem_port;
        port2port_name_map[circuit_lib.port_lib_name(tgate_input_ports[2])] = cur_mem_inv_port;
      } else {
        /* wire mem_inv to mem of module, wire mem to mem_inv of module */
        port2port_name_map[circuit_lib.port_lib_name(tgate_input_ports[1])] = cur_mem_inv_port;
        port2port_name_map[circuit_lib.port_lib_name(tgate_input_ports[2])] = cur_mem_port;
      }  
      /* Output an instance of the module */
      print_verilog_module_instance(fp, module_manager, module_id, tgate_module_id, port2port_name_map, circuit_lib.dump_explicit_port_map(tgate_model));
      /* IMPORTANT: this update MUST be called after the instance outputting!!!!
       * update the module manager with the relationship between the parent and child modules 
       */
      module_manager.add_child_module(module_id, tgate_module_id);
    }
  }
}

/*********************************************************************
 * Generate behavior-level Verilog codes modeling an branch circuit 
 * for a multiplexer with the given size 
 *********************************************************************/
static 
void generate_verilog_cmos_mux_branch_body_behavioral(std::fstream& fp,
                                                      const BasicPort& input_port,
                                                      const BasicPort& output_port,
                                                      const BasicPort& mem_port,
                                                      const MuxGraph& mux_graph,
                                                      const size_t& default_mem_val) {
  /* Make sure we have a valid file handler*/
  check_file_handler(fp);

  /* Verilog Behavior description for a MUX */
  print_verilog_comment(fp, std::string("---- Behavioral-level description -----"));

  /* Add an internal register for the output */
  BasicPort outreg_port("out_reg", mux_graph.num_outputs());
  /* Print the port */
  fp << "\t" << generate_verilog_port(VERILOG_PORT_REG, outreg_port) << ";" << std::endl; 

  /* Generate the case-switch table */
  fp << "\talways @(" << generate_verilog_port(VERILOG_PORT_CONKT, input_port) << ", " << generate_verilog_port(VERILOG_PORT_CONKT, mem_port) << ")" << std::endl; 
  fp << "\tcase (" << generate_verilog_port(VERILOG_PORT_CONKT, mem_port) << ")" << std::endl;

  /* Output the netlist following the connections in mux_graph */
  /* Iterate over the inputs */
  for (const auto& mux_input : mux_graph.inputs()) {
    BasicPort cur_input_port(input_port.get_name(), size_t(mux_graph.input_id(mux_input)), size_t(mux_graph.input_id(mux_input)));
    /* Iterate over the outputs */
    for (const auto& mux_output : mux_graph.outputs()) {
      BasicPort cur_output_port(output_port.get_name(), size_t(mux_graph.output_id(mux_output)), size_t(mux_graph.output_id(mux_output)));
      /* if there is a connection between the input and output, a tgate will be outputted */
      std::vector<MuxEdgeId> edges = mux_graph.find_edges(mux_input, mux_output);
      /* There should be only one edge or no edge*/
      VTR_ASSERT((1 == edges.size()) || (0 == edges.size()));
      /* No need to output tgates if there are no edges between two nodes */
      if (0 == edges.size()) {
        continue;
      }
      /* For each case, generate the logic levels for all the inputs */
      /* In each case, only one mem is enabled */
      fp << "\t\t" << mem_port.get_width() << "'b";
      std::string case_code(mem_port.get_width(), default_mem_val);

      /* Find the mem_id controlling the edge */
      MuxMemId mux_mem = mux_graph.find_edge_mem(edges[0]);
      /* Flip a bit by the mem_id */
      if (false == mux_graph.is_edge_use_inv_mem(edges[0])) {
        case_code[size_t(mux_mem)] = '1';
      } else {
        case_code[size_t(mux_mem)] = '0';
      }
      fp << case_code << ": " << generate_verilog_port(VERILOG_PORT_CONKT, outreg_port) << " <= ";
      fp << generate_verilog_port(VERILOG_PORT_CONKT, cur_input_port) << ";" << std::endl;
    }
  }

  /* Default case: outputs are at high-impedance state 'z' */
  std::string default_case(mux_graph.num_outputs(), 'z');
  fp << "\t\tdefault: " << generate_verilog_port(VERILOG_PORT_CONKT, outreg_port) << " <= ";
  fp << mux_graph.num_outputs() << "'b" << default_case << ";" << std::endl;

  /* End the case */
  fp << "\tendcase" << std::endl;

  /* Wire registers to output ports */
  fp << "\tassign " << generate_verilog_port(VERILOG_PORT_CONKT, output_port) << " = ";
  fp << generate_verilog_port(VERILOG_PORT_CONKT, outreg_port) << ";" << std::endl;
}

/*********************************************************************
 * Generate  Verilog codes modeling an branch circuit 
 * for a multiplexer with the given size 
 * Support structural and behavioral Verilog codes
 *********************************************************************/
static 
void generate_verilog_cmos_mux_branch_module(ModuleManager& module_manager,
                                             const CircuitLibrary& circuit_lib, 
                                             std::fstream& fp,
                                             const CircuitModelId& circuit_model, 
                                             const std::string& module_name, 
                                             const MuxGraph& mux_graph,
                                             const bool& use_structural_verilog) {
  /* Get the tgate model */
  CircuitModelId tgate_model = circuit_lib.pass_gate_logic_model(circuit_model);

  /* Skip output if the tgate model is a MUX2, it is handled by essential-gate generator */
  if (SPICE_MODEL_GATE == circuit_lib.model_type(tgate_model)) {
    VTR_ASSERT(SPICE_MODEL_GATE_MUX2 == circuit_lib.gate_type(tgate_model));
    return;
  }

  std::vector<CircuitPortId> tgate_global_ports = circuit_lib.model_global_ports_by_type(tgate_model, SPICE_MODEL_PORT_INPUT, true);

  /* Make sure we have a valid file handler*/
  check_file_handler(fp);

  /* Generate the Verilog netlist according to the mux_graph */
  /* Find out the number of inputs */ 
  size_t num_inputs = mux_graph.num_inputs();
  /* Find out the number of outputs */ 
  size_t num_outputs = mux_graph.num_outputs();
  /* Find out the number of memory bits */ 
  size_t num_mems = mux_graph.num_memory_bits();

  /* Check codes to ensure the port of Verilog netlists will match */
  /* MUX graph must have only 1 output */
  VTR_ASSERT(1 == num_outputs);
  /* MUX graph must have only 1 level*/
  VTR_ASSERT(1 == mux_graph.num_levels());

  /* Create a Verilog Module based on the circuit model, and add to module manager */
  ModuleId module_id = module_manager.add_module(module_name); 
  VTR_ASSERT(ModuleId::INVALID() != module_id);
  /* Add module ports */
  /* Add each global port */
  for (const auto& port : tgate_global_ports) {
    /* Configure each global port */
    BasicPort global_port(circuit_lib.port_lib_name(port), circuit_lib.port_size(port));
    module_manager.add_port(module_id, global_port, ModuleManager::MODULE_GLOBAL_PORT);
  }
  /* Add each input port */
  BasicPort input_port("in", num_inputs);
  module_manager.add_port(module_id, input_port, ModuleManager::MODULE_INPUT_PORT);
  /* Add each output port */
  BasicPort output_port("out", num_outputs);
  module_manager.add_port(module_id, output_port, ModuleManager::MODULE_OUTPUT_PORT);
  /* Add each memory port */
  BasicPort mem_port("mem", num_mems);
  module_manager.add_port(module_id, mem_port, ModuleManager::MODULE_INPUT_PORT);
  BasicPort mem_inv_port("mem_inv", num_mems);
  module_manager.add_port(module_id, mem_inv_port, ModuleManager::MODULE_INPUT_PORT);

  /* dump module definition + ports */
  print_verilog_module_declaration(fp, module_manager, module_id);

  /* Print the internal logic in either structural or behavioral Verilog codes */
  if (true == use_structural_verilog) {
    generate_verilog_cmos_mux_branch_body_structural(module_manager, circuit_lib, fp, tgate_model, module_id, input_port, output_port, mem_port, mem_inv_port, mux_graph);
  } else {
    VTR_ASSERT_SAFE(false == use_structural_verilog);
    /* Get the default value of SRAM ports */
    std::vector<CircuitPortId> sram_ports = circuit_lib.model_ports_by_type(circuit_model, SPICE_MODEL_PORT_SRAM, true);
    std::vector<CircuitPortId> non_mode_select_sram_ports;
    /* We should have only have 1 sram port except those are mode_bits */
    for (const auto& port : sram_ports) { 
      if (true == circuit_lib.port_is_mode_select(port)) {
        continue;
      }
      non_mode_select_sram_ports.push_back(port);
    }
    VTR_ASSERT(1 == non_mode_select_sram_ports.size());
    std::string mem_default_val = std::to_string(circuit_lib.port_default_value(non_mode_select_sram_ports[0]));
    /* Mem string must be only 1-bit! */
    VTR_ASSERT(1 == mem_default_val.length());
    generate_verilog_cmos_mux_branch_body_behavioral(fp, input_port, output_port, mem_port, mux_graph, mem_default_val[0]);
  }

  /* Put an end to the Verilog module */
  print_verilog_module_end(fp, module_name);
}

/***********************************************
 * Generate Verilog codes modeling an branch circuit 
 * for a multiplexer with the given size 
 **********************************************/
static 
void generate_verilog_mux_branch_module(ModuleManager& module_manager,
                                        const CircuitLibrary& circuit_lib, 
                                        std::fstream& fp, 
                                        const CircuitModelId& circuit_model, 
                                        const size_t& mux_size, 
                                        const MuxGraph& mux_graph) {
  std::string module_name = generate_verilog_mux_branch_subckt_name(circuit_lib, circuit_model, mux_size, mux_graph.num_inputs(), verilog_mux_basis_posfix);

  /* Multiplexers built with different technology is in different organization */
  switch (circuit_lib.design_tech_type(circuit_model)) {
  case SPICE_MODEL_DESIGN_CMOS:
    generate_verilog_cmos_mux_branch_module(module_manager, circuit_lib, fp, circuit_model, module_name, mux_graph, 
                                            circuit_lib.dump_structural_verilog(circuit_model));
    break;
  case SPICE_MODEL_DESIGN_RRAM:
    /* If requested, we can dump structural verilog for basis module */
    /*
    if (true == circuit_lib.dump_structural_verilog(circuit_model)) {
      dump_verilog_rram_mux_one_basis_module_structural(fp, mux_basis_subckt_name,
                                                        num_input_basis_subckt,
                                                        cur_spice_model);
    } else {
      dump_verilog_rram_mux_one_basis_module(fp, mux_basis_subckt_name,
                                             num_input_basis_subckt,
                                             cur_spice_model);
    }
    */
    break;
  default:
    vpr_printf(TIO_MESSAGE_ERROR,
               "(FILE:%s,LINE[%d]) Invalid design technology of multiplexer (name: %s)\n",
               __FILE__, __LINE__, circuit_lib.model_name(circuit_model).c_str()); 
    exit(1);
  }

  return;
}

/***********************************************
 * Generate Verilog modules for all the unique
 * multiplexers in the FPGA device
 **********************************************/
void print_verilog_submodule_muxes(ModuleManager& module_manager,
                                   const MuxLibrary& mux_lib,
                                   const CircuitLibrary& circuit_lib,
                                   t_sram_orgz_info* cur_sram_orgz_info,
                                   char* verilog_dir,
                                   char* submodule_dir) {

  /* TODO: Generate modules into a .bak file now. Rename after it is verified */
  std::string verilog_fname(my_strcat(submodule_dir, muxes_verilog_file_name));
  verilog_fname += ".bak";

  /* Create the file stream */
  std::fstream fp;
  fp.open(verilog_fname, std::fstream::out | std::fstream::trunc);

  check_file_handler(fp);

  /* Print out debugging information for if the file is not opened/created properly */
  vpr_printf(TIO_MESSAGE_INFO,
             "Creating Verilog netlist for Multiplexers (%s) ...\n",
             verilog_fname.c_str()); 

  print_verilog_file_header(fp, "Multiplexers"); 

  print_verilog_include_defines_preproc_file(fp, verilog_dir);
  
  /* Generate basis sub-circuit for unique branches shared by the multiplexers */
  for (auto mux : mux_lib.muxes()) {
    const MuxGraph& mux_graph = mux_lib.mux_graph(mux);
    CircuitModelId mux_circuit_model = mux_lib.mux_circuit_model(mux); 
    /* Create a mux graph for the branch circuit */
    std::vector<MuxGraph> branch_mux_graphs = mux_graph.build_mux_branch_graphs();
    /* Create branch circuits, which are N:1 one-level or 2:1 tree-like MUXes */
    for (auto branch_mux_graph : branch_mux_graphs) {
      generate_verilog_mux_branch_module(module_manager, circuit_lib, fp, mux_circuit_model, 
                                         mux_graph.num_inputs(), branch_mux_graph);
    }
  }

  /* Dump MUX graph one by one */

  /* Close the file steam */
  fp.close();

  /* TODO: 
   * Scan-chain configuration circuit does not need any BLs/WLs! 
   * SRAM MUX does not need any reserved BL/WLs!
   */
  /* Determine reserved Bit/Word Lines if a memory bank is specified,
   * At least 1 BL/WL should be reserved! 
   */
  try_update_sram_orgz_info_reserved_blwl(cur_sram_orgz_info, 
                                          mux_lib.max_mux_size(), mux_lib.max_mux_size());

  /* TODO: Add fname to the linked list when debugging is finished */
  /*
  submodule_verilog_subckt_file_path_head = add_one_subckt_file_name_to_llist(submodule_verilog_subckt_file_path_head, verilog_name);  
   */
}

