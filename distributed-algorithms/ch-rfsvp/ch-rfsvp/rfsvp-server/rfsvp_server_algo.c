/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_algo.c
 * \author sandeepprakash
 *
 * \date   05-Dec-2012
 *
 * \brief  
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include "rfsvp_server_env.h"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/
#define RFSVP_SERVER_DATA_FILENAME_PREFIX             "rfsvp_server_data"

#define RFSVP_SERVER_DATA_FILENAME_STR_LEN            (512)

#define RFSVP_SERVER_DATA_FILENAME_SUFFIX             ".txt"

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/
static RFSVP_RET_E rfsvp_server_node_get_max_vn (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_node_get_nodes_with_max_vn (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t *pui_node_set,
   uint32_t *pui_node_set_cnt);

static bool rfsvp_server_node_is_ds_subset_of_node_set_with_max_vn (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t *pui_node_set,
   uint32_t ui_node_set_cnt);

static RFSVP_RET_E rfsvp_server_node_get_no_of_reachable_nodes (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t *pui_reachable_cnt);

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_server_node_algo_init (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint8_t uca_filename [RFSVP_SERVER_DATA_FILENAME_STR_LEN] = { 0 };

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   (void) snprintf ((char *) uca_filename, sizeof(uca_filename), "%s_%d_%s%s",
      RFSVP_SERVER_DATA_FILENAME_PREFIX,
      px_rfsvp_server_ctxt->x_init_params.ui_node_index,
      px_rfsvp_server_ctxt->x_server_nodes.xa_nodes [px_rfsvp_server_ctxt->x_init_params.ui_node_index].uca_dns_name_str,
      RFSVP_SERVER_DATA_FILENAME_SUFFIX);

   RFSVP_S_LOG_LOW("Opening file: \"%s\"", uca_filename);
   e_pal_ret = pal_fopen (&(px_algo->hl_file_hdl), uca_filename,
      (const uint8_t *) "a+");
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (NULL == px_algo->hl_file_hdl))
   {
      RFSVP_S_LOG_LOW("pal_fopen failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_STATE_IDLE");
   px_algo->e_state = eRFSVP_SERVER_ALGO_STATE_IDLE;
   px_algo->x_vp.ui_vn = 0;
   px_algo->x_vp.ui_ru = px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
   (void) pal_memset (px_algo->x_vp.uia_ds, 0x00, sizeof(px_algo->x_vp.uia_ds));
   (void) pal_memset (px_algo->ba_reachable, 0x00,
      sizeof(px_algo->ba_reachable));
   (void) pal_memset (px_algo->ba_vote_rsp, 0x00, sizeof(px_algo->ba_vote_rsp));
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_algo_deinit (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_STATE_IDLE");
   px_algo->e_state = eRFSVP_SERVER_ALGO_STATE_IDLE;
   px_algo->x_vp.ui_vn = 0;
   px_algo->x_vp.ui_ru = px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
   (void) pal_memset (px_algo->x_vp.uia_ds, 0x00, sizeof(px_algo->x_vp.uia_ds));

   if (NULL != px_algo->px_cur_req_msg_hdr)
   {
      pal_free (px_algo->px_cur_req_msg_hdr);
      px_algo->px_cur_req_msg_hdr = NULL;
   }
   if (NULL != px_algo->hl_file_hdl)
   {
      e_pal_ret = pal_fclose (px_algo->hl_file_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_fclose failed: %d", e_pal_ret);
      }
      px_algo->hl_file_hdl = NULL;
   }

   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

void rfsvp_server_node_print_voting_table (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_i = 0;
   uint32_t ui_j = 0;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   printf ("|-%10s---%9s---%14s---%16s---%11s---%19s-|\n", "----------",
      "---------", "--------------", "----------------", "-----------",
      "-------------------");
   printf ("| %10s | %9s | %14s | %16s | %11s | %19s |\n", "Node Index",
      "Reachable", "Version Number", "Replicas Updated", "File Offset",
      "Distinguished Sites");
   printf ("|-%10s-+-%9s-+-%14s-+-%16s-+-%11s-+-%19s-|\n", "----------",
      "---------", "--------------", "----------------", "-----------",
      "-------------------");
   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      printf ("| %9d", ui_i);
      if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_node_index)
      {
         printf ("%s", "*");
      }
      else
      {
         printf ("%s", " ");
      }
      px_vp = &(px_algo->xa_nodex_vp[ui_i]);
      if (true == px_algo->ba_reachable[ui_i])
      {
         printf (" | %9s", "true");
      }
      else
      {
         printf (" | %9s", "false");
      }
      printf (" | %14d | %16d | %11d | ", px_vp->ui_vn, px_vp->ui_ru,
         px_vp->ui_cur_file_off);

      for (ui_j = 0; ui_j < px_vp->ui_ds_count; ui_j++)
      {
         printf ("%d,", px_vp->uia_ds[ui_j]);
      }
      printf ("\n");
   }
   printf ("|-%10s---%9s---%14s---%16s---%11s---%19s-|\n", "----------",
      "---------", "--------------", "----------------", "-----------",
      "-------------------");
CLEAN_RETURN:
   return;
}

static RFSVP_RET_E rfsvp_server_node_get_max_vn (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;
   uint32_t ui_max_vn = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_algo->ba_reachable[ui_i])
      {
         px_vp = &(px_algo->xa_nodex_vp[ui_i]);

         if (px_vp->ui_vn > ui_max_vn)
         {
            ui_max_vn = px_vp->ui_vn;
         }
      }
   }
   px_algo->ui_cur_max_vn = ui_max_vn;
   RFSVP_S_LOG_LOW("Max seen VN: %d", px_algo->ui_cur_max_vn);
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_get_nodes_with_max_vn (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t *pui_node_set,
   uint32_t *pui_node_set_cnt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;
   uint32_t ui_node_set_cnt = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == pui_node_set)
      || (NULL == pui_node_set_cnt))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_algo->ba_reachable[ui_i])
      {
         px_vp = &(px_algo->xa_nodex_vp[ui_i]);

         if (px_algo->ui_cur_max_vn == px_vp->ui_vn)
         {
            RFSVP_S_LOG_LOW("Node %d has Max VN", ui_i);
            pui_node_set[ui_node_set_cnt] = ui_i;
            ui_node_set_cnt++;
         }
      }
   }
   *pui_node_set_cnt = ui_node_set_cnt;
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

static bool rfsvp_server_node_is_ds_subset_of_node_set_with_max_vn (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t *pui_node_set,
   uint32_t ui_node_set_cnt)
{
   bool b_is_part_of = false;
   uint32_t ui_i = 0;
   uint32_t ui_j = 0;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == pui_node_set)
      || (0 == ui_node_set_cnt))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_vp = &(px_algo->x_vp);

   if ((0 == px_vp->ui_ds_count) || (0 == ui_node_set_cnt))
   {
      b_is_part_of = false;
      goto CLEAN_RETURN;
   }

   for (ui_i = 0; ui_i < px_vp->ui_ds_count; ui_i++)
   {
      for (ui_j = 0; ui_j < ui_node_set_cnt; ui_j++)
      {
         if (px_vp->uia_ds[ui_i] == pui_node_set[ui_j])
         {
            break;
         }
      }

      if (ui_j == ui_node_set_cnt)
      {
         break;
      }
   }

   if (ui_i == px_vp->ui_ds_count)
   {
      b_is_part_of = true;
   }
   else
   {
      b_is_part_of = false;
   }

CLEAN_RETURN:
   return b_is_part_of;
}

/*
 * Is-Distinguished function call
 * The procedure Is-Distinguished for node S[i] is defined as follows -
 * 1. Node S[i] , upon collecting responses from P[i] computes M , N and set I
 *    as follows:
 *    • M = max {VN[j] : S[j] ∈ P }
 *    • I = {S[k] : VN[k] = M }
 *    • N = RU[j] : S[j] ∈ I
 * 2. If |I| > N / 2,
 *       return YES.
 * 3. Otherwise,
 *    If |I| = N / 2 and DS[i] ∈ I, then
 *       return YES.
 * 4. Otherwise,
 *       return NO.
 */
bool rfsvp_server_node_is_distinguished (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   bool b_is_distinguished = false;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t uia_node_set[RFSVP_MAX_SERVER_NODES] = {0};
   uint32_t ui_node_set_cnt = 0;
   uint32_t ui_ru_by_max_vn_node = 0;
   /*!< Replicas updated by a node with the max version number. */
   uint32_t ui_majority = 0;
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   bool b_is_part_of = false;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_get_max_vn (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_get_max_vn failed: %d", e_error);
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_get_nodes_with_max_vn (px_rfsvp_server_ctxt,
      uia_node_set, &ui_node_set_cnt);
   if ((eRFSVP_RET_SUCCESS != e_error) || (0 == ui_node_set_cnt))
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_get_nodes_with_max_vn failed: %d",
         e_error);
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   ui_ru_by_max_vn_node = px_algo->xa_nodex_vp[uia_node_set[0]].ui_ru;

   ui_majority = ui_ru_by_max_vn_node / 2;

   if (ui_node_set_cnt > ui_majority)
   {
      RFSVP_S_LOG_LOW("Node is part of distinguished partition because of "
         "absolute majority: Node set count %d > Majority %d", ui_node_set_cnt,
         ui_majority);
      b_is_distinguished = true;
   }
   else if (ui_node_set_cnt == ui_majority)
   {
      b_is_part_of = rfsvp_server_node_is_ds_subset_of_node_set_with_max_vn (
         px_rfsvp_server_ctxt, uia_node_set, ui_node_set_cnt);
      b_is_distinguished = b_is_part_of;
      if (true == b_is_distinguished)
      {
         RFSVP_S_LOG_LOW("Node is part of distinguished partition because "
            "DS is a subset of node set with MAX VN");
      }
   }
   else
   {
      b_is_distinguished = false;
      RFSVP_S_LOG_LOW("Node is not part of distinguished partition");
   }
CLEAN_RETURN:
   return b_is_distinguished;
}

static RFSVP_RET_E rfsvp_server_node_get_no_of_reachable_nodes (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t *pui_reachable_cnt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_i = 0;
   uint32_t ui_reachable_cnt = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == pui_reachable_cnt))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_i++)
   {
      if (true == px_algo->ba_reachable [ui_i])
      {
         ui_reachable_cnt++;
      }
   }
   *pui_reachable_cnt = ui_reachable_cnt;
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}
/*
 * Procedure Do-Update for node S[i] :
 * 1. VN[i] = M + 1
 * 2. RU[i] = |P[i]|
 *
 *             --
 *             \ P[i] If |P[i]| = 3
 * 3. DS[i] = -<
 *             / S` If |P[i]| is even, |P[i]| > 3 and S` has highest ID in P[i]
 *             --
 */
RFSVP_RET_E rfsvp_server_node_do_update (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;
   uint32_t ui_reachable = 0;
   uint32_t ui_i = 0;
   uint32_t ui_j = 0;
   uint32_t ui_highest_id_idx = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_vp = &(px_algo->x_vp);

   /*
    * By the time do_update is called, synchronization would have happened and
    * the version number of the current node would have updated to the max
    * value that we would have calculated in the is_distinguished procedure.
    */
   px_vp->ui_vn += 1;

   e_error = rfsvp_server_node_get_no_of_reachable_nodes (px_rfsvp_server_ctxt,
      &ui_reachable);
   if ((eRFSVP_RET_SUCCESS != e_error) || (0 == ui_reachable))
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_get_no_of_reachable_nodes failed: %d",
         e_error);
      goto CLEAN_RETURN;
   }

   px_vp->ui_ru = ui_reachable;

   /*
    * TODO:
    *             --
    *             \ P[i] If |P[i]| = 3
    * 3. DS[i] = -<
    *             / S` If |P[i]| is even, |P[i]| > 3 and S` has highest ID in P[i]
    *             --
    */
   ui_j = 0;
   if (3 == ui_reachable)
   {
      for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
            ui_i++)
      {
         if (true == px_algo->ba_reachable [ui_i])
         {
            px_vp->uia_ds[ui_j] = ui_i;
            ui_j++;
            px_vp->ui_ds_count = ui_j;
         }
      }

   }
   else
   {
      if (ui_reachable > 3)
      {
         if (0 == (ui_reachable % 2))
         {
            // If |P[i]| is even, |P[i]| > 3 and S has highest ID in P[i]
            ui_highest_id_idx = px_rfsvp_server_ctxt->x_init_params.ui_no_nodes
               - 1;
            if (true == px_algo->ba_reachable [ui_highest_id_idx])
            {
               px_vp->uia_ds[ui_j] = ui_highest_id_idx;
               ui_j++;
               px_vp->ui_ds_count = ui_j;
            }
         }
      }
      else
      {

      }
   }

CLEAN_RETURN:
   return e_error;
}
