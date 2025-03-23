#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "cmsis_os.h"
#include "gpio.h"
#include "zenoh-pico.h"
#include "zenoh.h"

#define LINK_READY			(1 << 0)
#define MODE 				"peer"
#define LOCATOR 			"serial/ZUART#baudrate=921600"
#define KEYEXPR_LIVELINESS	"group1/zenoh-pico"
#define KEYEXPR_SUB 		"demo/example/**"
#define KEYEXPR_PUB_LOG 	"demo/example/log"
#define KEYEXPR_PUB_TICK 	"demo/example/ticks"

typedef StaticTask_t osStaticThreadDef_t;

/* Definitions for zenohReadTask */
static StackType_t zenohReadTaskBuffer[ 1024 ];
static StaticTask_t zenohReadTaskControlBlock;
static z_task_attr_t zenohReadTask_attributes = {
  .name = "zenohReadTask",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_depth = sizeof(zenohReadTaskBuffer) / sizeof(zenohReadTaskBuffer[0]),
  .static_allocation = true,
  .stack_buffer = zenohReadTaskBuffer,
  .task_buffer = &zenohReadTaskControlBlock
};
/* Definitions for zenohLinkTask */
static uint32_t zenohLinkTaskBuffer[ 512 ];
static osStaticThreadDef_t zenohLinkTaskControlBlock;
static const osThreadAttr_t zenohLinkTask_attributes = {
  .name = "zenohLinkTask",
  .cb_mem = &zenohLinkTaskControlBlock,
  .cb_size = sizeof(zenohLinkTaskControlBlock),
  .stack_mem = &zenohLinkTaskBuffer[0],
  .stack_size = sizeof(zenohLinkTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for zenohLeaseTask */
static StackType_t zenohLeaseTaskBuffer[ 512 ];
static StaticTask_t zenohLeaseTaskControlBlock;
static z_task_attr_t zenohLeaseTask_attributes = {
  .name = "zenohLeaseTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_depth = sizeof(zenohLeaseTaskBuffer) / sizeof(zenohLeaseTaskBuffer[0]),
  .static_allocation = true,
  .stack_buffer = zenohLeaseTaskBuffer,
  .task_buffer = &zenohLeaseTaskControlBlock
};

static z_owned_session_t session;
static z_owned_subscriber_t sub;
static z_owned_publisher_t log_pub;
static z_owned_publisher_t tick_pub;

static void StartZenohLinkTask(void *argument);
static void LivelinessHandler(z_loaned_sample_t *sample, void *ctx);
static void DataHandler(z_loaned_sample_t *sample, void *ctx);
static void QueryHandler(z_loaned_reply_t *reply, void *ctx);
static int Init();
static int Close();


// *****************************************************************************
//                     T O P   L E V E L   F U N C T I O N S
// *****************************************************************************


/**
 * Initializes the Zenoh services.
 *
 * @return 0 on success, a negative error code on failure
 */
int Z_Init()
{
	osThreadNew(StartZenohLinkTask, xTaskGetCurrentTaskHandle(), &zenohLinkTask_attributes);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	return 0;
}

/**
 * Sends the tick count to the tick topic.
 *
 * @param ticks the number of ticks
 * @return 0 on success, a negative error code on failure
 */
int Z_TickCount(uint32_t ticks)
{
	char str[16];
	snprintf(str, sizeof(str), "%li", ticks);

    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, str);
	z_publisher_put(z_loan(tick_pub), z_move(payload), NULL);

	return 0;
}

/**
 * Gets some value synchronously from queryable.
 *
 * @return 0 on success, a negative error code on failure
 */
int Z_GetSync()
{
	z_view_keyexpr_t ke;
	z_view_keyexpr_from_str(&ke, "demo/example/zenoh-pico-queryable");
	z_owned_fifo_handler_reply_t handler;
	z_owned_closure_reply_t closure;
	z_fifo_channel_reply_new(&closure, &handler, 16);
	z_get(z_loan(session), z_loan(ke), "params", z_move(closure), NULL);

	z_owned_reply_t reply;
	while (z_recv(z_loan(handler), &reply) == Z_OK) {
		if (z_reply_is_ok(z_loan(reply))) {
			const z_loaned_sample_t *sample = z_reply_ok(z_loan(reply));
			z_view_string_t keystr;
			z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
			z_owned_string_t reply_data;
			z_bytes_to_string(z_sample_payload(sample), &reply_data);
			if (strncmp("Queryable from Pico!",
					z_string_data(z_loan(reply_data)),
					z_string_len(z_loan(reply_data))) == 0) {
				HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
			}
			z_drop(z_move(reply_data));
		}
		z_drop(z_move(reply));
	}

	z_drop(z_move(handler));

	return 0;
}

/**
 * Gets some value asynchronously from queryable.
 *
 * @return 0 on success, a negative error code on failure
 */
int Z_GetAsync()
{
	z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, "demo/example/zenoh-pico-queryable");
    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, "Hello NUCLEO");
    z_get_options_t opts;
    z_get_options_default(&opts);
    opts.payload = z_move(payload);
	z_owned_closure_reply_t callback;
	z_closure(&callback, QueryHandler, NULL, NULL);
	if (z_get(z_loan(session), z_loan(ke), "test", z_move(callback), &opts) < 0) {
		return -1;
	}

	return 0;
}


// *****************************************************************************
//                                  T A S K S
// *****************************************************************************


HeapStats_t _heap_st;
void StartZenohLinkTask(void *argument)
{
	for(;;)
	{
		// Heap information to check memory leaks
		vPortGetHeapStats(&_heap_st);

		// Initialize
		if (Init() < 0) {
			Error_Handler();
		}

		// Loop until lease task fails
		xTaskNotifyGive((TaskHandle_t)argument);
		const z_loaned_session_t *zs = z_loan(session);
		while (_Z_RC_IN_VAL(zs)->_tp._transport._unicast._common._lease_task_running) {
			vPortGetHeapStats(&_heap_st);
			osDelay(100);
		}

		// Cleanup
		if (Close() < 0) {
			Error_Handler();
		}

		// Allow background tasks to cleanup themselves
		osDelay(100);
	}
}


//*****************************************************************************
//                               C A L L B A C K
// *****************************************************************************


/**
 * Called when subscribed liveliness tokens status is changed.
 *
 * @param sample Represents a data sample, i.e. the value associated to a given
 * topic at a given point in time
 * @param ctx the context associated to the callback
 */
static void LivelinessHandler(z_loaned_sample_t *sample, void *ctx)
{
	switch (z_sample_kind(sample)) {
		case Z_SAMPLE_KIND_PUT:
			HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
			break;
		case Z_SAMPLE_KIND_DELETE:
			HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
			break;
	}
}

/**
 * Subscriber callback.
 *
 * @param sample Represents a data sample, i.e. the value associated to a given
 * topic at a given point in time
 * @param ctx the context associated to the callback
 */
static void DataHandler(z_loaned_sample_t *sample, void *ctx)
{
	int val;

    // Sample handling of topics and values
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
    const char *k = z_string_data(z_loan(keystr));
    int kl = z_string_len(z_loan(keystr));
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);
    const char *v = z_string_data(z_loan(value));
    int vl = z_string_len(z_loan(value));

    if (strncmp(k, "demo/example/zenoh-pico-pub", kl) == 0) {
    	if (sscanf(v, "[%d]", &val) == 1) {
        	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, (val % 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    	}
    }

    z_drop(z_move(value));
}

/**
 * Called as a reply to a Z_GetRoutesAsync() call.
 *
 * @param reply Represents a matching query
 * @param ctx the context associated to the callback
 */
static void QueryHandler(z_loaned_reply_t *reply, void *ctx)
{
	if (z_reply_is_ok(reply)) {
		const z_loaned_sample_t *sample = z_reply_ok(reply);
		z_view_string_t keystr;
		z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
		z_owned_string_t reply_data;
		z_bytes_to_string(z_sample_payload(sample), &reply_data);

		if (strncmp("Queryable from Pico!",
				z_string_data(z_loan(reply_data)),
				z_string_len(z_loan(reply_data))) == 0) {
			HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		}

		z_drop(z_move(reply_data));
	}
}


// *****************************************************************************
//                     L O W   L E V E L   F U N C T I O N S
// *****************************************************************************


/**
 * Initializes the Zenoh services.
 *
 * @return 0 on success, a negative error code on failure
 */
static int Init()
{
	z_view_keyexpr_t ke;

	// Initialize session and other parameters
	z_owned_config_t config;
	z_config_default(&config);
	zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
	zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, LOCATOR);

	// Open the session
	// Note: this will initiate the connection sequence by sending the Z_INIT(Syn) message
	if (z_open(&session, z_move(config), NULL) <0 ) {
		return -EFAULT;
	}

	// Start the receive and the session lease loops
	zp_start_read_task(z_loan_mut(session), &(zp_task_read_options_t){.task_attributes = &zenohReadTask_attributes});
	zp_start_lease_task(z_loan_mut(session), &(zp_task_lease_options_t){.task_attributes = &zenohLeaseTask_attributes});

	// Declare the background liveliness subscriber
	z_view_keyexpr_from_str(&ke, KEYEXPR_LIVELINESS);
    z_owned_closure_sample_t livelinessCallback;
    z_closure(&livelinessCallback, LivelinessHandler, NULL, NULL);
    if (z_liveliness_declare_background_subscriber(z_loan(session), z_loan(ke), z_move(livelinessCallback), NULL) < 0) {
		return -EFAULT;
	}

	// Declare the subscriber
	z_view_keyexpr_from_str(&ke, KEYEXPR_SUB);
	z_owned_closure_sample_t callback;
	z_closure(&callback, DataHandler, NULL, NULL);
	if (z_declare_subscriber(z_loan(session), &sub, z_loan(ke), z_move(callback), NULL) < 0) {
		return -EFAULT;
	}

	// Declare the publishers
	z_view_keyexpr_from_str(&ke, KEYEXPR_PUB_LOG);
	if (z_declare_publisher(z_loan(session), &log_pub, z_loan(ke), NULL) < 0) {
		return -EFAULT;
	}
	z_view_keyexpr_from_str(&ke, KEYEXPR_PUB_TICK);
	if (z_declare_publisher(z_loan(session), &tick_pub, z_loan(ke), NULL) < 0) {
		return -EFAULT;
	}

	// Wait for the connection to be established
	// Note: the message sequence Z_INIT(Syn) -> Z_INIT(Ack) -> Z_OPEN(Syn) -> Z_OPEN(Ack)
	//	   will take place; after the sequence completes the peers will be connected and in sync
	// Note: be sure not to publish before the connection complete or messages will be dropped
	osDelay(100);

	return 0;
}

/**
 * Closes the Zenoh services.
 *
 * @return 0 on success, a negative error code on failure
 */
static int Close()
{
	// Cleanup publishers and subscriber
	z_drop(z_move(tick_pub));
	z_drop(z_move(log_pub));
	z_drop(z_move(sub));

	// Close the session (also stop read and lease tasks)
	z_drop(z_move(session));

	return 0;
}
