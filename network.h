#ifndef __DBUS2VDR_NETWORK_H
#define __DBUS2VDR_NETWORK_H

#include <gio/gio.h>

#include <vdr/plugin.h>

#include "connection.h"
#include "mainloop.h"


class cDBusNetworkAddress
{
private:
  cString _address;

public:
  const cString Host;
  const int     Port;

  cDBusNetworkAddress(const char *host, int port)
   :Host(host),Port(port) {};
  virtual ~cDBusNetworkAddress(void) {};

  const char *Address(void);

  static cDBusNetworkAddress *LoadFromFile(const char *Filename);
};

class cDBusNetwork
{
private:
  static gboolean  do_connect(gpointer user_data);
  static gboolean  do_disconnect(gpointer user_data);
  static void      on_file_changed(GFileMonitor *monitor, GFile *first, GFile *second, GFileMonitorEvent event, gpointer user_data);
  static void      on_name_acquired(cDBusConnection *Connection, gpointer UserData);
  static void      on_name_lost(cDBusConnection *Connection, gpointer UserData);

  gchar               *_busname;
  gchar               *_filename;
  GMainContext        *_context;
  GFileMonitor        *_monitor;
  GMainContext        *_monitor_context;
  cDBusMainLoop       *_monitor_loop;
  gulong               _signal_handler_id;
  cDBusNetworkAddress *_address;
  cDBusConnection     *_connection;

  cPlugin            *_avahi4vdr;
  cString             _avahi_name;
  cString             _avahi_id;

public:
  cDBusNetwork(const char *Busname, const char *Filename, GMainContext *Context);
  virtual ~cDBusNetwork(void);

  const char *Name(void) const { return "NetworkHandler"; }

  // "Start" is async
  void  Start(void);
  // "Stop" blocks
  void  Stop(void);
};

class cDBusNetworkClient : public cListObject
{
private:
  static void  OnConnect(cDBusConnection *Connection, gpointer UserData);
  static void  OnDisconnect(cDBusConnection *Connection, gpointer UserData);
  static void  OnTimerChange(const gchar *SenderName, const gchar *ObjectPath, const gchar *Interface, const gchar *Signal, GVariant *Parameters, gpointer UserData);
  static void  OnTimerList(GVariant *Reply, gpointer UserData);

  static GMainContext *_context;
  static cList<cDBusNetworkClient> _clients;
  static cPlugin      *_avahi4vdr;
  static cString       _avahi_browser_id;

  gchar *_name;
  gchar *_host;
  gchar *_address;
  int    _port;
  gchar *_busname;

  cDBusConnection *_connection;
  cDBusSignal     *_signal_timer_change;

public:
  cDBusNetworkClient(const char *Name, const char *Host, const char *Address, int Port, const char *Busname);
  virtual ~cDBusNetworkClient(void);

  const char  *Name() const { return _name; }
  const char  *Host() const { return _host; }
  const char  *Address() const { return _address; }
  int          Port() const { return _port; }
  const char  *Busname() const { return _busname; }

  virtual int Compare(const cListObject &ListObject) const;

  static bool  StartClients(GMainContext *Context);
  static void  StopClients(void);
  static const char *AvahiBrowserId(void) { return *_avahi_browser_id; }
  static void  RemoveClient(const char *Name);
};

#endif
