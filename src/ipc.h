/*
 * GQmpeg
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#ifndef IPC_H
#define IPC_H


void ipc_send(gchar *command);
void ipc_on(void);
void ipc_off(void);
gint ipc_another_process_active(void);

void ipc_status_update(void);
void ipc_status_send(const gchar *text);
gint ipc_status_register(gchar *key, gchar *path);
void ipc_status_unregister(gchar *key);


#endif
