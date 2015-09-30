#include "epg.h"
#include "common.h"
#include "helper.h"

#include <limits.h>

#include <vdr/eit.h>
#include <vdr/epg.h>
#include <vdr/svdrp.h>
#include <vdr/timers.h>


namespace cDBusEpgHelper
{
  enum eMode   { dmmAll, dmmPresent, dmmFollowing, dmmAtTime };

  static const char *_xmlNodeInfo = 
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "       \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\""DBUS_VDR_EPG_INTERFACE"\">\n"
    "    <method name=\"DisableScanner\">\n"
    "      <arg name=\"eitdisabletime\" type=\"i\" direction=\"in\"/>\n"
    "      <arg name=\"replycode\"      type=\"i\" direction=\"out\"/>\n"
    "      <arg name=\"replymessage\"   type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"EnableScanner\">\n"
    "      <arg name=\"replycode\"      type=\"i\" direction=\"out\"/>\n"
    "      <arg name=\"replymessage\"   type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"ClearEPG\">\n"
    "      <arg name=\"channel\"        type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"eitdisabletime\" type=\"i\" direction=\"in\"/>\n"
    "      <arg name=\"replycode\"      type=\"i\" direction=\"out\"/>\n"
    "      <arg name=\"replymessage\"   type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"PutEntry\">\n"
    "      <arg name=\"entryline\"      type=\"as\" direction=\"in\"/>\n"
    "      <arg name=\"replycode\"      type=\"i\"  direction=\"out\"/>\n"
    "      <arg name=\"replymessage\"   type=\"s\"  direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"PutFile\">\n"
    "      <arg name=\"filename\"       type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"replycode\"      type=\"i\" direction=\"out\"/>\n"
    "      <arg name=\"replymessage\"   type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Now\">\n"
    "      <arg name=\"channel\"        type=\"s\"  direction=\"in\"/>\n"
    "      <arg name=\"replycode\"      type=\"i\"  direction=\"out\"/>\n"
    "      <arg name=\"replymessage\"   type=\"s\"  direction=\"out\"/>\n"
    "      <arg name=\"event_list\"     type=\"aa(sv)\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Next\">\n"
    "      <arg name=\"channel\"        type=\"s\"  direction=\"in\"/>\n"
    "      <arg name=\"replycode\"      type=\"i\"  direction=\"out\"/>\n"
    "      <arg name=\"replymessage\"   type=\"s\"  direction=\"out\"/>\n"
    "      <arg name=\"event_list\"     type=\"aa(sv)\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"At\">\n"
    "      <arg name=\"channel\"        type=\"s\"  direction=\"in\"/>\n"
    "      <arg name=\"time\"           type=\"t\"  direction=\"in\"/>\n"
    "      <arg name=\"replycode\"      type=\"i\"  direction=\"out\"/>\n"
    "      <arg name=\"replymessage\"   type=\"s\"  direction=\"out\"/>\n"
    "      <arg name=\"event_list\"     type=\"aa(sv)\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "</node>\n";

  static void DisableScanner(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
  #if APIVERSNUM >= 10711
    int eitDisableTime = 3600; // one hour should be ok as a default
    int s = 0;
    g_variant_get(Parameters, "(i)", &s);
    if (s > 0)
       eitDisableTime = s;
    cString replyMessage = cString::sprintf("EIT scanner disabled for %d sseconds", eitDisableTime);
    isyslog("dbus2vdr: %s.DisableScanner: %s", DBUS_VDR_EPG_INTERFACE, *replyMessage);
    cEitFilter::SetDisableUntil(time(NULL) + eitDisableTime);
    cDBusHelper::SendReply(Invocation, 250, *replyMessage);
  #else
    esyslog("dbus2vdr: %s.DisableScanner: you need at least vdr 1.7.11", DBUS_VDR_EPG_INTERFACE);
    cDBusHelper::SendReply(Invocation, 550, "you need at least vdr 1.7.11");
  #endif
  };

  static void EnableScanner(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
  #if APIVERSNUM >= 10711
    isyslog("dbus2vdr: %s.EnableScanner: EIT scanner enabled", DBUS_VDR_EPG_INTERFACE);
    cEitFilter::SetDisableUntil(0);
    cDBusHelper::SendReply(Invocation, 250, "EIT scanner enabled");
  #else
    esyslog("dbus2vdr: %s.EnableScanner: you need at least vdr 1.7.11", DBUS_VDR_EPG_INTERFACE);
    cDBusHelper::SendReply(Invocation, 550, "you need at least vdr 1.7.11");
  #endif
  };

  static void ClearEPG(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    const char *channel = NULL;
    int timeout = 0;
    g_variant_get(Parameters, "(&si)", &channel, &timeout);
    int eitDisableTime = 10; // seconds until EIT processing is enabled again after a CLRE command
    if (timeout > 0) {
       eitDisableTime = timeout;
       isyslog("dbus2vdr: %s.ClearEPG: using %d seconds as EIT disable timeout", DBUS_VDR_EPG_INTERFACE, eitDisableTime);
       }

    if (channel) {
       tChannelID ChannelID = tChannelID::InvalidID;
       if (isnumber(channel)) {
          int o = strtol(channel, NULL, 10);
#if VDRVERSNUM > 20300
          LOCK_CHANNELS_READ;
          const cChannels *channels = Channels;
#else
          cChannels *channels = &Channels;
#endif
          if (o >= 1 && o <= channels->MaxNumber())
             ChannelID = channels->GetByNumber(o)->GetChannelID();
          }
       else {
          ChannelID = tChannelID::FromString(channel);
          if (ChannelID == tChannelID::InvalidID) {
             const cChannels *channels = NULL;
#if VDRVERSNUM > 20300
             LOCK_CHANNELS_READ;
             channels = Channels;
#else
             channels = &Channels;
#endif
             for (const cChannel *Channel = channels->First(); Channel; Channel = channels->Next(Channel)) {
                 if (!Channel->GroupSep()) {
                    if (strcasecmp(Channel->Name(), channel) == 0) {
                       ChannelID = Channel->GetChannelID();
                       break;
                       }
                    }
                 }
             }
          }
       if (!(ChannelID == tChannelID::InvalidID)) {
#if VDRVERSNUM > 20300
          cStateKey StateKey;
          cSchedules *s = cSchedules::GetSchedulesWrite(StateKey, 1000);
#else
          cSchedulesLock SchedulesLock(true, 1000);
          cSchedules *s = (cSchedules *)cSchedules::Schedules(SchedulesLock);
#endif
          if (s) {
             cSchedule *Schedule = NULL;
             ChannelID.ClrRid();
             for (cSchedule *p = s->First(); p; p = s->Next(p)) {
                 if (p->ChannelID() == ChannelID) {
                    Schedule = p;
                    break;
                    }
                 }
             if (Schedule) {
                Schedule->Cleanup(INT_MAX);
                #if APIVERSNUM >= 10711
                cEitFilter::SetDisableUntil(time(NULL) + eitDisableTime);
                #endif
                cString replyMessage = cString::sprintf("EPG data of channel \"%s\" cleared", channel);
                cDBusHelper::SendReply(Invocation, 250, *replyMessage);
                }
             else {
                cString replyMessage = cString::sprintf("No EPG data found for channel \"%s\"", channel);
                cDBusHelper::SendReply(Invocation, 550, *replyMessage);
                }
#if VDRVERSNUM > 20300
             StateKey.Remove();
#endif
             }
          else
             cDBusHelper::SendReply(Invocation, 451, "Can't get EPG data");
          }
       else {
          cString replyMessage = cString::sprintf("Undefined channel \"%s\"", channel);
          cDBusHelper::SendReply(Invocation, 501, *replyMessage);
          }
       }
    else {
#if VDRVERSNUM > 20300
       LOCK_TIMERS_WRITE;
       LOCK_SCHEDULES_WRITE;
       for (cTimer *Timer = Timers->First(); Timer; Timer = Timers->Next(Timer))
           Timer->SetEvent(NULL); // processing all timers here (local *and* remote)
       for (cSchedule *Schedule = Schedules->First(); Schedule; Schedule = Schedules->Next(Schedule))
           Schedule->Cleanup(INT_MAX);
#else
       cSchedules::ClearAll();
#endif
       cEitFilter::SetDisableUntil(time(NULL) + eitDisableTime);
       cDBusHelper::SendReply(Invocation, 250, "EPG data cleared");
       }
  };

#if VDRVERSNUM > 20300
// copied from svdrp.c, maybe it will be available in future versions?
// --- cPUTEhandler ----------------------------------------------------------

class cPUTEhandler {
private:
  FILE *f;
  int status;
  const char *message;
public:
  cPUTEhandler(void);
  ~cPUTEhandler();
  bool Process(const char *s);
  int Status(void) { return status; }
  const char *Message(void) { return message; }
  };

cPUTEhandler::cPUTEhandler(void)
{
  if ((f = tmpfile()) != NULL) {
     status = 354;
     message = "Enter EPG data, end with \".\" on a line by itself";
     }
  else {
     LOG_ERROR;
     status = 554;
     message = "Error while opening temporary file";
     }
}

cPUTEhandler::~cPUTEhandler()
{
  if (f)
     fclose(f);
}

bool cPUTEhandler::Process(const char *s)
{
  if (f) {
     if (strcmp(s, ".") != 0) {
        fputs(s, f);
        fputc('\n', f);
        return true;
        }
     else {
        rewind(f);
        if (cSchedules::Read(f)) {
           cSchedules::Cleanup(true);
           status = 250;
           message = "EPG data processed";
           }
        else {
           status = 451;
           message = "Error while processing EPG data";
           }
        fclose(f);
        f = NULL;
        }
     }
  return false;
}
#endif

  static void PutEntry(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    gsize len = 0;
    GVariant *array = g_variant_get_child_value(Parameters, 0);
    const gchar **line = g_variant_get_strv(array, &len);
    if (len < 1) {
       g_free(line);
       cDBusHelper::SendReply(Invocation, 501, "at least one element must be given");
       return;
       }

    cPUTEhandler *handler = new cPUTEhandler();
    if (handler->Status() == 354) {
       for (gsize i = 0; i < len; i++) {
           d4syslog("dbus2vdr: %s.PutEntry: item = %s", DBUS_VDR_EPG_INTERFACE, line[i]);
           d4syslog("dbus2vdr: %s.PutEntry: status = %d, message = %s", DBUS_VDR_EPG_INTERFACE, handler->Status(), handler->Message());
           if (!handler->Process(line[i]))
              break;
           }
       d4syslog("dbus2vdr: %s.PutEntry: status = %d, message = %s", DBUS_VDR_EPG_INTERFACE, handler->Status(), handler->Message());
       if (handler->Status() == 354)
          handler->Process(".");
       }
    cDBusHelper::SendReply(Invocation, handler->Status(), handler->Message());
    delete handler;
    g_free(line);
  };

  static void PutFile(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    const char *filename = NULL;
    g_variant_get(Parameters, "(&s)", &filename);

    if ((filename != NULL) && (*filename != 0)) {
       FILE *f = fopen(filename, "r");
       if (f) {
          cString message = cString::sprintf("start reading epg data from %s", filename);
          cDBusHelper::SendReply(Invocation, 0, *message);
          if (cSchedules::Read(f))
             cSchedules::Cleanup(true);
          fclose(f);
          }
       else {
          esyslog("dbus2vdr: %s.PutFile: error opening %s", DBUS_VDR_EPG_INTERFACE, filename);
          cString message = cString::sprintf("error opening %s", filename);
          cDBusHelper::SendReply(Invocation, -2, *message);
          }
       }
    else
       cDBusHelper::SendReply(Invocation, -1, "no filename");
  };

  static void sAddEvent(GVariantBuilder *Array, const cEvent &Event)
  {
    const char *c;
    guint32 tu32;
    guint64 tu64;
    int ti;
    gboolean tb;

    GVariantBuilder *arr = g_variant_builder_new(G_VARIANT_TYPE("a(sv)"));

    cString cid = Event.ChannelID().ToString();
    c = *cid;
    cDBusHelper::AddKeyValue(arr, "ChannelID", "s", (void**)&c);

    tu32 = Event.EventID();
    cDBusHelper::AddKeyValue(arr, "EventID", "u", (void**)&tu32);

    c = Event.Title();
    if (c != NULL)
       cDBusHelper::AddKeyValue(arr, "Title", "s", (void**)&c);

    c = Event.ShortText();
    if (c != NULL)
       cDBusHelper::AddKeyValue(arr, "ShortText", "s", (void**)&c);

    c = Event.Description();
    if (c != NULL)
       cDBusHelper::AddKeyValue(arr, "Description", "s", (void**)&c);

    tu64 = Event.StartTime();
    cDBusHelper::AddKeyValue(arr, "StartTime", "t", (void**)&tu64);

    tu64 = Event.EndTime();
    cDBusHelper::AddKeyValue(arr, "EndTime", "t", (void**)&tu64);

    tu64 = Event.Duration();
    cDBusHelper::AddKeyValue(arr, "Duration", "t", (void**)&tu64);

    tu64 = Event.Vps();
    cDBusHelper::AddKeyValue(arr, "Vps", "t", (void**)&tu64);

    ti = Event.RunningStatus();
    cDBusHelper::AddKeyValue(arr, "RunningStatus", "i", (void**)&ti);

  #if VDRVERSNUM >= 10711
    ti = Event.ParentalRating();
    cDBusHelper::AddKeyValue(arr, "ParentalRating", "i", (void**)&ti);

    tb = Event.HasTimer();
    cDBusHelper::AddKeyValue(arr, "HasTimer", "b", (void**)&tb);

    for (int i = 0; i < MaxEventContents; i++) {
        tu32 = Event.Contents(i);
        if (tu32 != 0) {
           cDBusHelper::AddKeyValue(arr, *cString::sprintf("ContentID[%d]", i), "u", (void**)&tu32);
           c = cEvent::ContentToString(tu32);
           cDBusHelper::AddKeyValue(arr, *cString::sprintf("Content[%d]", i), "s", (void**)&c);
           }
        }
  #endif

    g_variant_builder_add_value(Array, g_variant_builder_end(arr));
    g_variant_builder_unref(arr);
  }

#if VDRVERSNUM > 20300
  static bool sGetChannel(GVariant *Arg, const char **Input, const cChannels* Channels, const cChannel **Channel)
#else
  static bool sGetChannel(GVariant *Arg, const char **Input, cChannels* Channels, const cChannel **Channel)
#endif
  {
    *Channel = NULL;
    *Input = NULL;
    if (g_variant_is_of_type(Arg, G_VARIANT_TYPE_STRING)) {
       g_variant_get(Arg, "&s", Input);
       if (**Input == 0)
          return true;
       if (isnumber(*Input))
          *Channel = Channels->GetByNumber(strtol(*Input, NULL, 10));
       else
          *Channel = Channels->GetByChannelID(tChannelID::FromString(*Input));
       if (*Channel == NULL)
          return false;
       }
    return true;
  }

  static void sReturnError(GDBusMethodInvocation *Invocation, int  ReplyCode, const char *ReplyMessage)
  {
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("(isaa(sv))"));
    g_variant_builder_add(builder, "i", ReplyCode);
    g_variant_builder_add(builder, "s", ReplyMessage);
    GVariantBuilder *array = g_variant_builder_new(G_VARIANT_TYPE("aa(sv)"));
    GVariantBuilder *innerArray = g_variant_builder_new(G_VARIANT_TYPE("a(sv)"));
    g_variant_builder_add_value(array, g_variant_builder_end(innerArray));
    g_variant_builder_add_value(builder, g_variant_builder_end(array));
    g_dbus_method_invocation_return_value(Invocation, g_variant_builder_end(builder));
    g_variant_builder_unref(innerArray);
    g_variant_builder_unref(array);
    g_variant_builder_unref(builder);
  };
  
  static void sGetEntries(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation, eMode mode)
  {
#if VDRVERSNUM > 20300
    LOCK_CHANNELS_READ;
    const cChannels *channels = Channels;
#else
    cChannels *channels = &Channels;
#endif
    const cChannel *channel = NULL;
    guint64 atTime = 0;
    GVariant *first = g_variant_get_child_value(Parameters, 0);
    
    const char *c = NULL;
    if (!sGetChannel(first, &c, channels, &channel)) {
       cString reply = cString::sprintf("channel \"%s\" not defined", c);
       esyslog("dbus2vdr: %s.GetEntries: %s", DBUS_VDR_EPG_INTERFACE, *reply);
       sReturnError(Invocation, 501, *reply);
       g_variant_unref(first);
       return;
       }
    if (mode == dmmAtTime) {
       GVariant *second = g_variant_get_child_value(Parameters, 1);
       g_variant_get(second, "t", &atTime);
       g_variant_unref(second);
       if (atTime == 0) {
          cString reply = cString::sprintf("missing time");
          esyslog("dbus2vdr: %s.GetEntries: %s", DBUS_VDR_EPG_INTERFACE, *reply);
          sReturnError(Invocation, 501, *reply);
          g_variant_unref(first);
          return;
          }
       }
    g_variant_unref(first);

    const cSchedules *scheds = NULL;
#if VDRVERSNUM > 20300
    cStateKey StateKey;
    scheds = cSchedules::GetSchedulesRead(StateKey, 1000);
#else
    cSchedulesLock sl(false, 1000);
    if (!sl.Locked()) {
       sReturnError(Invocation, 550, "got no lock on schedules");
       return;
       }

    scheds = cSchedules::Schedules(sl);
#endif
    if (scheds == NULL) {
       sReturnError(Invocation, 550, "got no schedules");
       return;
       }

    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("(isaa(sv))"));
    g_variant_builder_add(builder, "i", 250);
    g_variant_builder_add(builder, "s", "");
    GVariantBuilder *array = g_variant_builder_new(G_VARIANT_TYPE("aa(sv)"));

    bool next = false;
    if (channel == NULL) {
       channel = channels->First();
       next = true;
       }

    while (channel) {
          const cSchedule *s = scheds->GetSchedule(channel, false);
          if (s != NULL) {
             const cEvent *e = NULL;
             switch (mode) {
               case dmmPresent:
                 e = s->GetPresentEvent();
                 break;
               case dmmFollowing:
                 e = s->GetFollowingEvent();
                 break;
               case dmmAtTime:
                 e = s->GetEventAround(atTime);
                 break;
               default:
                 e = NULL;
                 break;
               }
             if (e != NULL)
                sAddEvent(array, *e);
             }
          if (next)
             channel = channels->Next(channel);
          else
             break;
          }

#if VDRVERSNUM > 20300
    StateKey.Remove();
#endif
    g_variant_builder_add_value(builder, g_variant_builder_end(array));
    g_dbus_method_invocation_return_value(Invocation, g_variant_builder_end(builder));
    g_variant_builder_unref(array);
    g_variant_builder_unref(builder);
  };

  static void Now(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    sGetEntries(Object, Parameters, Invocation, dmmPresent);
  };

  static void Next(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    sGetEntries(Object, Parameters, Invocation, dmmFollowing);
  };

  static void At(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    sGetEntries(Object, Parameters, Invocation, dmmAtTime);
  };
}

cDBusEpg::cDBusEpg(void)
:cDBusObject("/EPG", cDBusEpgHelper::_xmlNodeInfo)
{
  AddMethod("DisableScanner", cDBusEpgHelper::DisableScanner);
  AddMethod("EnableScanner", cDBusEpgHelper::EnableScanner);
  AddMethod("ClearEPG", cDBusEpgHelper::ClearEPG);
  AddMethod("PutEntry", cDBusEpgHelper::PutEntry);
  AddMethod("PutFile", cDBusEpgHelper::PutFile);
  AddMethod("Now", cDBusEpgHelper::Now);
  AddMethod("Next", cDBusEpgHelper::Next);
  AddMethod("At", cDBusEpgHelper::At);
}

cDBusEpg::~cDBusEpg(void)
{
}
