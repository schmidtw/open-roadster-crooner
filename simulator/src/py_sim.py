#!/usr/bin/env python

import sys
import gobject

try:
    import pygtk
    pygtk.require("2.0")
except:
    pass

try:
    import gtk
    import gtk.glade
except:
    print "You need to install pyGTK or GTKv2";
    sys.exit(1);

class radio_gui:
    gladefile = "simulator.glade";
    glademainwidget = "simulator";
    windowname = "Radio Simulator";
    # mc is the variable for the mode control buttons 1-6
    # trick is the variable for next/previous song or fast forward/rewind depending on the mode
    def __init__(self):
        print "Loading Radio Simulator..."; 
        self.wTree = gtk.glade.XML (self.gladefile);
        self.cd_mode = cd_selection();
        self.mc = media_card(self.wTree.get_widget("media_entry"));
        self.trick = trick_mode(self.wTree.get_widget("m_bt"), self.wTree.get_widget("scan_bt"));
        dic = { "window1_destroy_cb" : self.window1_destroy_cb,
                "1_bt_clicked_cb": self.cd_mode.bt_cb_1,
                "2_bt_clicked_cb": self.cd_mode.bt_cb_2,
                "3_bt_clicked_cb": self.cd_mode.bt_cb_3,
                "4_bt_clicked_cb": self.cd_mode.bt_cb_4,
                "5_bt_clicked_cb": self.cd_mode.bt_cb_5,
                "6_bt_clicked_cb": self.cd_mode.bt_cb_6,
                "insert_bt_toggled_cb": self.mc.toggle_insert,
                "m_bt_toggled_cb": self.trick.toggle_m_bt,
                "rw_bt_clicked_cb": self.trick.rw_bt,
                "ff_bt_clicked_cb": self.trick.ff_bt,
                "scan_bt_toggled_cb": self.trick.scan_bt,
                "power_bt_toggled_cb": self.power_bt_cb
                 };
        self.wTree.signal_autoconnect(dic);
        self.window = self.wTree.get_widget(self.glademainwidget);
        self.toggle_radio_power(False);
        if(self.window):
            self.window.set_title(self.windowname);
            self.window.show();
        else:
            sys.exit(1);
    def window1_destroy_cb(self, widget):
        self.window.destroy();
        print "...Closing Radio Simulator\n";
        gtk.main_quit();
    def power_bt_cb(self, widget):
        print "power button";
        self.toggle_radio_power(widget.get_active());
        if( widget.get_active() ):
            widget.set_label("Turn Off Car");
        else:
            widget.set_label("Turn On Car");
    def toggle_radio_power(self, isPowerOn):
        print "Turning power ",
        if(isPowerOn==True):
            print "on";
        else:
            print "off";
        self.wTree.get_widget("rw_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("ff_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("m_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("1_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("2_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("3_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("4_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("5_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("6_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("scan_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("mode_1_bt").set_sensitive(isPowerOn);
        self.wTree.get_widget("mode_2_bt").set_sensitive(isPowerOn);

# This sub class is of the 1-6 Keys on the radio
class cd_selection:
    last_cd = 1;
    def __init__(self):
        print "Selection/Mode buttons loaded";
    def bt_cb_1(self, widget):
        self.bt_cb(widget, 1);
    def bt_cb_2(self, widget):
        self.bt_cb(widget, 2);
    def bt_cb_3(self, widget):
        self.bt_cb(widget, 3);
    def bt_cb_4(self, widget):
        self.bt_cb(widget, 4);
    def bt_cb_5(self, widget):
        self.bt_cb(widget, 5);
    def bt_cb_6(self, widget):
        self.bt_cb(widget, 6);
    def bt_cb(self, widget, cd):
        if( cd != self.last_cd ):
            self.last_cd = cd;
            print "Now in mode/selection ",self.last_cd;

# Media Card Class
class media_card:
    def __init__(self, entry):
        if(entry):
            entry.set_text("/dev/mmcblk0p1");
            self.entry = entry;
        else:
            print "Media Card Failed initialization";
            sys.exit(1);
        print "Media Card initialized";
    def toggle_insert(self, widget):
        isMediaInserted = widget.get_active();
        if( isMediaInserted ):
            widget.set_label("Remove");
            self.entry.set_sensitive(False);
        else:
            widget.set_label("Insert");
            self.entry.set_sensitive(True);
        print "isMediaInserted now = ",isMediaInserted;

# Trick mode Class
class trick_mode:
    # Constants
    # rate is in milliseconds
    RATE_OF_CALLBACK = 3000;
    def __init__(self, m_widget, scan_widget):
        self.m_widget = m_widget;
        self.scan_widget = scan_widget;
        print "Initializing the trick mode buttons";
        # Get a callback from gtk.main so we can do updates to display
    
    def timeout_callback(self):
        print "time callback";
        self.m_widget.set_active(False);
        return False;
    
    def toggle_m_bt(self, widget):
        isActive = widget.get_active();
        if( isActive ):
            self.timer_source_id = gobject.timeout_add(self.RATE_OF_CALLBACK, self.timeout_callback);
            print "Active";
        else:
            gobject.source_remove(self.timer_source_id);
            print "Not active";
            
    def rw_bt(self, widget):
        self.scan_widget.set_active(False);
        if( self.m_widget.get_active() ):
            self.restart_timer();
            print "rewind";
        else:
            print "seek back";
    def ff_bt(self, widget):
        self.scan_widget.set_active(False);
        if( self.m_widget.get_active() ):
            self.restart_timer();
            print "fast forward";
        else:
            print "seek forward";
    def scan_bt(self, widget):
        if( widget.get_active() ):
            print "now in scan mode";
        else:
            print "leaving scan mode";
        
    def restart_timer(self):
        print "timer reset";
        gobject.source_remove(self.timer_source_id);
        self.timer_source_id = gobject.timeout_add(self.RATE_OF_CALLBACK, self.timeout_callback);
        

if("__main__" == __name__):
    app = radio_gui();
    gtk.main();

