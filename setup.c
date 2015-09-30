#include "setup.h"
#include "common.h"
#include "helper.h"

#include <limits.h>

#include <vdr/config.h>
#include <vdr/device.h>
#include <vdr/epg.h>
#include <vdr/plugin.h>
#include <vdr/recording.h>
#include <vdr/themes.h>


class cDBusSetupHelper
{
public:
  static const char *_xmlNodeInfo;

private:
  static cSetupLine *FindSetupLine(cConfig<cSetupLine>& config, const char *name, const char *plugin)
  {
    if (name != NULL) {
       cSetupLine *sl = config.First();
       while (sl != NULL) {
             if (((sl->Plugin() == NULL) == (plugin == NULL))
              && ((plugin == NULL) || (strcasecmp(sl->Plugin(), plugin) == 0))
              && (strcasecmp(sl->Name(), name) == 0))
                return sl;
             sl = config.Next(sl);
             }
       }
    return NULL;
  };

  class cSetupBinding : public cListObject
  {
  private:
    cSetupBinding() {}

  public:
    enum eType { dstString, dstInt32, dstTimeT };

    const char *Name;
    eType Type;
    void *Value;
    // String
    int StrMaxLength;
    // Int32
    int Int32MinValue;
    int Int32MaxValue;

    virtual int Compare(const cListObject &ListObject) const
    {
      const cSetupBinding *sb = (cSetupBinding*)&ListObject;
      return strcasecmp(Name, sb->Name);
    }
    
    static cSetupBinding *NewString(void* value, const char *name, int maxLength)
    {
      cSetupBinding *b = new cSetupBinding();
      b->Name = name;
      b->Type = dstString;
      b->Value = value;
      b->StrMaxLength = maxLength;
      return b;
    }

    static cSetupBinding *NewInt32(void* value, const char *name, int minValue = 0, int maxValue = INT_MAX)
    {
      cSetupBinding *b = new cSetupBinding();
      b->Name = name;
      b->Type = dstInt32;
      b->Value = value;
      b->Int32MinValue = minValue;
      b->Int32MaxValue = maxValue;
      return b;
    }

    static cSetupBinding *NewTimeT(void* value, const char *name)
    {
      cSetupBinding *b = new cSetupBinding();
      b->Name = name;
      b->Type = dstTimeT;
      b->Value = value;
      return b;
    }

    static const cSetupBinding* Find(const cList<cSetupBinding>& bindings, const char *name)
    {
      if (name == NULL)
         return NULL;
      const cSetupBinding *sb = bindings.First();
      while ((sb != NULL) && (strcasecmp(sb->Name, name) != 0))
            sb = bindings.Next(sb);
      return sb;
    }
  };
  static cList<cSetupBinding> _bindings;

public:
  static void InitBindings(void)
  {
    if (_bindings.Count() == 0) {
       _bindings.Add(cSetupBinding::NewString(Setup.OSDLanguage, "OSDLanguage", I18N_MAX_LOCALE_LEN));
       _bindings.Add(cSetupBinding::NewString(Setup.OSDSkin, "OSDSkin", MaxSkinName));
       _bindings.Add(cSetupBinding::NewString(Setup.OSDTheme, "OSDTheme", MaxThemeName));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.PrimaryDVB, "PrimaryDVB", 1, cDevice::NumDevices()));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.ShowInfoOnChSwitch, "ShowInfoOnChSwitch", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.TimeoutRequChInfo, "TimeoutRequChInfo", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MenuScrollPage, "MenuScrollPage", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MenuScrollWrap, "MenuScrollWrap", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MenuKeyCloses, "MenuKeyCloses", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MarkInstantRecord, "MarkInstantRecord", 0, 1));
       _bindings.Add(cSetupBinding::NewString(Setup.NameInstantRecord, "NameInstantRecord", MaxFileName));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.InstantRecordTime, "InstantRecordTime", 1, 24 * 60 -1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.LnbSLOF, "LnbSLOF"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.LnbFrequLo, "LnbFrequLo"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.LnbFrequHi, "LnbFrequHi"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.DiSEqC, "DiSEqC", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.SetSystemTime, "SetSystemTime", 0, 1));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.TimeSource, "TimeSource"));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.TimeTransponder, "TimeTransponder"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MarginStart, "MarginStart"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MarginStop, "MarginStop"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.DisplaySubtitles, "DisplaySubtitles", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.SubtitleOffset, "SubtitleOffset", -100, 100));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.SubtitleFgTransparency, "SubtitleFgTransparency", 0, 9));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.SubtitleBgTransparency, "SubtitleBgTransparency", 0, 10));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.EPGScanTimeout, "EPGScanTimeout"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.EPGBugfixLevel, "EPGBugfixLevel", 0, MAXEPGBUGFIXLEVEL));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.EPGLinger, "EPGLinger", 0));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.SVDRPTimeout, "SVDRPTimeout"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.ZapTimeout, "ZapTimeout"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.ChannelEntryTimeout, "ChannelEntryTimeout", 0));
#if VDRVERSNUM < 10726
       _bindings.Add(cSetupBinding::NewInt32(&Setup.PrimaryLimit, "PrimaryLimit", 0, MAXPRIORITY));
#endif
       _bindings.Add(cSetupBinding::NewInt32(&Setup.DefaultPriority, "DefaultPriority", 0, MAXPRIORITY));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.DefaultLifetime, "DefaultLifetime", 0, MAXLIFETIME));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.PausePriority, "PausePriority", 0, MAXPRIORITY));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.PauseLifetime, "PauseLifetime", 0, MAXLIFETIME));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.PauseKeyHandling, "PauseKeyHandling"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.UseSubtitle, "UseSubtitle", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.UseVps, "UseVps", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.VpsMargin, "VpsMargin", 0));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.RecordingDirs, "RecordingDirs", 0, 1));
#if VDRVERSNUM >= 10712
       _bindings.Add(cSetupBinding::NewInt32(&Setup.FoldersInTimerMenu, "FoldersInTimerMenu", 0, 1));
#endif
#if VDRVERSNUM >= 10715
       _bindings.Add(cSetupBinding::NewInt32(&Setup.NumberKeysForChars, "NumberKeysForChars", 0, 1));
#endif
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.VideoDisplayFormat, "VideoDisplayFormat"));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.VideoFormat, "VideoFormat"));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.UpdateChannels, "UpdateChannels"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.UseDolbyDigital, "UseDolbyDigital", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.ChannelInfoPos, "ChannelInfoPos", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.ChannelInfoTime, "ChannelInfoTime", 1, 60));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.OSDLeft, "OSDLeft"));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.OSDTop, "OSDTop"));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.OSDWidth, "OSDWidth"));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.OSDHeight, "OSDHeight"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.OSDMessageTime, "OSDMessageTime", 1, 60));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.UseSmallFont, "UseSmallFont"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.AntiAlias, "AntiAlias", 0, 1));
       _bindings.Add(cSetupBinding::NewString(Setup.FontOsd, "FontOsd", MAXFONTNAME));
       _bindings.Add(cSetupBinding::NewString(Setup.FontSml, "FontSml", MAXFONTNAME));
       _bindings.Add(cSetupBinding::NewString(Setup.FontFix, "FontFix", MAXFONTNAME));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.FontOsdSize, "FontOsdSize"));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.FontSmlSize, "FontSmlSize"));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.FontFixSize, "FontFixSize"));
#if VDRVERSNUM >= 10704
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MaxVideoFileSize, "MaxVideoFileSize", MINVIDEOFILESIZE, MAXVIDEOFILESIZETS));
#endif
       _bindings.Add(cSetupBinding::NewInt32(&Setup.SplitEditedFiles, "SplitEditedFiles", 0, 1));
       //_bindings.Add(cSetupBinding::NewInt32(&Setup.DelTimeshiftRec, "DelTimeshiftRec"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MinEventTimeout, "MinEventTimeout"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MinUserInactivity, "MinUserInactivity"));
       _bindings.Add(cSetupBinding::NewTimeT(&Setup.NextWakeupTime, "NextWakeupTime"));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.MultiSpeedMode, "MultiSpeedMode", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.ShowReplayMode, "ShowReplayMode", 0, 1));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.ResumeID, "ResumeID", 0, 99));
       _bindings.Add(cSetupBinding::NewInt32(&Setup.InitialVolume, "InitialVolume", -1, 255));
#if VDRVERSNUM >= 10712
       _bindings.Add(cSetupBinding::NewInt32(&Setup.ChannelsWrap, "ChannelsWrap", 0, 1));
#endif
       _bindings.Add(cSetupBinding::NewInt32(&Setup.EmergencyExit, "EmergencyExit", 0, 1));

       _bindings.Sort();
       }
  }

public:
  static void List(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    // a(sv)
    GVariantBuilder *array = g_variant_builder_new(G_VARIANT_TYPE("a(sv)"));
    GVariantBuilder *element = NULL;
    GVariantBuilder *variant = NULL;
    GVariantBuilder *struc = NULL;

    for (cSetupBinding *b = _bindings.First(); b; b = _bindings.Next(b)) {
        element = g_variant_builder_new(G_VARIANT_TYPE("(sv)"));
        g_variant_builder_add_value(element, g_variant_new_string(b->Name));
        
        switch (b->Type) {
         case cSetupBinding::dstString:
          {
           struc = g_variant_builder_new(G_VARIANT_TYPE("(si)"));
           g_variant_builder_add_value(struc, g_variant_new_string((const char*)b->Value));
           g_variant_builder_add_value(struc, g_variant_new_int32(b->StrMaxLength));

           variant = g_variant_builder_new(G_VARIANT_TYPE("v"));
           g_variant_builder_add_value(variant, g_variant_builder_end(struc));
           g_variant_builder_add_value(element, g_variant_builder_end(variant));
           g_variant_builder_unref(variant);
           g_variant_builder_unref(struc);
           break;
          }
         case cSetupBinding::dstInt32:
          {
           struc = g_variant_builder_new(G_VARIANT_TYPE("(iii)"));
           g_variant_builder_add_value(struc, g_variant_new_int32(*(int*)(b->Value)));
           g_variant_builder_add_value(struc, g_variant_new_int32(b->Int32MinValue));
           g_variant_builder_add_value(struc, g_variant_new_int32(b->Int32MaxValue));

           variant = g_variant_builder_new(G_VARIANT_TYPE("v"));
           g_variant_builder_add_value(variant, g_variant_builder_end(struc));
           g_variant_builder_add_value(element, g_variant_builder_end(variant));
           g_variant_builder_unref(variant);
           g_variant_builder_unref(struc);
           break;
          }
         case cSetupBinding::dstTimeT:
          {
           variant = g_variant_builder_new(G_VARIANT_TYPE("v"));
           g_variant_builder_add_value(variant, g_variant_new_int64(*(time_t*)(b->Value)));
           g_variant_builder_add_value(element, g_variant_builder_end(variant));
           g_variant_builder_unref(variant);
           break;
          }
         }
         g_variant_builder_add_value(array, g_variant_builder_end(element));
         g_variant_builder_unref(element);
        }
    int nolimit = -1;
    cString name;
    for (cSetupLine *line = Setup.First(); line; line = Setup.Next(line)) {
        // output all plugins and unknown settings
        if ((line->Plugin() == NULL) && (cSetupBinding::Find(_bindings, line->Name()) != NULL))
           continue;
        element = g_variant_builder_new(G_VARIANT_TYPE("(sv)"));
        if (line->Plugin() == NULL)
           name = cString::sprintf("%s", line->Name());
        else
           name = cString::sprintf("%s.%s", line->Plugin(), line->Name());
        g_variant_builder_add_value(element, g_variant_new_string(*name));
        struc = g_variant_builder_new(G_VARIANT_TYPE("(si)"));
        g_variant_builder_add_value(struc, g_variant_new_string(line->Value()));
        g_variant_builder_add_value(struc, g_variant_new_int32(nolimit));
        variant = g_variant_builder_new(G_VARIANT_TYPE("v"));
        g_variant_builder_add_value(variant, g_variant_builder_end(struc));
        g_variant_builder_add_value(element, g_variant_builder_end(variant));
        g_variant_builder_add_value(array, g_variant_builder_end(element));
        g_variant_builder_unref(variant);
        g_variant_builder_unref(struc);
        g_variant_builder_unref(element);
        }

    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("(a(sv))"));
    g_variant_builder_add_value(builder, g_variant_builder_end(array));
    g_dbus_method_invocation_return_value(Invocation, g_variant_builder_end(builder));
    g_variant_builder_unref(array);
    g_variant_builder_unref(builder);
  };

  static void Get(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    const gchar *name = NULL;
    g_variant_get(Parameters, "(&s)", &name);

    gint32 replyCode = 501;
    cString replyMessage = "missing arguments";
    if (name != NULL) {
       const cSetupBinding *b = cSetupBinding::Find(_bindings, name);
       if (b == NULL) {
          char *point = (char*)strchr(name, '.');
          char *plugin = NULL;
          const char *key = NULL;
          char *dummy = NULL;
          if (point == NULL) { // this is an unknown setting
             key = name;
             isyslog("dbus2vdr: %s.Get: looking for %s", DBUS_VDR_SETUP_INTERFACE, key);
             }
          else { // this is a plugin setting
             dummy = strdup(name);
             plugin = compactspace(dummy);
             point = strchr(plugin, '.');
             *point = 0;
             key = point + 1;
             isyslog("dbus2vdr: %s.Get: looking for %s.%s", DBUS_VDR_SETUP_INTERFACE, plugin, key);
             }
          const char *value = NULL;
          for (cSetupLine *line = Setup.First(); line; line = Setup.Next(line)) {
              if ((line->Plugin() == NULL) != (plugin == NULL))
                 continue;
              if ((plugin != NULL) && (strcasecmp(plugin, line->Plugin()) != 0))
                 continue;
              if (strcasecmp(key, line->Name()) != 0)
                 continue;
              value = line->Value();
              break;
              }
          if (dummy != NULL)
             free(dummy);
          if (value == NULL) {
             replyMessage = cString::sprintf("%s not found in setup.conf", name);
             esyslog("dbus2vdr: %s.Get: %s not found in setup.conf", DBUS_VDR_SETUP_INTERFACE, name);
             GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("(vis)"));
             GVariantBuilder *variant = g_variant_builder_new(G_VARIANT_TYPE("v"));
             g_variant_builder_add_value(variant, g_variant_new_string(""));
             g_variant_builder_add_value(builder, g_variant_builder_end(variant));
             g_variant_builder_add(builder, "i", replyCode);
             g_variant_builder_add(builder, "s", *replyMessage);
             g_dbus_method_invocation_return_value(Invocation, g_variant_builder_end(builder));
             g_variant_builder_unref(builder);
             g_variant_builder_unref(variant);
             return;
             }
          replyCode = 900;
          replyMessage = cString::sprintf("getting %s", name);

          GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("(vis)"));
          GVariantBuilder *variant = g_variant_builder_new(G_VARIANT_TYPE("v"));
          g_variant_builder_add_value(variant, g_variant_new_string(value));
          g_variant_builder_add_value(builder, g_variant_builder_end(variant));
          g_variant_builder_add(builder, "i", replyCode);
          g_variant_builder_add(builder, "s", *replyMessage);
          g_dbus_method_invocation_return_value(Invocation, g_variant_builder_end(builder));
          g_variant_builder_unref(builder);
          g_variant_builder_unref(variant);
          return;
          }

       replyCode = 900;
       replyMessage = cString::sprintf("getting %s", name);

       GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("(vis)"));
       GVariantBuilder *variant = g_variant_builder_new(G_VARIANT_TYPE("v"));
       switch (b->Type) {
         case cSetupBinding::dstString:
          {
           g_variant_builder_add(variant, "s", (const char*)b->Value);
           break;
          }
         case cSetupBinding::dstInt32:
          {
           g_variant_builder_add(variant, "i", *(int*)(b->Value));
           break;
          }
         case cSetupBinding::dstTimeT:
          {
           g_variant_builder_add(variant, "x", *(time_t*)(b->Value));
           break;
          }
         }

       g_variant_builder_add_value(builder, g_variant_builder_end(variant));
       g_variant_builder_add(builder, "i", replyCode);
       g_variant_builder_add(builder, "s", *replyMessage);
       g_dbus_method_invocation_return_value(Invocation, g_variant_builder_end(builder));
       g_variant_builder_unref(builder);
       g_variant_builder_unref(variant);
       return;
       }

    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("(vis)"));
    GVariantBuilder *variant = g_variant_builder_new(G_VARIANT_TYPE("v"));
    g_variant_builder_add_value(variant, g_variant_new_string(""));
    g_variant_builder_add_value(builder, g_variant_builder_end(variant));
    g_variant_builder_add(builder, "i", replyCode);
    g_variant_builder_add(builder, "s", *replyMessage);
    g_dbus_method_invocation_return_value(Invocation, g_variant_builder_end(builder));
    g_variant_builder_unref(builder);
    g_variant_builder_unref(variant);
  };

  static void Set(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    const gchar *name = NULL;
    GVariant *nameChild = g_variant_get_child_value(Parameters, 0);
    g_variant_get(nameChild, "&s", &name);

    GVariant *variant = g_variant_get_child_value(Parameters, 1);
    GVariant *child = g_variant_get_child_value(variant, 0);

    gint32 replyCode = 501;
    cString replyMessage = "missing arguments";
    if (name != NULL) {
       const cSetupBinding *b = cSetupBinding::Find(_bindings, name);
       if (b == NULL) {
          const char *value = NULL;
          if (!g_variant_is_of_type(child, G_VARIANT_TYPE_STRING))
             replyMessage = cString::sprintf("argument for %s is not a string", name);
          else {
             g_variant_get(child, "&s", &value);
             cPlugin *plugin = NULL;
             char *dummy = NULL;
             char *pluginName = NULL;
             const char *key = NULL;
             char *point = (char*)strchr(name, '.');
             if (point == NULL) {
                key = name;
                isyslog("dbus2vdr: %s.Set: looking for %s", DBUS_VDR_SETUP_INTERFACE, key);
                }
             else { // this is a plugin setting
                dummy = strdup(name);
                pluginName = compactspace(dummy);
                point = strchr(pluginName, '.');
                *point = 0;
                key = point + 1;
                isyslog("dbus2vdr: %s.Set: looking for %s.%s", DBUS_VDR_SETUP_INTERFACE, pluginName, key);
                plugin = cPluginManager::GetPlugin(pluginName);
                if (plugin == NULL)
                   isyslog("dbus2vdr: %s.Set: plugin %s not loaded, try to set it directly", DBUS_VDR_SETUP_INTERFACE, pluginName);
                }
             if (plugin == NULL) {
                // save vdr-setup, load it in own Setup-container
                // adjust value, save as setup.conf, reload vdr-setup
                cSetupLine *line = FindSetupLine(Setup, key, pluginName);
                if (line != NULL) {
                    isyslog("dbus2vdr: %s.Set: found %s%s%s = %s", DBUS_VDR_SETUP_INTERFACE, (line->Plugin() == NULL) ? "" : line->Plugin(), (line->Plugin() == NULL) ? "" : ".", line->Name(), line->Value());
                    Setup.Save();
                    char *filename = strdup(Setup.FileName());
                    cConfig<cSetupLine> tmpSetup;
                    tmpSetup.Load(filename, true);
                    line = FindSetupLine(tmpSetup, key, pluginName);
                    if (line != NULL) {
                       cSetupLine *newLine = new cSetupLine(key, value, pluginName);
                       tmpSetup.Add(newLine, line);
                       tmpSetup.Del(line);
                       tmpSetup.Save();
                       Setup.Load(filename);
                       }
                    free(filename);
                    }
                else {
                   isyslog("dbus2vdr: %s.Set: add new line to setup.conf: %s%s%s = %s", DBUS_VDR_SETUP_INTERFACE, (pluginName == NULL) ? "" : pluginName, (pluginName == NULL) ? "" : ".", key, value);
                   Setup.Add(new cSetupLine(key, value, pluginName));
                   Setup.Save();
                   }
                replyCode = 900;
                replyMessage = cString::sprintf("storing %s%s%s = %s", (pluginName == NULL) ? "" : pluginName, (pluginName == NULL) ? "" : ".", key, value);
                }
             else {
                if (!plugin->SetupParse(key, value)) {
                   replyMessage = cString::sprintf("plugin %s can't parse %s = %s", pluginName, key, value);
                   esyslog("dbus2vdr: %s.Set: plugin %s can't parse %s = %s", DBUS_VDR_SETUP_INTERFACE, pluginName, key, value);
                   }
                else {
                   replyCode = 900;
                   replyMessage = cString::sprintf("storing %s = %s", name, value);
                   plugin->SetupStore(key, value);
                   Setup.Save();
                   }
                }
             if (dummy != NULL)
                free(dummy);
             }
          cDBusHelper::SendReply(Invocation, replyCode, *replyMessage);
          g_variant_unref(nameChild);
          g_variant_unref(child);
          g_variant_unref(variant);
          return;
          }

       bool save = false;
       bool ModifiedAppearance = false;
       switch (b->Type) {
         case cSetupBinding::dstString:
          {
           const char *str = NULL;
           if (!g_variant_is_of_type(child, G_VARIANT_TYPE_STRING))
              replyMessage = cString::sprintf("argument for %s is not a string", name);
           else {
              g_variant_get(child, "&s", &str);
              replyMessage = cString::sprintf("setting %s = %s", name, str);
              Utf8Strn0Cpy((char*)b->Value, str, b->StrMaxLength);
              save = true;
              // special handling of some setup values
              if ((strcasecmp(name, "OSDLanguage") == 0)
               || (strcasecmp(name, "FontOsd") == 0)
               || (strcasecmp(name, "FontSml") == 0)
               || (strcasecmp(name, "FontFix") == 0)) {
                 ModifiedAppearance = true;
                 }
              else if (strcasecmp(name, "OSDSkin") == 0) {
                 Skins.SetCurrent(str);
                 ModifiedAppearance = true;
                 }
              else if (strcasecmp(name, "OSDTheme") == 0) {
                 cThemes themes;
                 themes.Load(Skins.Current()->Name());
                 if ((themes.NumThemes() > 0) && Skins.Current()->Theme()) {
                    int themeIndex = themes.GetThemeIndex(str);
                    if (themeIndex >= 0) {
                       Skins.Current()->Theme()->Load(themes.FileName(themeIndex));
                       ModifiedAppearance = true;
                       }
                    }
                 }
             }
           break;
          }
         case cSetupBinding::dstInt32:
          {
           int i32;
           if (!g_variant_is_of_type(child, G_VARIANT_TYPE_INT32))
              replyMessage = cString::sprintf("argument for %s is not a 32bit-integer", name);
           else {
              g_variant_get(child, "i", &i32);
              if ((i32 < b->Int32MinValue) || (i32 > b->Int32MaxValue))
                 replyMessage = cString::sprintf("argument for %s is out of range", name);
              else {
                 replyMessage = cString::sprintf("setting %s = %d", name, i32);
                 (*((int*)b->Value)) = i32;
                 save = true;
                 if (strcasecmp(name, "AntiAlias") == 0) {
                    ModifiedAppearance = true;
                    }
                 }
              }
           break;
          }
         case cSetupBinding::dstTimeT:
          {
           time_t i64;
           if (!g_variant_is_of_type(child, G_VARIANT_TYPE_INT64))
              replyMessage = cString::sprintf("argument for %s is not a 64bit-integer", name);
           else {
              g_variant_get(child, "x", &i64);
              replyMessage = cString::sprintf("setting %s = %ld", name, i64);
              (*((time_t*)b->Value)) = i64;
              save = true;
              }
           break;
          }
         }

       if (save) {
          Setup.Save();
          replyCode = 900;
#if VDRVERSNUM > 10706
          if (ModifiedAppearance)
             cOsdProvider::UpdateOsdSize(true);
#endif
          }
       }

    cDBusHelper::SendReply(Invocation, replyCode, *replyMessage);
    g_variant_unref(nameChild);
    g_variant_unref(child);
    g_variant_unref(variant);
  };

  static void Del(cDBusObject *Object, GVariant *Parameters, GDBusMethodInvocation *Invocation)
  {
    const gchar *name = NULL;
    g_variant_get(Parameters, "(&s)", &name);

    isyslog("dbus2vdr: %s.Del: %s", DBUS_VDR_SETUP_INTERFACE, name);

    if (endswith(name, ".*")) {
       // delete all plugin settings
       char *plugin = strdup(name);
       plugin[strlen(name) - 2] = 0;
       cSetupLine *line = Setup.First();
       cSetupLine *next;
       while (line) {
             next = Setup.Next(line);
             if ((line->Plugin() != NULL) && (strcasecmp(line->Plugin(), plugin) == 0)) {
                isyslog("dbus2vdr: %s.Del: deleting %s.%s = %s", DBUS_VDR_SETUP_INTERFACE, line->Plugin(), line->Name(), line->Value());
                Setup.Del(line);
                }
             line = next;
             }
       Setup.Save();
       cDBusHelper::SendReply(Invocation, 900, *cString::sprintf("deleted all settings for plugin %s", plugin));
       free(plugin);
       return;
       }

    cSetupLine delLine;
    if (!delLine.Parse((char*)*cString::sprintf("%s=", name))) {
       esyslog("dbus2vdr: %s.Del: can't parse %s", DBUS_VDR_SETUP_INTERFACE, name);
       cDBusHelper::SendReply(Invocation, 501, *cString::sprintf("can't parse %s", name));
       return;
       }

    for (cSetupLine *line = Setup.First(); line; line = Setup.Next(line)) {
        if (line->Compare(delLine) == 0) {
           Setup.Del(line);
           Setup.Save();
           cDBusHelper::SendReply(Invocation, 900, *cString::sprintf("%s deleted from setup.conf", name));
           return;
           }
        }

    cDBusHelper::SendReply(Invocation, 550, *cString::sprintf("%s not found in setup.conf", name));
  };
};

cList<cDBusSetupHelper::cSetupBinding> cDBusSetupHelper::_bindings;

const char *cDBusSetupHelper::_xmlNodeInfo = 
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "       \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\""DBUS_VDR_SETUP_INTERFACE"\">\n"
    "    <method name=\"List\">\n"
    "      <arg name=\"key_value_list\" type=\"a(sv)\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Get\">\n"
    "      <arg name=\"name\"         type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"value\"        type=\"v\" direction=\"out\"/>\n"
    "      <arg name=\"replycode\"    type=\"i\" direction=\"out\"/>\n"
    "      <arg name=\"replymessage\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Set\">\n"
    "      <arg name=\"name\"         type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"value\"        type=\"v\" direction=\"in\"/>\n"
    "      <arg name=\"replycode\"    type=\"i\" direction=\"out\"/>\n"
    "      <arg name=\"replymessage\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Del\">\n"
    "      <arg name=\"name\"         type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"replycode\"    type=\"i\" direction=\"out\"/>\n"
    "      <arg name=\"replymessage\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "</node>\n";


cDBusSetup::cDBusSetup(void)
:cDBusObject("/Setup", cDBusSetupHelper::_xmlNodeInfo)
{
  cDBusSetupHelper::InitBindings();
  AddMethod("List", cDBusSetupHelper::List);
  AddMethod("Get", cDBusSetupHelper::Get);
  AddMethod("Set", cDBusSetupHelper::Set);
  AddMethod("Del", cDBusSetupHelper::Del);
}

cDBusSetup::~cDBusSetup(void)
{
}
