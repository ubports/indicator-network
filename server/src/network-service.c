#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <nm-utils.h>
#include <nm-client.h>
#include <nm-remote-settings.h>
#include <nm-device-wifi.h>
#include <libdbusmenu-glib/dbusmenu-glib.h>

GMainLoop *loop;

/*
 * unity.widgets.systemsettings.*.sectiontitle
 * Build properties:
 *    "type"             - string = "x-system-settings"
 *    "children-display" - string = "inline"
 *    "x-tablet-widget"  - string = "unity.widgets.systemsettings.tablet.sectiontitle"
 * Other properties:
 *    "label"  - string  - Optional, may have label or not
 *    "x-busy" - boolean - Shows a progress indicator
 *
 * unity.widgets.systemsettings.*.accesspoint
 * Build properties:
 *    "type"            - string = "x-system-settings"
 *    "toggle-type"     - string = "radio"
 *    "x-tablet-widget" - string = "unity.widgets.systemsettings.tablet.accesspoint"
 * Other properties:
 *    "x-wifi-strength"   - int    - Signal strength
 *    "x-wifi-is-adhoc"   - bool   - Whether it is an adhoc network or not
 *    "x-wifi-is-secure"  - bool   - Whether the network is open or requires password
 *    "x-wifi-bssid"      - string - The internal unique id for the AP
 */

static void
access_point_selected (DbusmenuMenuitem *item,
                       guint             timestamp,
                       gpointer          data)
{
  gint              i;
  NMDeviceWifi     *device          = (NMDeviceWifi*)data;
  const GPtrArray  *apsarray        = nm_device_wifi_get_access_points (device);
  const gchar      *bssid           = dbusmenu_menuitem_property_get (item, "x-wifi-bssid");
  NMAccessPoint    *ap              = NULL;

  for (i=0; i<apsarray->len; i++)
    {
      NMAccessPoint **aps = (NMAccessPoint**)(apsarray->pdata);
      if (g_strcmp0 (nm_access_point_get_bssid (aps[i]), bssid) == 0)
        {
          ap = aps[i];
          break;
        }
    }

  if (ap != NULL)
    {
      NMRemoteSettings *rs              = nm_remote_settings_new (NULL);
      GSList           *rs_connections  = nm_remote_settings_list_connections (rs);
      GSList           *dev_connections = nm_device_filter_connections (NM_DEVICE (device), rs_connections);
      GSList           *connections     = nm_access_point_filter_connections (ap, dev_connections);

      if (g_slist_length (connections) < 1)
        {
          /*TODO: Fill all the arguments
          nm_client_add_and_activate_connection (client, //Where do I get the client from :/
                                                 NMConnection *partial,
                                                 device,
                                                 (const char*)specific_object, //dbus path
                                                 NULL,
                                                 NULL);*/
        }
      else
        {
          /* TODO: Check if it contains the active connection */
        }

      g_slist_free (connections);
      g_slist_free (rs_connections);
      g_slist_free (dev_connections);
      g_object_unref (rs);
    }
}

static gint
wifi_aps_sort (NMAccessPoint **a,
               NMAccessPoint **b)
{
  NMAccessPoint *ap1 = *a;
  NMAccessPoint *ap2 = *b;

  guint8 strength1 = nm_access_point_get_strength (ap1);
  guint8 strength2 = nm_access_point_get_strength (ap2);

  if (strength1 == strength2)
    return 0;
  if (strength1 > strength1)
    return 1;
  return -1;
}

static void
wifi_populate_accesspoints (DbusmenuMenuitem *parent,
                            NMClient         *client,
                            NMDeviceWifi     *device,
                            gint             *id)
{
  gint              i;
  GPtrArray        *sortedarray;
  const GPtrArray  *apsarray = nm_device_wifi_get_access_points (device);
  NMAccessPoint   **aps;

  /* Creating a new GPtrArray that we can sort */
  sortedarray = g_ptr_array_new ();
  g_ptr_array_set_size (sortedarray, apsarray->len);
  memcpy (sortedarray->pdata, apsarray->pdata, sizeof(NMAccessPoint*) * apsarray->len);
  g_ptr_array_sort (sortedarray, (GCompareFunc)wifi_aps_sort);

  aps = (NMAccessPoint**)(sortedarray->pdata);
  for (i=0; i < sortedarray->len; i++)
    {
      gboolean          is_adhoc   = FALSE;
      gboolean          is_secure  = FALSE;
      NMAccessPoint    *ap = aps[i];
      DbusmenuMenuitem *ap_item = dbusmenu_menuitem_new_with_id ((*id)++);
      char             *utf_ssid;

      utf_ssid = nm_utils_ssid_to_utf8 (nm_access_point_get_ssid (ap));

      if (nm_access_point_get_mode (ap) == NM_802_11_MODE_ADHOC)
        is_adhoc  = TRUE;
      if (nm_access_point_get_flags (ap) == NM_802_11_AP_FLAGS_PRIVACY)
        is_secure = TRUE;

      dbusmenu_menuitem_property_set (ap_item, DBUSMENU_MENUITEM_PROP_LABEL, utf_ssid);
      dbusmenu_menuitem_property_set (ap_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_RADIO);

      dbusmenu_menuitem_property_set (ap_item, "x-wifi-bssid", nm_access_point_get_bssid (ap));
      dbusmenu_menuitem_property_set (ap_item, "type", "x-system-settings");
      dbusmenu_menuitem_property_set (ap_item, "x-tablet-widget", "unity.widgets.systemsettings.tablet.accesspoint");

      dbusmenu_menuitem_property_set_int  (ap_item, "x-wifi-strength",   nm_access_point_get_strength (ap));
      dbusmenu_menuitem_property_set_bool (ap_item, "x-wifi-is-adhoc",   is_adhoc);
      dbusmenu_menuitem_property_set_bool (ap_item, "x-wifi-is-secure",  is_secure);

      dbusmenu_menuitem_child_append  (parent, ap_item);

      g_signal_connect (ap_item, "item-activated",
                        G_CALLBACK (access_point_selected),
                        device);

      g_free (utf_ssid);
    }
  g_free (aps);
}

static void
wireless_toggle_activated (DbusmenuMenuitem *toggle,
                           guint             timestamp,
                           gpointer          data)
{
  NMClient *client = (NMClient*)data;
  gboolean enabled = nm_client_wireless_get_enabled (client);

  nm_client_wireless_set_enabled (client, !enabled);
  dbusmenu_menuitem_property_set_int (toggle, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, !enabled);
}

static void
wifi_device_handler (DbusmenuMenuitem *parent, NMClient *client, NMDevice *device, gint *id)
{
  /* Wifi enable/disable toggle */
  gboolean          wifienabled   = nm_client_wireless_get_enabled (client);
  DbusmenuMenuitem *togglesep     = dbusmenu_menuitem_new_with_id ((*id)++);
  DbusmenuMenuitem *toggle        = dbusmenu_menuitem_new_with_id ((*id)++);

  DbusmenuMenuitem *networksgroup = dbusmenu_menuitem_new_with_id ((*id)++);

  dbusmenu_menuitem_property_set (togglesep, DBUSMENU_MENUITEM_PROP_LABEL, "Turn Wifi On/Off");
  dbusmenu_menuitem_property_set (togglesep, DBUSMENU_MENUITEM_PROP_TYPE,  "x-system-settings");
  dbusmenu_menuitem_property_set (togglesep, "x-tablet-widget",            "unity.widgets.systemsettings.tablet.sectiontitle");

  dbusmenu_menuitem_property_set (toggle, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
  dbusmenu_menuitem_property_set (toggle, DBUSMENU_MENUITEM_PROP_LABEL, "Wifi");
  dbusmenu_menuitem_property_set_int (toggle, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, wifienabled);

  g_signal_connect (toggle, "item-activated",
                    G_CALLBACK (wireless_toggle_activated),
                    client);

  dbusmenu_menuitem_child_append (parent, togglesep);
  dbusmenu_menuitem_child_append (parent, toggle);


  /* Access points */
  if (wifienabled)
    {
      dbusmenu_menuitem_property_set (networksgroup, DBUSMENU_MENUITEM_PROP_LABEL, "Select wireless network");
      dbusmenu_menuitem_property_set (networksgroup, "type",             "x-system-settings");
      dbusmenu_menuitem_property_set_bool (networksgroup, "x-busy", TRUE);
      dbusmenu_menuitem_property_set (networksgroup, "x-group-class",    "accesspoints");
      dbusmenu_menuitem_property_set (networksgroup, "children-display", "inline");
      dbusmenu_menuitem_property_set (networksgroup, "x-tablet-widget",  "unity.widgets.systemsettings.tablet.sectiontitle");
      dbusmenu_menuitem_child_append (parent, networksgroup);
      wifi_populate_accesspoints (networksgroup, client, NM_DEVICE_WIFI (device), id);
    }

  /* TODO: Remove this when toggle is removed */
  /*g_signal_connect (client, "notify::WirelessEnabled",
                    G_CALLBACK (wireless_state_changed)
                    toggle);*/
  /*g_signal_connect (client, "notify::WirelessHardwareEnabled",
                    G_CALLBACK (wireless_state_changed)
                    toggle);*/
}

static void
on_bus (GDBusConnection * connection, const gchar * name, gpointer user_data)
{
  gint               i, id = 0;
  const GPtrArray   *devarray;
  NMClient          *client;
  NMDevice         **devices;
  DbusmenuServer    *server = dbusmenu_server_new("/com/ubuntu/networksettings");
  DbusmenuMenuitem  *root   = dbusmenu_menuitem_new_with_id (id++);

  dbusmenu_server_set_root (server, root);

  client = nm_client_new ();
  devarray = nm_client_get_devices (client);

  devices = (NMDevice**) devarray->pdata;
  for (i=0; i < devarray->len; i++)
    {
      NMDevice *device = devices[i];
      gint type = nm_device_get_device_type (device);

      switch (type)
        {
        case NM_DEVICE_TYPE_WIFI:
          wifi_device_handler (root, client, device, &id);
          break;
        }
    }
  /* TODO: Advance tab (per device?) */
  /* TODO: Airplane mode */
  return;
}

static void
name_lost (GDBusConnection * connection, const gchar * name, gpointer user_data)
{
  g_main_loop_quit (loop);
	return;
}

int
main (int argc, char** argv)
{
  g_type_init ();

  g_bus_own_name(G_BUS_TYPE_SESSION,
                 "com.ubuntu.networksettings",
                 G_BUS_NAME_OWNER_FLAGS_NONE,
                 on_bus,
                 NULL,
                 name_lost,
                 NULL,
                 NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  return 0;
}
