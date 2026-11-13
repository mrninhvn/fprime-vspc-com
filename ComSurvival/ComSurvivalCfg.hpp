// ======================================================================
// \title  ComSurvivalCfg.hpp
// \brief  Limits of Vspc.ComSurvival; a deployment can define any of them
//         first (compile definitions)
// ======================================================================

#ifndef Vspc_ComSurvivalCfg_HPP
#define Vspc_ComSurvivalCfg_HPP

#ifndef COM_SURVIVAL_ARG_MAX
#define COM_SURVIVAL_ARG_MAX 20U  // serialised arguments of one step command, bytes
#endif

#ifndef COM_SURVIVAL_MAX_PHASES
#define COM_SURVIVAL_MAX_PHASES 8U  // COMs in one round
#endif

#ifndef COM_SURVIVAL_CMD_TIMEOUT_S
#define COM_SURVIVAL_CMD_TIMEOUT_S 5U  // response wait of one step command
#endif

#ifndef COM_SURVIVAL_RETRIES
#define COM_SURVIVAL_RETRIES 2U  // extra tries of a failed exit / restore step (enter / beacon: none)
#endif

#endif
