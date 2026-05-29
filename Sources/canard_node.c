#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "canard_node.h"
#include "dsdl/dronecan_msgs.h"
#include "fdcan.h"
#include "libcanard/canard.h"
#include "main.h"
#include "stm32g4xx_hal.h"

// No DNA allocation if UAVCAN_NODE_ID > 0
#define UAVCAN_NODE_ID 0
// Set to a number greater than 0 for preferred node id.
#define PREFERRED_NODE_ID 69

#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 0
#define APP_NODE_NAME "org.bco.bms"

typedef struct {
  uint32_t period_ms;
  uint32_t last_run_ms;
  void (*task)(void);
} task_t;

static struct {
  uint32_t send_next_node_id_allocation_request_at_ms;
  uint32_t node_id_allocation_unique_id_offset;
} DNA;

typedef enum { NODE_STATE_DNA, NODE_STATE_OPERATIONAL } node_state_t;

static void task_1hz(void);
static void task_5hz(void);
static void task_10hz(void);

static task_t loop_tasks[] = {
    {1000ULL, 0, task_1hz},
    {200ULL, 0, task_5hz},
    {100ULL, 0, task_10hz},
};
static const size_t loop_task_count =
    sizeof(loop_tasks) / sizeof(loop_tasks[0]);

static uint8_t canard_memory_pool[2048];

CanardInstance canard;

static struct uavcan_protocol_NodeStatus node_status;
static node_state_t node_state;

// TODO
static uint8_t DUMMY_HW_ID[16] = {0xDE, 0x23, 0x45, 0x67, 0x89, 0xAB,
                                  0xCD, 0xEF, 0x10, 0x32, 0x54, 0x76,
                                  0x98, 0xBA, 0xDC, 0xFE};

static void set_node_id(uint8_t id) {
  canardSetLocalNodeID(&canard, id);
  node_state = NODE_STATE_OPERATIONAL;
}
static void handle_dna_allocation(CanardInstance *ins,
                                  CanardRxTransfer *transfer) {
  if (canardGetLocalNodeID(&canard) != CANARD_BROADCAST_NODE_ID) {
    printf("already allocated\n");
    // already allocated
    return;
  }

  // Rule C - updating the randomized time interval
  DNA.send_next_node_id_allocation_request_at_ms =
      (get_uptime_ms()) +
      UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS +
      (UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

  if (transfer->source_node_id == CANARD_BROADCAST_NODE_ID) {
    printf("Allocation request from another allocatee\n");
    DNA.node_id_allocation_unique_id_offset = 0;
    return;
  }

  // Copying the unique ID from the message
  struct uavcan_protocol_dynamic_node_id_Allocation msg;

  uavcan_protocol_dynamic_node_id_Allocation_decode(transfer, &msg);

  // Obtaining the local unique ID
  uint8_t my_unique_id[sizeof(msg.unique_id.data)];
  memcpy(my_unique_id, DUMMY_HW_ID, sizeof(my_unique_id));

  // Matching the received UID against the local one
  if (memcmp(msg.unique_id.data, my_unique_id, msg.unique_id.len) != 0) {
    printf("Mismatching allocation response\n");
    DNA.node_id_allocation_unique_id_offset = 0;
    // No match, return
    return;
  }

  if (msg.unique_id.len < sizeof(msg.unique_id.data)) {
    // The allocator has confirmed part of unique ID, switching to
    // the next stage and updating the timeout.
    DNA.node_id_allocation_unique_id_offset = msg.unique_id.len;
    DNA.send_next_node_id_allocation_request_at_ms -=
        UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS;
    printf("Matching allocation response: %d\n", msg.unique_id.len);
  } else {
    // Allocation complete - copying the allocated node ID from the message
    set_node_id(msg.node_id);
    printf("Node ID allocated: %d\n", msg.node_id);
  }
}

static void request_DNA() {
  const uint32_t now = get_uptime_ms();
  static uint8_t node_id_allocation_transfer_id = 0;

  DNA.send_next_node_id_allocation_request_at_ms =
      now + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS +
      (UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

  // Structure of the request is documented in the DSDL definition
  // See
  // http://uavcan.org/Specification/6._Application_level_functions/#dynamic-node-id-allocation
  uint8_t allocation_request[CANARD_CAN_FRAME_MAX_DATA_LEN - 1];
  allocation_request[0] = (uint8_t)(PREFERRED_NODE_ID << 1U);

  if (DNA.node_id_allocation_unique_id_offset == 0) {
    allocation_request[0] |= 1; // First part of unique ID
  }

  static const uint8_t MaxLenOfUniqueIDInRequest = 6;
  uint8_t uid_size = (uint8_t)(16 - DNA.node_id_allocation_unique_id_offset);

  if (uid_size > MaxLenOfUniqueIDInRequest) {
    uid_size = MaxLenOfUniqueIDInRequest;
  }
  uint8_t my_unique_id[16];
  memcpy(my_unique_id, DUMMY_HW_ID, sizeof(my_unique_id));
  memmove(&allocation_request[1],
          &my_unique_id[DNA.node_id_allocation_unique_id_offset], uid_size);

  const int16_t bcast_res = canardBroadcast(
      &canard, UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE,
      UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID,
      &node_id_allocation_transfer_id, CANARD_TRANSFER_PRIORITY_LOW,
      &allocation_request[0], (uint16_t)(uid_size + 1));
  if (bcast_res < 0) {
    printf("Could not broadcast ID allocation req; error %d\n", bcast_res);
  }

  // Preparing for timeout; if response is received, this value will be updated
  // from the callback.
  DNA.node_id_allocation_unique_id_offset = 0;
}

static void handle_getNodeInfo(CanardInstance *ins,
                               CanardRxTransfer *transfer) {
  uint8_t buffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];
  struct uavcan_protocol_GetNodeInfoResponse pkt;
  memset(&buffer, 0, sizeof(buffer));
  memset(&pkt, 0, sizeof(struct uavcan_protocol_GetNodeInfoResponse));
  node_status.uptime_sec = get_uptime_sec();
  pkt.status = node_status;

  pkt.software_version.major = APP_VERSION_MAJOR;
  pkt.software_version.minor = APP_VERSION_MINOR;
  pkt.software_version.optional_field_flags = 0;
  pkt.software_version.vcs_commit = 0;

  pkt.hardware_version.major = 1;
  pkt.hardware_version.minor = 0;

  memcpy(pkt.hardware_version.unique_id, DUMMY_HW_ID, sizeof(DUMMY_HW_ID));
  const uint8_t node_name_len = strlen(APP_NODE_NAME);
  strncpy((char *)pkt.name.data, APP_NODE_NAME, node_name_len);
  pkt.name.len = node_name_len;

  uint16_t total_size =
      uavcan_protocol_GetNodeInfoResponse_encode(&pkt, buffer);

  canardRequestOrRespond(
      ins, transfer->source_node_id, UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE,
      UAVCAN_PROTOCOL_GETNODEINFO_ID, &transfer->transfer_id,
      transfer->priority, CanardResponse, &buffer[0], total_size);
}

static void broadcast_node_status(void) {
  uint8_t buffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];
  memset(buffer, 0, UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE);
  node_status.uptime_sec = get_uptime_sec();
  node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
  node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
  node_status.sub_mode = 0;
  node_status.vendor_specific_status_code = 0;

  uint32_t len = uavcan_protocol_NodeStatus_encode(&node_status, buffer);
  static uint8_t transfer_id;
  canardBroadcast(&canard, UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
                  UAVCAN_PROTOCOL_NODESTATUS_ID, &transfer_id,
                  CANARD_TRANSFER_PRIORITY_LOW, buffer, len);
}

static void onTransferReceived(CanardInstance *ins,
                               CanardRxTransfer *transfer) {

  if ((transfer->transfer_type == CanardTransferTypeRequest)) {
    switch (transfer->data_type_id) {
    case UAVCAN_PROTOCOL_GETNODEINFO_ID: {
      handle_getNodeInfo(ins, transfer);
      break;
    }
    }
  }

  if (transfer->transfer_type == CanardTransferTypeBroadcast) {
    switch (transfer->data_type_id) {
    case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID: {
      handle_dna_allocation(ins, transfer);
      break;
    }
    }
  }
}

static bool shouldAcceptTransfer(const CanardInstance *ins,
                                 uint64_t *out_data_type_signature,
                                 uint16_t data_type_id,
                                 CanardTransferType transfer_type,
                                 uint8_t source_node_id) {

  if (transfer_type == CanardTransferTypeRequest) {
    switch (data_type_id) {
    case UAVCAN_PROTOCOL_GETNODEINFO_ID: {
      *out_data_type_signature = UAVCAN_PROTOCOL_GETNODEINFO_REQUEST_SIGNATURE;
      return true;
    }
    }
  }
  if (transfer_type == CanardTransferTypeBroadcast) {
    switch (data_type_id) {
    case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID: {
      *out_data_type_signature =
          UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE;
      return true;
    }
    }
  }

  return false;
}

static void handle_canard_tx_rx_queue(void) {
  canardCleanupStaleTransfers(&canard, get_uptime_ms() * 1000ULL);
  HAL_Delay(1);
  for (CanardCANFrame *txf = NULL;
       (txf = canardPeekTxQueue(&canard)) != NULL;) {
    int32_t errCode =
        FDCAN_Send(txf->id | CANARD_CAN_FRAME_EFF, txf->data, txf->data_len);
    HAL_Delay(10);
    if (errCode == 0) {
      canardPopTxQueue(&canard);
    }
  }
}

static void task_1hz(void) { broadcast_node_status(); }

static void task_5hz(void) {
  // TODO
}

static void task_10hz(void) {
  // TODO
}

void canard_app_loop(void) {
  handle_canard_tx_rx_queue();

  uint32_t now = get_uptime_ms();

  switch (node_state) {

  case NODE_STATE_DNA:
    if (now >= DNA.send_next_node_id_allocation_request_at_ms) {
      request_DNA();
    }
    break;

  case NODE_STATE_OPERATIONAL:
    for (uint32_t i = 0; i < loop_task_count; i++) {
      if ((now - loop_tasks[i].last_run_ms) >= loop_tasks[i].period_ms) {
        loop_tasks[i].last_run_ms += loop_tasks[i].period_ms;
        loop_tasks[i].task();
      }
    }
    break;

  default:
    break;
  }
}

void canard_app_init(void) {
  // TODO
  //  get_hardware_id();
  canardInit(&canard, canard_memory_pool, sizeof(canard_memory_pool),
             onTransferReceived, shouldAcceptTransfer, NULL);

  if (UAVCAN_NODE_ID > 0) {
    set_node_id(UAVCAN_NODE_ID);
  } else {
    node_state = NODE_STATE_DNA;
  }
}
