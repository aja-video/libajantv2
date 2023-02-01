/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA OEM boards.
//
// Filename: ntv2serial.c
// Purpose:	 Serial port control
//
///////////////////////////////////////////////////////////////

#include "ntv2serial.h"
#include "ajatypes.h"
#include "ntv2publicinterface.h"
#include "ntv2enums.h"
#include "ntv2kona2.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2devicefeatures.h"
#include "registerio.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0))
#define NTV2_USE_TTY_GROUP
#endif

/* debug messages */
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002
#define NTV2_DEBUG_SERIAL_STATE			0x00000010
#define NTV2_DEBUG_SERIAL_STREAM		0x00000020

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	((ntv2_debug_mask & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_SERIAL_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_SERIAL_ERROR(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_SERIAL_STATE(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_SERIAL_STATE, string, __VA_ARGS__)
#define NTV2_MSG_SERIAL_STREAM(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_SERIAL_STREAM, string, __VA_ARGS__)

static uint32_t ntv2_debug_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR | NTV2_DEBUG_SERIAL_STATE;

#define NTV2_SERIAL_CLOSE_TIMEOUT		10000000

/* serial status register */
#define ntv2_kona_reg_serial_status					0x0000
#define ntv2_kona_fld_serial_rx_valid				0x00000001
#define ntv2_kona_fld_serial_rx_full				0x00000002
#define ntv2_kona_fld_serial_tx_empty				0x00000004
#define ntv2_kona_fld_serial_tx_full				0x00000008
#define ntv2_kona_fld_serial_interrupt_state		0x00000010
#define ntv2_kona_fld_serial_error_overrun			0x00000020
#define ntv2_kona_fld_serial_error_frame			0x00000040
#define ntv2_kona_fld_serial_error_parity			0x00000080
#define ntv2_kona_fld_serial_int_active				0x00000100
#define ntv2_kona_fld_serial_loopback_state			0x40000000

/* serial control register */
#define ntv2_kona_reg_serial_control				0x0010
#define ntv2_kona_fld_serial_reset_tx				0x00000001
#define ntv2_kona_fld_serial_reset_rx				0x00000002
#define ntv2_kona_fld_serial_interrupt_enable		0x00000010
#define ntv2_kona_fld_serial_interrupt_clear		0x00000100
#define ntv2_kona_fld_serial_loopback_enable		0x40000000
#define ntv2_kona_fld_serial_rx_trigger				0x80000000

/* serial rx register */
#define ntv2_kona_reg_serial_rx						0x0020
#define ntv2_kona_fld_serial_rx_data				0x000000ff
#define ntv2_kona_fld_serial_rx_active				0x80000000

/* serial tx register */
#define ntv2_kona_reg_serial_tx						0x0030
#define ntv2_kona_fld_serial_tx_data				0x000000ff

static bool ntv2_serial_receive(struct ntv2_serial *ntv2_ser);
static bool ntv2_serial_transmit(struct ntv2_serial *ntv2_ser);
static void ntv2_serial_control(struct ntv2_serial *ntv2_ser, u32 clear_bits, u32 set_bits);
static void	ntv2_serial_route(struct ntv2_serial *ntv2_ser, bool enable);

static unsigned int ntv2_uartops_tx_empty(struct uart_port *port)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);
	u32 empty = ntv2_kona_fld_serial_tx_empty;
	unsigned long flags;
	unsigned int ret;

	/* empty if not enabled */
	if (!ntv2_ser->uart_enable)
		return TIOCSER_TEMT;

	spin_lock_irqsave(&port->lock, flags);
	ret = ntv2ReadRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_status);
	spin_unlock_irqrestore(&port->lock, flags);

	return (ret & empty)? TIOCSER_TEMT : 0;
}

static unsigned int ntv2_uartops_get_mctrl(struct uart_port *port)
{
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void ntv2_uartops_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	/* N/A */
}

static void ntv2_uartops_stop_tx(struct uart_port *port)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);

	NTV2_MSG_SERIAL_STREAM("%s: uart stop transmit\n", ntv2_ser->name);
}

static void ntv2_uartops_start_tx(struct uart_port *port)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);

	NTV2_MSG_SERIAL_STREAM("%s: uart start transmit\n", ntv2_ser->name);

	ntv2_serial_transmit(ntv2_ser);
}

static void ntv2_uartops_stop_rx(struct uart_port *port)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);

	NTV2_MSG_SERIAL_STREAM("%s: uart stop receive\n", ntv2_ser->name);

	/* don't forward any more data (like !CREAD) */
	port->ignore_status_mask = 
		ntv2_kona_fld_serial_rx_valid |
		ntv2_kona_fld_serial_error_overrun |
		ntv2_kona_fld_serial_error_frame |
		ntv2_kona_fld_serial_error_parity;
}

static void ntv2_uartops_break_ctl(struct uart_port *port, int ctl)
{
	/* N/A */
}

static int ntv2_uartops_startup(struct uart_port *port)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);
	int ret;

	NTV2_MSG_SERIAL_STREAM("%s: uart startup\n", ntv2_ser->name);

	/* enable serial port */
	ret = ntv2_serial_enable(ntv2_ser);

	return ret;
}

static void ntv2_uartops_shutdown(struct uart_port *port)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);

	NTV2_MSG_SERIAL_STREAM("%s: uart shutdown\n", ntv2_ser->name);

	ntv2_serial_disable(ntv2_ser);
}

static void ntv2_uartops_set_termios(struct uart_port *port,
									 struct ktermios *termios,
									 struct ktermios *old)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);
	u32 valid = ntv2_kona_fld_serial_rx_valid;
	u32 overrun = ntv2_kona_fld_serial_error_overrun;
	u32 frame = ntv2_kona_fld_serial_error_frame;
	u32 parity = ntv2_kona_fld_serial_error_parity;
	u32 full = ntv2_kona_fld_serial_tx_full;
	unsigned long flags;
	unsigned int baud;

	NTV2_MSG_SERIAL_STREAM("%s: uart set termios\n", ntv2_ser->name);

	spin_lock_irqsave(&port->lock, flags);

	port->read_status_mask = valid | overrun | full;

	if (termios->c_iflag & INPCK)
		port->read_status_mask |= parity | frame;

	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= parity | frame | overrun;

	/* ignore all characters if CREAD is not set */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= valid | parity | frame | overrun;

	/* update timeout */
	baud = uart_get_baud_rate(port, termios, old, 0, 460800);
	uart_update_timeout(port, termios->c_cflag, baud);

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *ntv2_uartops_type(struct uart_port *port)
{
	return port->type == PORT_UARTLITE ? "uartlite" : NULL;
}

static void ntv2_uartops_release_port(struct uart_port *port)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);

	NTV2_MSG_SERIAL_STREAM("%s: uart release port\n", ntv2_ser->name);

	ntv2_ser->busy = false;
}

static int ntv2_uartops_request_port(struct uart_port *port)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);

	NTV2_MSG_SERIAL_STREAM("%s: uart request port\n", ntv2_ser->name);

	/* try to allocate uart */
	if (ntv2_ser->busy)
		return -EBUSY;
	ntv2_ser->busy = true;

	return 0;
}

static void ntv2_uartops_config_port(struct uart_port *port, int flags)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);

	NTV2_MSG_SERIAL_STREAM("%s: uart config port\n", ntv2_ser->name);

//	if (!ntv2_uartops_request_port(port))
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_UARTLITE;
		ntv2_uartops_request_port(port);
	}
}

static int ntv2_uartops_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	struct ntv2_serial *ntv2_ser = container_of(port, struct ntv2_serial, uart_port);

	NTV2_MSG_SERIAL_STREAM("%s: uart verify port\n", ntv2_ser->name);

	return -EINVAL;
}

static struct uart_ops ntv2_uartops = {
	.tx_empty		= ntv2_uartops_tx_empty,
	.set_mctrl		= ntv2_uartops_set_mctrl,
	.get_mctrl		= ntv2_uartops_get_mctrl,
	.stop_tx		= ntv2_uartops_stop_tx,
	.start_tx		= ntv2_uartops_start_tx,
	.stop_rx		= ntv2_uartops_stop_rx,
	.break_ctl		= ntv2_uartops_break_ctl,
	.startup		= ntv2_uartops_startup,
	.shutdown		= ntv2_uartops_shutdown,
	.set_termios	= ntv2_uartops_set_termios,
	.type			= ntv2_uartops_type,
	.release_port	= ntv2_uartops_release_port,
	.request_port	= ntv2_uartops_request_port,
	.config_port	= ntv2_uartops_config_port,
	.verify_port	= ntv2_uartops_verify_port,
};

struct ntv2_serial *ntv2_serial_open(Ntv2SystemContext* sys_con,
									 const char *name, int index)
{
	struct ntv2_serial *ntv2_ser = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_ser = ntv2MemoryAlloc(sizeof(struct ntv2_serial));
	if (ntv2_ser == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_serial instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_ser, 0, sizeof(struct ntv2_serial));

	ntv2_ser->index = index;
	sprintf(ntv2_ser->name, "%s%d", name, index);
	ntv2_ser->system_context = sys_con;

	spin_lock_init(&ntv2_ser->state_lock);

	NTV2_MSG_SERIAL_INFO("%s: open ntv2_serial\n", ntv2_ser->name);

	return ntv2_ser;
}

void ntv2_serial_close(struct ntv2_serial *ntv2_ser)
{
	struct uart_port *port;

	if (ntv2_ser == NULL)
		return;

	NTV2_MSG_SERIAL_INFO("%s: close ntv2_serial\n", ntv2_ser->name);

	/* stop the uart */
	ntv2_serial_disable(ntv2_ser);

	port = &ntv2_ser->uart_port;
	if (port->iobase != 0) {
		uart_remove_one_port(getNTV2ModuleParams()->uart_driver, port);
		port->iobase = 0;
	}

	memset(ntv2_ser, 0, sizeof(struct ntv2_serial));
	ntv2MemoryFree(ntv2_ser, sizeof(struct ntv2_serial));
}

Ntv2Status ntv2_serial_configure(struct ntv2_serial *ntv2_ser,
								 Ntv2Register uart_reg,
								 Ntv2Register route_reg,
								 u32 route_mask)
{
	struct uart_port *port;
	int index;
	int ret;

	if (ntv2_ser == NULL)
		return NTV2_STATUS_FAIL;

	NTV2_MSG_SERIAL_INFO("%s: configure serial device\n", ntv2_ser->name);

	ntv2_ser->uart_reg = uart_reg;
	ntv2_ser->route_reg = route_reg;
	ntv2_ser->route_mask = route_mask;

	/* get next serial port index */
	index = atomic_inc_return(&getNTV2ModuleParams()->uart_index) - 1;
	if (index >= getNTV2ModuleParams()->uart_max) {
		NTV2_MSG_SERIAL_ERROR("%s: ntv2_serial too many uarts %d\n", ntv2_ser->name, index + 1);
		return NTV2_STATUS_NO_MEMORY;
	}

	/* configure the serial port */
	port = &ntv2_ser->uart_port;
	port->fifosize = 16;
	port->regshift = 2;
	port->iotype = UPIO_MEM;
	port->iobase = 1; /* mark port in use */
	port->ops = &ntv2_uartops;
	port->flags = UPF_BOOT_AUTOCONF;
	port->dev = &ntv2_ser->system_context->pDevice->dev;
	port->type = PORT_UNKNOWN;
	port->line = (unsigned int)index;

	NTV2_MSG_SERIAL_INFO("%s: register serial device: %s  port: %d\n",
						 ntv2_ser->name, NTV2_TTY_NAME, index);

	/* register the serial port */
	ret = uart_add_one_port(getNTV2ModuleParams()->uart_driver, port);
	if (ret < 0) {
		NTV2_MSG_SERIAL_ERROR("%s: uart_add_one_port() failed %d  port: %d\n",
							  ntv2_ser->name, ret, index);
		port->iobase = 0;
		return ret;
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_serial_enable(struct ntv2_serial *ntv2_ser)
{
	u32 reset_tx = ntv2_kona_fld_serial_reset_tx;
	u32 reset_rx = ntv2_kona_fld_serial_reset_rx;
	u32 enable = ntv2_kona_fld_serial_interrupt_enable;
	unsigned long flags;

	if (ntv2_ser == NULL)
		return NTV2_STATUS_FAIL;

	spin_lock_irqsave(&ntv2_ser->state_lock, flags);

	if (ntv2_ser->uart_enable) {
		spin_unlock_irqrestore(&ntv2_ser->state_lock, flags);
		return NTV2_STATUS_SUCCESS;
	}

	NTV2_MSG_SERIAL_STATE("%s: serial state: enable\n", ntv2_ser->name);

	ntv2_ser->uart_enable = true;

	/* connect uart */
	ntv2_serial_route(ntv2_ser, true);

	/* enable interrupt */
	ntv2_serial_control(ntv2_ser, 0, reset_tx | reset_rx | enable);

	spin_unlock_irqrestore(&ntv2_ser->state_lock, flags);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_serial_disable(struct ntv2_serial *ntv2_ser)
{
	u32 enable = ntv2_kona_fld_serial_interrupt_enable;
	unsigned long flags;

	if (ntv2_ser == NULL)
		return NTV2_STATUS_FAIL;

	spin_lock_irqsave(&ntv2_ser->state_lock, flags);
	
	if (!ntv2_ser->uart_enable) {
		spin_unlock_irqrestore(&ntv2_ser->state_lock, flags);
		return NTV2_STATUS_SUCCESS;
	}

	NTV2_MSG_SERIAL_STATE("%s: serial state: disable\n", ntv2_ser->name);

	ntv2_ser->uart_enable = false;

	/* disable interrupt */
	ntv2_serial_control(ntv2_ser, enable, 0);

	/* disconnect uart */
	ntv2_serial_route(ntv2_ser, false);

	spin_unlock_irqrestore(&ntv2_ser->state_lock, flags);

	return NTV2_STATUS_SUCCESS;
}

bool ntv2_serial_active(struct ntv2_serial *ntv2_ser)
{
	if (ntv2_ser == NULL)
		return false;
	
	return ntv2_ser->uart_enable;
}


Ntv2Status ntv2_serial_interrupt(struct ntv2_serial *ntv2_ser)
{
	struct uart_port *port = &ntv2_ser->uart_port;
	u32 active = ntv2_kona_fld_serial_int_active;
	u32 clear = ntv2_kona_fld_serial_interrupt_clear;
	u32 status;
	bool busy;
	int count = 0;
	unsigned long flags;

	if (ntv2_ser == NULL)
		return NTV2_STATUS_FAIL;

	spin_lock_irqsave(&ntv2_ser->state_lock, flags);

	/* is interrupt active */
	status = ntv2ReadRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_status);
	if ((status & active) == 0) {
		spin_unlock_irqrestore(&ntv2_ser->state_lock, flags);
		return NTV2_STATUS_SUCCESS;
	}

	/* clear interrupt */
	ntv2_serial_control(ntv2_ser, 0, clear);

	/* check enabled */
	if (!ntv2_ser->uart_enable) {
		spin_unlock_irqrestore(&ntv2_ser->state_lock, flags);
		return NTV2_STATUS_SUCCESS;
	}

	NTV2_MSG_SERIAL_STREAM("%s: uart interrupt status %08x\n", ntv2_ser->name, status);

	/* manage uart */
	do {
		busy  = ntv2_serial_receive(ntv2_ser);
		busy |= ntv2_serial_transmit(ntv2_ser);
		count++;
	} while (busy);

	if (count > 1)
		tty_flip_buffer_push(&port->state->port);

	spin_unlock_irqrestore(&ntv2_ser->state_lock, flags);

	return NTV2_STATUS_SUCCESS;
}

static bool ntv2_serial_receive(struct ntv2_serial *ntv2_ser)
{
	struct uart_port *port = &ntv2_ser->uart_port;
	struct tty_port *tport = &port->state->port;
	u32 valid = ntv2_kona_fld_serial_rx_valid;
	u32 overrun = ntv2_kona_fld_serial_error_overrun;
	u32 frame = ntv2_kona_fld_serial_error_frame;
	u32 parity = ntv2_kona_fld_serial_error_parity;
	u32 trigger = ntv2_kona_fld_serial_rx_trigger;
	u32 active = ntv2_kona_fld_serial_rx_active;
	u32 status;
	u32 rx = 0;
	int i;

	char flag = TTY_NORMAL;

	status = ntv2ReadRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_status);
	if ((status & (valid | overrun | frame)) == 0)
		return false;

	/* gather statistics */
	if ((status & valid) != 0) {
		port->icount.rx++;

		/* trigger read of uart rx fifo */
		ntv2_serial_control(ntv2_ser, 0, trigger);

		/* read rx data from pci */
		for (i = 0; i < 10; i++) {
			rx = ntv2ReadRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_rx);
			if ((rx & active) == 0)
				break;
		}
			
		NTV2_MSG_SERIAL_STREAM("%s: uart rx %02x  busy %d\n", ntv2_ser->name, (u8)rx, i);

		if ((status & parity) != 0)
			port->icount.parity++;
	}

	if ((status & overrun) != 0)
		port->icount.overrun++;

	if ((status & frame) != 0)
		port->icount.frame++;

	/* drop byte with parity error if IGNPAR specificed */
	if ((status & port->ignore_status_mask & parity) != 0)
		status &= ~valid;

	status &= port->read_status_mask;

	if ((status & parity) != 0)
		flag = TTY_PARITY;

	status &= ~port->ignore_status_mask;

	if ((status & valid) != 0)
		tty_insert_flip_char(tport, (u8)rx, flag);

	if ((status & overrun) != 0)
		tty_insert_flip_char(tport, 0, TTY_OVERRUN);

	if ((status & frame) != 0)
		tty_insert_flip_char(tport, 0, TTY_FRAME);

	return true;
}

static bool ntv2_serial_transmit(struct ntv2_serial *ntv2_ser)
{
	struct uart_port *port = &ntv2_ser->uart_port;
	struct circ_buf *xmit  = &port->state->xmit;
	u32 full = ntv2_kona_fld_serial_tx_full;
	u32 status;

	status = ntv2ReadRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_status);
	if (status & full)
		return false;

	/* tx xon/xoff */
	if ((port->x_char) != 0) {
		NTV2_MSG_SERIAL_STREAM("%s: uart tx %02x\n", ntv2_ser->name, (u8)port->x_char);
		ntv2WriteRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_tx, (u32)port->x_char);
		port->x_char = 0;
		port->icount.tx++;
		return true;
	}

	if (uart_circ_empty(xmit) || uart_tx_stopped(port))
		return false;

	/* tx data */
	NTV2_MSG_SERIAL_STREAM("%s: uart tx %02x\n", ntv2_ser->name, (u8)xmit->buf[xmit->tail]);
	ntv2WriteRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_tx, (u32)xmit->buf[xmit->tail]);
	xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE-1);
	port->icount.tx++;

	/* wake up */
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	return true;
}

static void ntv2_serial_control(struct ntv2_serial *ntv2_ser, u32 clear_bits, u32 set_bits)
{
	u32 enable = ntv2_kona_fld_serial_interrupt_enable;
	u32 loop = ntv2_kona_fld_serial_loopback_enable;
	u32 status;

	if (ntv2_ser == NULL)
		return;

	/* read current status */
	status = ntv2ReadRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_status);

	/* filter non state bits */
	status &= enable | loop;

	/* clear and set bits */
	status = (status & ~clear_bits) | set_bits;

	/* write control */
	ntv2WriteRegister32(ntv2_ser->uart_reg + ntv2_kona_reg_serial_control, status);
}

static void	ntv2_serial_route(struct ntv2_serial *ntv2_ser, bool connect)
{
	u32 status;

	if ((ntv2_ser == NULL) ||
		(ntv2_ser->route_reg == 0) ||
		(ntv2_ser->route_mask == 0))
		return;

	/* read current enable register */
	status = ntv2ReadRegister32(ntv2_ser->route_reg);

	/* clear or set bit */
	if (connect)
	{
		status |= ntv2_ser->route_mask;
	}
	else
	{
		status &= (~ntv2_ser->route_mask);
	}

	/* write enable register */
	ntv2WriteRegister32(ntv2_ser->route_reg, status);
}
