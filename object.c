#include "object.h"
#include "connection.h"


const GDBusInterfaceVTable cDBusObject::_interface_vtable =
{
  cDBusObject::handle_method_call,
  NULL,
  NULL
};

void  cDBusObject::handle_method_call(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
  if (user_data == NULL)
     return;

  dsyslog("dbus2vdr: handle_method_call: sender '%s', object '%s', interface '%s', method '%s'", sender, object_path, interface_name, method_name);
  cDBusObject *obj = (cDBusObject*)user_data;
  for (cDBusMethod *m = obj->_methods.First(); m; m = obj->_methods.Next(m)) {
      if (g_strcmp0(m->_name, method_name) == 0) {
         m->_method(obj, parameters, invocation);
         return;
         }
      }

  obj->HandleMethodCall(connection, sender, object_path, interface_name, method_name, parameters, invocation);
}

cDBusObject::cDBusObject(const char *Path, const char *XmlNodeInfo)
{
  _path = g_strdup(Path);
  _registration_ids = NULL;
  _connection = NULL;

  GError *err = NULL;
  _introspection_data = g_dbus_node_info_new_for_xml(XmlNodeInfo, &err);
  if (err != NULL) {
     esyslog("dbus2vdr: g_dbus_node_info_new_for_xml reports: %s", err->message);
     g_error_free(err);
     }
}

cDBusObject::~cDBusObject(void)
{
  if (_introspection_data != NULL) {
     g_dbus_node_info_unref(_introspection_data);
     _introspection_data = NULL;
     }

  if (_path != NULL) {
     g_free(_path);
     _path = NULL;
     }
  if (_registration_ids != NULL) {
     g_array_free(_registration_ids, TRUE);
     _registration_ids = NULL;
     }
}

void  cDBusObject::Register(void)
{
  if ((_connection == NULL) || (_connection->GetConnection() == NULL))
     return;

  if (_registration_ids != NULL)
     Unregister();
  int len = 0;
  while (_introspection_data->interfaces[len] != NULL)
        len++;
  if (_registration_ids != NULL)
     g_array_free(_registration_ids, TRUE);
  _registration_ids = g_array_new(FALSE, TRUE, sizeof(guint));
  for (int i = 0; i < len; i++) {
      guint id = g_dbus_connection_register_object(_connection->GetConnection(), Path(), _introspection_data->interfaces[i], &_interface_vtable, this, NULL, NULL);
      g_array_append_val(_registration_ids, id);
      dsyslog("dbus2vdr: %s: register object %s with interface %s on id %d", _connection->Name(), Path(), _introspection_data->interfaces[i]->name, id);
      }
}

void  cDBusObject::Unregister(void)
{
  if (_registration_ids != NULL) {
     if ((_connection != NULL) && (_connection->GetConnection() != NULL)) {
        for (guint i = 0; i < _registration_ids->len; i++) {
            guint id = g_array_index(_registration_ids, guint, i);
            if (id != 0) {
               dsyslog("dbus2vdr: %s: unregister object %s with interface %s on id %d", _connection->Name(), Path(), _introspection_data->interfaces[i]->name, id);
               g_dbus_connection_unregister_object(_connection->GetConnection(), id);
               }
            }
        }
     g_array_free(_registration_ids, TRUE);
     _registration_ids = NULL;
     }
}

void  cDBusObject::SetPath(const char *Path)
{
  if (_path != NULL) {
     g_free(_path);
     _path = NULL;
     }
  _path = g_strdup(Path);
}

void  cDBusObject::AddMethod(const char *Name, cDBusMethodFunc Method)
{
  if ((Name != NULL) && (Method != NULL))
     _methods.Add(new cDBusMethod(Name, Method));
}

void  cDBusObject::HandleMethodCall(GDBusConnection       *connection,
                                    const gchar           *sender,
                                    const gchar           *object_path,
                                    const gchar           *interface_name,
                                    const gchar           *method_name,
                                    GVariant              *parameters,
                                    GDBusMethodInvocation *invocation)
{
  gchar *message = g_strdup_printf("method '%s.%s' on object '%s' is not implemented yet", interface_name, method_name, object_path);
  g_dbus_method_invocation_return_error(invocation, G_IO_ERROR, G_IO_ERROR_FAILED_HANDLED, message);
  g_free(message);
}
