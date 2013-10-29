/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_mgr_main.c
 * \author sandeepprakash
 *
 * \date   24-Sep-2012
 *
 * \brief
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include <ch-pal/exp_pal.h>
#include <ch-sockmon/exp_sockmon.h>

#include "rfsvp.h"
#include "rfsvp_node_comm.h"
#include "rfsvp_mgr.h"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/
#define APP_LEADER_HOST_NAME_STR_MAX_LEN           (512)

#define APP_MAX_MONITORED_SOCKETS                  (RFSVP_MAX_SERVER_NODES + 1)

#define APP_CMD_FILENAME_STR_LEN                   (512)

#define APP_ARG_NO_OF_NODES_INDEX                  (1)
#define APP_ARG_LISTEN_PORT_START_INDEX            (2)
#define APP_ARG_COMMAND_FILENAME                   (3)
#define APP_ARG_ENABLE_CONSOLE_LOGGING_INDEX       (4)
#define APP_ARG_APP_INTERNAL_PORT_INDEX            (5)

#define APP_TOTAL_MANDATORY_ARGUMENTS              (5)

/******************************** ENUMERATIONS ********************************/
typedef enum _APP_RET_E
{
   eAPP_RET_FAILURE              = -1,

   eAPP_RET_SUCCESS              = 0x00000000
} APP_RET_E;

/************************* STRUCTURE/UNION DATA TYPES *************************/
typedef struct _APP_ARGS_X
{
   uint32_t    ui_no_nodes;

   uint32_t    ui_enable_console_logging;

   uint16_t    us_listen_port_start_ho;

   uint16_t    us_app_internal_port_ho;

   uint8_t     uca_cmd_filename_str[APP_CMD_FILENAME_STR_LEN];
} APP_ARGS_X;

typedef struct _APP_CTXT_X
{
   APP_ARGS_X  x_app_args;

   SOCKMON_HDL hl_sockmon_hdl;

   RFSVP_MGR_HDL hl_rfsvp_mgr_hdl;
} APP_CTXT_X;

/************************ STATIC FUNCTION PROTOTYPES **************************/

/****************************** LOCAL FUNCTIONS *******************************/
APP_RET_E app_env_init (
   APP_CTXT_X *px_app_ctxt)
{
   APP_RET_E e_main_ret = eAPP_RET_FAILURE;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   RFSVP_RET_E e_rfsvp_ret = eRFSVP_RET_FAILURE;
   SOCKMON_CREATE_PARAMS_X x_sockmon_params = {0};
   RFSVP_MGR_INIT_PARAMS_X x_rfsvp_params = {0};

   if (NULL == px_app_ctxt)
   {
      goto CLEAN_RETURN;
   }

   x_sockmon_params.us_port_range_start =
      px_app_ctxt->x_app_args.us_app_internal_port_ho;
   x_sockmon_params.us_port_range_end =
      px_app_ctxt->x_app_args.us_app_internal_port_ho;
   x_sockmon_params.ui_max_monitored_socks = APP_MAX_MONITORED_SOCKETS;
   e_sockmon_ret = sockmon_create (&(px_app_ctxt->hl_sockmon_hdl),
      &x_sockmon_params);
   if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
   {
      goto CLEAN_RETURN;
   }

   x_rfsvp_params.hl_sockmon_hdl = px_app_ctxt->hl_sockmon_hdl;
   x_rfsvp_params.us_listen_port_start_ho =
         px_app_ctxt->x_app_args.us_listen_port_start_ho;
   x_rfsvp_params.ui_no_nodes = px_app_ctxt->x_app_args.ui_no_nodes;
   (void) pal_strncpy(x_rfsvp_params.uca_cmd_filename_str,
      px_app_ctxt->x_app_args.uca_cmd_filename_str,
      sizeof(x_rfsvp_params.uca_cmd_filename_str));
   e_rfsvp_ret = rfsvp_mgr_init (&(px_app_ctxt->hl_rfsvp_mgr_hdl),
      &x_rfsvp_params);
   if (eRFSVP_RET_SUCCESS != e_rfsvp_ret)
   {
      e_sockmon_ret = sockmon_destroy (px_app_ctxt->hl_sockmon_hdl);
      if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
      {
      }
   }
   else
   {
      e_main_ret = eAPP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_main_ret;
}

APP_RET_E app_env_deinit (
   APP_CTXT_X *px_app_ctxt)
{
   APP_RET_E e_main_ret = eAPP_RET_FAILURE;
   RFSVP_RET_E e_rfsvp_ret = eRFSVP_RET_FAILURE;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;

   if (NULL == px_app_ctxt)
   {
      goto CLEAN_RETURN;
   }

   if (NULL != px_app_ctxt->hl_rfsvp_mgr_hdl)
   {
      e_rfsvp_ret = rfsvp_mgr_deinit (px_app_ctxt->hl_rfsvp_mgr_hdl);
      if (eRFSVP_RET_SUCCESS != e_rfsvp_ret)
      {

      }
      px_app_ctxt->hl_rfsvp_mgr_hdl = NULL;
   }

   if (NULL != px_app_ctxt->hl_sockmon_hdl)
   {
      e_sockmon_ret = sockmon_destroy (px_app_ctxt->hl_sockmon_hdl);
      if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
      {

      }
      px_app_ctxt->hl_sockmon_hdl = NULL;
   }
   e_main_ret = eAPP_RET_SUCCESS;
CLEAN_RETURN:
   return e_main_ret;
}

void app_print_usage(
   int i_argc,
   char **ppuc_argv)
{

   printf ("Usage: \n\t%s "
      "<Total Server Nodes> "
      "<Listen Port Range Start> "
      "<Command Filename/Path> "
      "<Console Logging> "
      "[<Port No For Internal Use>]"
      "\n\n", ppuc_argv[0]);

   printf ("Parameter Details: \n"
      "\t<Total Server Nodes, Range: 2 - 10> - Mandatory\n"
      "\t<Listen Port Range Start (> 15000 Preferred)> - Mandatory\n"
      "\t<Command filename/path (%d characters)> - Mandatory\n"
      "\t<Console Logging, 1 - Enable, 0 - Disable> - Mandatory\n"
      "\t[<Port No For Internal Use (Defaults to 19000)>] - Optional. But "
      "required if bind fails for the internal port\n"
      "\n", APP_CMD_FILENAME_STR_LEN);

}

APP_RET_E app_parse_cmd_line(
   APP_CTXT_X *px_app_ctxt,
   int i_argc,
   char **ppuc_argv)
{
   APP_RET_E e_main_ret = eAPP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   int32_t i_temp = 0;
   uint32_t ui_str_len = 0;

   if ((NULL == px_app_ctxt) || (NULL == ppuc_argv) || (0 == i_argc))
   {
      e_main_ret = eAPP_RET_FAILURE;
      goto CLEAN_RETURN;
   }

   if (i_argc < APP_TOTAL_MANDATORY_ARGUMENTS)
   {
      app_print_usage (i_argc, ppuc_argv);
      e_main_ret = eAPP_RET_FAILURE;
      goto CLEAN_RETURN;
   }

   if ((NULL == ppuc_argv [APP_ARG_NO_OF_NODES_INDEX])
      || (NULL == ppuc_argv [APP_ARG_LISTEN_PORT_START_INDEX])
      || (NULL == ppuc_argv [APP_ARG_COMMAND_FILENAME])
      || (NULL == ppuc_argv [APP_ARG_ENABLE_CONSOLE_LOGGING_INDEX]))
   {
      app_print_usage (i_argc, ppuc_argv);
      e_main_ret = eAPP_RET_FAILURE;
      goto CLEAN_RETURN;
   }

   e_pal_ret = pal_atoi((uint8_t *) ppuc_argv[APP_ARG_NO_OF_NODES_INDEX],
      (int32_t *) &(px_app_ctxt->x_app_args.ui_no_nodes));
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      app_print_usage (i_argc, ppuc_argv);
      e_main_ret = eAPP_RET_FAILURE;
      goto CLEAN_RETURN;
   }

   e_pal_ret = pal_atoi (
      (uint8_t *) ppuc_argv [APP_ARG_LISTEN_PORT_START_INDEX], &(i_temp));
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (i_temp > USHRT_MAX))
   {
      app_print_usage (i_argc, ppuc_argv);
      e_main_ret = eAPP_RET_FAILURE;
      goto CLEAN_RETURN;
   }
   px_app_ctxt->x_app_args.us_listen_port_start_ho = (uint16_t) i_temp;


   ui_str_len = pal_strlen (
      (const uint8_t *) ppuc_argv [APP_ARG_COMMAND_FILENAME]);
   if (ui_str_len > sizeof (px_app_ctxt->x_app_args.uca_cmd_filename_str))
   {
      app_print_usage (i_argc, ppuc_argv);
      e_main_ret = eAPP_RET_FAILURE;
      goto CLEAN_RETURN;
   }
   (void) pal_strncpy (px_app_ctxt->x_app_args.uca_cmd_filename_str,
      (const uint8_t *) ppuc_argv [APP_ARG_COMMAND_FILENAME],
      sizeof(px_app_ctxt->x_app_args.uca_cmd_filename_str));

   e_pal_ret = pal_atoi (
      (uint8_t *) ppuc_argv [APP_ARG_ENABLE_CONSOLE_LOGGING_INDEX],
      (int32_t *) &(px_app_ctxt->x_app_args.ui_enable_console_logging));
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      app_print_usage (i_argc, ppuc_argv);
      e_main_ret = eAPP_RET_FAILURE;
      goto CLEAN_RETURN;
   }

   if (((APP_TOTAL_MANDATORY_ARGUMENTS + 1) == i_argc)
      && (NULL != ppuc_argv [APP_ARG_APP_INTERNAL_PORT_INDEX]))
   {
      e_pal_ret = pal_atoi (
         (uint8_t *) ppuc_argv [APP_ARG_APP_INTERNAL_PORT_INDEX], &(i_temp));
      if ((ePAL_RET_SUCCESS != e_pal_ret) || (i_temp > USHRT_MAX))
      {
         app_print_usage (i_argc, ppuc_argv);
         e_main_ret = eAPP_RET_FAILURE;
         goto CLEAN_RETURN;
      }
      px_app_ctxt->x_app_args.us_app_internal_port_ho = (uint16_t) i_temp;
   }

   printf ("User Entered:\n"
      "\tTotal ServerNodes       : %d\n"
      "\tListen Port Range Start : %d\n"
      "\tCommand Filename/Path   : %s\n"
      "\tEnable Console Logging  : %d\n"
      "\tPort For Internal Use   : %d\n",
      px_app_ctxt->x_app_args.ui_no_nodes,
      px_app_ctxt->x_app_args.us_listen_port_start_ho,
      px_app_ctxt->x_app_args.uca_cmd_filename_str,
      px_app_ctxt->x_app_args.ui_enable_console_logging,
      px_app_ctxt->x_app_args.us_app_internal_port_ho);

   printf ("\n\nEnter 1 to Quit Application\n\n");

   e_main_ret = eAPP_RET_SUCCESS;
CLEAN_RETURN:
   return e_main_ret;
}

int main (int argc, char **argv)
{
   APP_RET_E e_main_ret = eAPP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   APP_CTXT_X *px_app_ctxt = NULL;
   bool b_pal_init = false;

#if 0
   int32_t i_command = -1;
   int32_t i_ret = -1;
#endif

   PAL_LOGGER_INIT_PARAMS_X x_logger_params = {false};

   e_pal_ret = pal_env_init();
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      goto CLEAN_RETURN;
   }
   b_pal_init = true;

   px_app_ctxt = pal_malloc(sizeof(APP_CTXT_X), NULL);
   if (NULL == px_app_ctxt)
   {
      goto CLEAN_RETURN;
   }

   e_main_ret = app_parse_cmd_line (px_app_ctxt, argc, argv);
   if (eAPP_RET_SUCCESS != e_main_ret)
   {
      goto CLEAN_RETURN;
   }

   x_logger_params.b_enable_console_logging =
         (0 == px_app_ctxt->x_app_args.ui_enable_console_logging) ?
               false : true;
   x_logger_params.b_enable_file_logging = true;
   (void) pal_strncpy (x_logger_params.uca_filename_prefix,
      (uint8_t *) "rfsvp_mgr_log_node_",
      sizeof(x_logger_params.uca_filename_prefix));
   x_logger_params.ui_file_name_suffix = 0;
   pal_logger_env_init(&x_logger_params);

   e_main_ret = app_env_init (px_app_ctxt);
   if (eAPP_RET_SUCCESS != e_main_ret)
   {
      goto CLEAN_RETURN;
   }

   while (1)
   {
#if 0
      i_ret = scanf ("%d", &i_command); (void) i_ret;
      if (1 == i_command)
      {
         break;
      }
#else
      pal_sleep(1000);
#endif
   }

   e_main_ret = app_env_deinit (px_app_ctxt);
   if (eAPP_RET_SUCCESS != e_main_ret)
   {
   }

   pal_logger_env_deinit ();
CLEAN_RETURN:
   if (NULL != px_app_ctxt)
   {
      pal_free(px_app_ctxt);
      px_app_ctxt = NULL;
   }
   if (true == b_pal_init)
   {
      e_pal_ret = pal_env_deinit();
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         e_main_ret = eAPP_RET_FAILURE;
      }
      b_pal_init = false;
   }
   return (int) e_main_ret;
}
