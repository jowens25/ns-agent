/*****************************************************************************
 * Project			: nsAgent
 * Author			: Bryan Wilcutt bwilcutt@yahoo.com
 * Date				: 5-13-18
 * System			: Nano PI
 * File				: nsTrap.c
 *
 * Description		:
 *
 * This file contains the trap handlers for the nsAgent.  Currently supported
 * traps include:
 *
 * Power Supply
 * Temperature
 * Channel
 * GPS Lock/Unlock
 *
 * Written for Novus Power.
 *
 * Copyright (c) Novus Power All Rights Reserved
 *****************************************************************************/

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "novusAgent.h"
#include "nsTrap.h"
#include "nsStartup.h"

extern int traps;
extern float ps_temp_high;
extern float ps_temp_low;

void send_novus_notification(unsigned int clientreg, void *clientarg);
void trap_ps();
void trap_temp();
void trap_chan();
void trap_gps();
void trap_ant();

void trap_major_alarm();
void trap_minor_alarm();

char trap_status[TRAP_COUNT];

/* Constructs for sending traps */

static const oid snmp_trap_oid[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

static const size_t snmp_trap_oid_len = OID_LENGTH(snmp_trap_oid);
static const size_t snmp_trap_msg_len = OID_LENGTH(nsTrapMsg_oid);

/************************************************
 * Function       : init_notification
 * Input          : None
 * Output         : None
 * Description    : Initializes a 1 second repeating
 * 	timer from SNMP that will call the given callback
 * 	method to check SNMP traps.
 ************************************************/
void init_notification()
{
	snmp_alarm_register(1,						 /* seconds */
						SA_REPEAT,				 /* repeat (every 30 seconds). */
						send_novus_notification, /* our callback */
						NULL);					 /* no callback data needed */
}

/************************************************
 * Function       : send_novus_notification
 * Input          : int ClientReg (ignored)
 * 				   void *ClientReg (ignored)
 * Output         : None
 * Description    : This is the callback method called
 * 	each time our SNMP timer expires.  It is responsible
 * 	for sending traps based on specific events.
 ************************************************/
void send_novus_notification(unsigned int clientreg, void *clientarg)
{
	/* Power Supply Traps */

	trap_ps();

	/* Temp traps */

	trap_temp();

	/* Channel Traps */

	trap_chan();

	/* Misc traps */

	trap_gps();

	trap_ant();

	trap_major_alarm();
	trap_minor_alarm();
}

/************************************************
 * Function       : trap_ps
 * Input          : netsnmp_variable_list **
 * Output         : n = Traps added, 0 = No traps added
 * Description    : Sends power supply event traps.
 ************************************************/
void trap_ps()
{
	unsigned int fault;
	netsnmp_variable_list *nv = NULL;
	oid ps_trap[sizeof(nsPS1Status_oid)];
	char trapmsg[45];

	strcpy(trapmsg, radio.nsFaultPowerSupplyByte);
	if (strlen(trapmsg) == 4) // && strncmp(trapmsg, "0x", 4) == 0)
	{
		sscanf(&trapmsg[2], "%X", &fault);

		/* Indicate ps is faulted or not. */

		for (int i = 0; i < 8; i++)
		{
			nv = NULL;

			memcpy(ps_trap, nsPS1Status_oid, sizeof(nsPS1Status_oid));
			ps_trap[10] = (i + 1);

			if ((fault & (1 << i)) && trap_status[TRAP_NSPS1STATUS + i] != PS_FAULT)
			{
				trap_status[TRAP_NSPS1STATUS + i] = PS_FAULT;

				// Set up trap

				snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)&ps_trap, NS_OID_LEN * sizeof(oid));

				// Indicate msg of trap LOW

				sprintf(trapmsg, "PS %d FAULT", i + 1);
				snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
				send_v2trap(nv);
				snmp_free_varbind(nv);
			}
			else if (!(fault & (1 << i)) && trap_status[TRAP_NSPS1STATUS + i] != PS_OK)
			{
				trap_status[TRAP_NSPS1STATUS + i] = PS_OK;

				// Set up trap

				snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)&ps_trap, NS_OID_LEN * sizeof(oid));

				// Indicate msg of trap LOW

				sprintf(trapmsg, "PS %d OK", i + 1);
				snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
				send_v2trap(nv);
				snmp_free_varbind(nv);
			}
		}
	}
}

/************************************************
 * Function       : trap_temp
 * Input          : netsnmp_variable_list **
 * Output         : n = Traps added, 0 = No traps added
 * Description    : Sends temperature event traps.
 ************************************************/
void trap_temp()
{
	int val;
	netsnmp_variable_list *nv = NULL;
	char trapmsg[45];

	val = atoi(radio.nsPSTemp);

	if (val < ps_temp_low && trap_status[TRAP_NSPSTEMP] != TEMP_LOW)
	{
		trap_status[TRAP_NSPSTEMP] = TEMP_LOW;

		// Set up trap

		snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)nsPSTemp_oid, sizeof(nsPSTemp_oid));

		// Indicate msg of trap LOW

		sprintf(trapmsg, "PS TEMP LOW");
		snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
		send_v2trap(nv);
		snmp_free_varbind(nv);
	}
	else if (val > ps_temp_high && trap_status[TRAP_NSPSTEMP] != TEMP_HIGH)
	{
		trap_status[TRAP_NSPSTEMP] = TEMP_HIGH;

		// Set up trap

		snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)nsPSTemp_oid, sizeof(nsPSTemp_oid));

		// Indicate msg of trap LOW

		sprintf(trapmsg, "PS TEMP HIGH");
		snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
		send_v2trap(nv);
		snmp_free_varbind(nv);
	}
	else if (trap_status[TRAP_NSPSTEMP] != TEMP_OK)
	{
		trap_status[TRAP_NSPSTEMP] = TEMP_OK;

		// Set up trap

		snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)nsPSTemp_oid, sizeof(nsPSTemp_oid));

		// Indicate msg of trap LOW

		sprintf(trapmsg, "PS TEMP OK");
		snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
		send_v2trap(nv);
		snmp_free_varbind(nv);
	}
}

/************************************************
 * Function       : trap_chan
 * Input          : netsnmp_variable_list **
 * Output         : None
 * Description    : Sends channel event traps.
 ************************************************/
void trap_chan()
{
	unsigned int fault;
	netsnmp_variable_list *nv = NULL;
	oid ch_trap[sizeof(nsChannel1Vrms_oid)];
	char trapmsg[45];

	/* Indicate chan is faulted or not. */

	strcpy(trapmsg, radio.nsFaultChannelBytes);
	if (strlen(trapmsg) == 6) // && strncmp(trapmsg, "0x", 2) == 0)
	{
		sscanf(&trapmsg[2], "%X", &fault);

		for (int i = 0; i < 16; i++)
		{
			nv = NULL;
			memcpy(ch_trap, nsChannel1Vrms_oid, sizeof(nsChannel1Vrms_oid));

			ch_trap[10] = (i + 1);

			if ((fault & (1 << i)) && trap_status[TRAP_NSCHANNEL1VRMS + i] != CHAN_FAULT)
			{
				trap_status[TRAP_NSCHANNEL1VRMS + i] = CHAN_FAULT;

				// Set up trap

				snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)&ch_trap, sizeof(ch_trap));

				// Indicate msg of trap LOW

				sprintf(trapmsg, "CHANNEL %d FAULT", i + 1);
				snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
				send_v2trap(nv);
				snmp_free_varbind(nv);
			}
			else if (!(fault & (1 << i)) && trap_status[TRAP_NSCHANNEL1VRMS + i] != CHAN_OK)
			{
				trap_status[TRAP_NSCHANNEL1VRMS + i] = CHAN_OK;

				// Set up trap

				snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)&ch_trap, sizeof(ch_trap));

				// Indicate msg of trap LOW

				sprintf(trapmsg, "CHANNEL %d OK", i + 1);
				snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
				send_v2trap(nv);
				snmp_free_varbind(nv);
			}
		}
	}
}

/************************************************
 * Function       : trap_gps
 * Input          : None
 * Output         : None
 * Description    : Sends gps lock event traps.
 ************************************************/
void trap_gps()
{
	netsnmp_variable_list *nv = NULL;
	char trapmsg[45];

	/*
	 * add in the additional objects defined as part of the trap
	 */

	if (radio.nsSysGPSLock[0] == 'A' && trap_status[TRAP_NSEVENTGPSLOCK] != GPS_LOCK)
	{
		snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)&nsEventGPSLock_oid, NS_OID_LEN * sizeof(oid));

		sprintf(trapmsg, "GPS LOCK");
		snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
		send_v2trap(nv);
		snmp_free_varbind(nv);
		trap_status[TRAP_NSEVENTGPSLOCK] = GPS_LOCK;
		syslog(LOG_INFO, "Sent Trap GPS LOCK\n");
	}
	else if (radio.nsSysGPSLock[0] == 'V' && trap_status[TRAP_NSEVENTGPSLOCK] != GPS_UNLOCK)
	{
		snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)&nsEventGPSLock_oid, NS_OID_LEN * sizeof(oid));

		sprintf(trapmsg, "GPS UNLOCK");
		snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
		send_v2trap(nv);
		snmp_free_varbind(nv);
		trap_status[TRAP_NSEVENTGPSLOCK] = GPS_UNLOCK;
		syslog(LOG_INFO, "Sent Trap GPS UNLOCK\n");
	}
}

void trap_ant()
{
	double val;
	netsnmp_variable_list *nv = NULL;
	char trapmsg[45];
	int new_state;

	val = atof(radio.nsMeasureAnt);

	if (val <= 0.001)
	{
		new_state = VOLT_LOW;
		sprintf(trapmsg, "ANT VOLT LOW: %s", radio.nsMeasureAnt);
	}

	else
	{
		new_state = VOLT_OK;
		sprintf(trapmsg, "ANT VOLT OK: %s", radio.nsMeasureAnt);
	}

	if (new_state == trap_status[TRAP_NSANTVOLTAGE])
	{
		return;
	}
	else
	{
		trap_status[TRAP_NSANTVOLTAGE] = new_state;
	}

	snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)nsMeasureAnt_oid, sizeof(nsMeasureAnt_oid));
	snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
	send_v2trap(nv);
	snmp_free_varbind(nv);
}

// nsMeasureAnt

static const char *major_alarm_names[] = {
	"MAJOR: FREQUENCY ERROR ($PSVAR)", // bit 0
	"MAJOR: PPS OUT OF SPEC ($PSDIF)", // bit 1
	"MAJOR: BELOW (SATSTH2)",
	"MAJOR: RECEIVER FAULT", // add new alarms here, one line per bit
};
#define NUM_MAJOR_ALARMS (sizeof(major_alarm_names) / sizeof(major_alarm_names[0]))

void trap_major_alarm()
{
	int val;
	netsnmp_variable_list *nv = NULL;
	char trapmsg[128] = "";
	size_t len = 0;
	int new_state;

	val = atoi(radio.nsMajorAlarm);

	if (val == 0)
	{
		snprintf(trapmsg, sizeof(trapmsg), "MAJOR CLEARED");
		new_state = ALARM_CLEAR;
	}
	else
	{
		new_state = ALARM_SET;

		for (unsigned i = 0; i < NUM_MAJOR_ALARMS; i++)
		{
			if (!(val & (1 << i)))
				continue;

			int n = snprintf(trapmsg + len, sizeof(trapmsg) - len, "%s%s", len ? " | " : "", major_alarm_names[i]);
			if (n < 0 || (size_t)n >= sizeof(trapmsg) - len)
			{
				len = sizeof(trapmsg) - 1; // truncated, stop appending
				break;
			}
			len += n;
		}
	}

	if (new_state == trap_status[TRAP_MAJOR_ALARM])
	{
		return;
	}
	else
	{
		trap_status[TRAP_MAJOR_ALARM] = new_state;
	}

	snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)nsMajorAlarm_oid, sizeof(nsMajorAlarm_oid));
	snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
	send_v2trap(nv);
	snmp_free_varbind(nv);
}

static const char *minor_alarm_names[] = {
	"MINOR: BELOW SATSTH1", // bit 0
	"MINOR: UNIT REBOOTED", // bit 1
							// add new alarms here, one line per bit
};
#define NUM_MINOR_ALARMS (sizeof(minor_alarm_names) / sizeof(minor_alarm_names[0]))

void trap_minor_alarm()
{
	int val;
	netsnmp_variable_list *nv = NULL;
	char trapmsg[128] = "";
	size_t len = 0;
	int new_state;

	val = atoi(radio.nsMinorAlarm);

	if (val == 0)
	{
		snprintf(trapmsg, sizeof(trapmsg), "MINOR CLEARED");
		new_state = ALARM_CLEAR;
	}
	else
	{
		new_state = ALARM_SET;

		for (unsigned i = 0; i < NUM_MINOR_ALARMS; i++)
		{
			if (!(val & (1 << i)))
				continue;

			if (val & (1 << 1))
			{
				// if the unit has been reset, clear the error
				writeRadio("$CLRRST\r\n");
			}

			int n = snprintf(trapmsg + len, sizeof(trapmsg) - len, "%s%s", len ? " | " : "", minor_alarm_names[i]);
			if (n < 0 || (size_t)n >= sizeof(trapmsg) - len)
			{
				len = sizeof(trapmsg) - 1; // truncated, stop appending
				break;
			}
			len += n;
		}
	}

	if (new_state == trap_status[TRAP_MINOR_ALARM])
	{
		return;
	}
	else
	{
		trap_status[TRAP_MINOR_ALARM] = new_state;
	}

	snmp_varlist_add_variable(&nv, snmp_trap_oid, snmp_trap_oid_len, ASN_OBJECT_ID, (u_char *)nsMinorAlarm_oid, sizeof(nsMinorAlarm_oid));
	snmp_varlist_add_variable(&nv, nsTrapMsg_oid, snmp_trap_msg_len, ASN_OCTET_STR, (u_char *)trapmsg, strlen(trapmsg));
	send_v2trap(nv);
	snmp_free_varbind(nv);
}