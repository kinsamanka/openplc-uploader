#!/usr/bin/env python

import copy
import pickle
from threading import Thread
import subprocess
import os
from serial.tools import list_ports
from pathlib import Path
from pubsub import pub
import platform
import sys
import wx
from wx.lib.embeddedimage import PyEmbeddedImage


exit_code = 0
is_frozen = getattr(sys, 'frozen', False) and hasattr(sys, '_MEIPASS')
is_win = platform.system() == "Windows"


class WorkerThread(Thread):

    def __init__(self, cwd, e):
        Thread.__init__(self)
        self.cwd = cwd
        self.env = {}
        self.upload = False

        if is_frozen:
            self.cmd = [f'{Path(sys._MEIPASS) / "frozenpy"}', 'pio']
        else:
            self.cmd = ['pio']

        if e['bootloader']:
            self.cmd.extend(['--no-ansi', 'run', '-e', 'stm32_bootloader'])
        else:
            self.cmd.extend(['--no-ansi', 'run', '-e', e['id']])

        esc_quote = "\\'" if not is_win else ""

        if e['src']:
            self.env['OPENPLC_SRC'] = e['src']
        if e['port']:
            self.upload = True
            self.cmd.extend(['-t', 'upload'])
            if e['port'] != 'Default':
                self.env['PLATFORMIO_UPLOAD_PORT'] = e['port']

        f = []
        t = e['mb']['current']['holding_count']
        f.append(f'-DHOLDING_REG_COUNT={t}')
        t = e['mb']['current']['input_count']
        f.append(f'-DINPUT_REG_COUNT={t}')
        t = e['mb']['current']['discrete_count']
        f.append(f'-DDISCRETE_COUNT={t}')
        t = e['mb']['current']['coil_count']
        f.append(f'-DCOIL_COUNT={t}')

        t = ','.join(e['io']['cur_config']['din'])
        f.append(f'-DDIN={esc_quote}{{{t}}}{esc_quote}')
        t = ','.join(e['io']['cur_config']['dout'])
        f.append(f'-DDOUT={esc_quote}{{{t}}}{esc_quote}')
        t = ','.join(e['io']['cur_config']['ain'])
        f.append(f'-DAIN={esc_quote}{{{t}}}{esc_quote}')
        t = ','.join(e['io']['cur_config']['aout'])
        f.append(f'-DAOUT={esc_quote}{{{t}}}{esc_quote}')

        if e['rtu']['master_en']:
            if e['rtu']['master_port']:
                f.append(f'-DMODBUS_MASTER')
                t = e['rtu']['master_port']
                f.append(f'-DMBMASTER_IFACE={t}')
                t = e['rtu']['master_baud']
                f.append(f'-DMASTER_BAUD_RATE={t}')
                t = e['rtu']['master_pin']
                if t:
                    f.append(f'-DRS485_MASTER_EN_PIN={t}')
                    f.append(f'-DRS485_MASTER_EN')

        if e['rtu']['slave_en']:
            if e['rtu']['slave_port']:
                f.append(f'-DMODBUS_SLAVE')
                t = e['rtu']['slave_port']
                f.append(f'-DMBSLAVE_IFACE={t}')
                t = e['rtu']['slave_baud']
                f.append(f'-DSLAVE_BAUD_RATE={t}')
                t = e['rtu']['slave_id']
                f.append(f'-DSLAVE_ADDRESS={t}')
                t = e['rtu']['slave_pin']
                if t:
                    f.append(f'-DRS485_SLAVE_EN_PIN={t}')
                    f.append(f'-DRS485_SLAVE_EN')

        if e['tcp']['en'] and e['tcp']['wired']:
            f.append(f'-DMODBUS_ETH')
            t = e['tcp']['ip'].replace('.', ',')
            if t != '0,0,0,0':
                f.append(f'-DCONFIG_IP={esc_quote}{{{t}}}{esc_quote}')
            t = e['tcp']['subnet'].replace('.', ',')
            if t != '0,0,0,0':
                f.append(f'-DCONFIG_SUBNET={esc_quote}{{{t}}}{esc_quote}')
            t = e['tcp']['gw'].replace('.', ',')
            if t != '0,0,0,0':
                f.append(f'-DCONFIG_GW={esc_quote}{{{t}}}{esc_quote}')
            t = e['tcp']['dns'].replace('.', ',')
            if t != '0,0,0,0':
                f.append(f'-DCONFIG_DNS={esc_quote}{{{t}}}{esc_quote}')
            t = ','.join([f'0x{n}' for n in e['tcp']['mac'].split(':')])
            f.append(f'-DCONFIG_MAC={esc_quote}{{{t}}}{esc_quote}')

        if e['tcp']['en'] and not e['tcp']['wired']:
            f.append(f'-DMODBUS_WIFI')
            t = e['tcp']['ip'].replace('.', ',')
            if t != '0,0,0,0':
                f.append(f'-DCONFIG_WIFI_IP={esc_quote}{{{t}}}{esc_quote}')
            t = e['tcp']['subnet'].replace('.', ',')
            if t != '0,0,0,0':
                f.append(f'-DCONFIG_WIFI_SUBNET={esc_quote}{{{t}}}{esc_quote}')
            t = e['tcp']['gw'].replace('.', ',')
            if t != '0,0,0,0':
                f.append(f'-DCONFIG_WIFI_GW={esc_quote}{{{t}}}{esc_quote}')
            t = e['tcp']['dns'].replace('.', ',')
            if t != '0,0,0,0':
                f.append(f'-DCONFIG_WIFI_DNS={esc_quote}{{{t}}}{esc_quote}')
            t = e['tcp']['ssid']
            if t:
                if is_win:
                    f.append(f"-DCONFIG_SSID='\"{t}\"'")
                else:
                    t = t.replace(' ', '\\ ')
                    f.append(f'-DCONFIG_SSID=\\"{t}\\"')
            t = e['tcp']['password']
            if t:
                if is_win:
                    f.append(f"-DCONFIG_WIFI_PASS='\"{t}\"'")
                else:
                    t = t.replace(' ', '\\ ')
                    f.append(f'-DCONFIG_WIFI_PASS=\\"{t}\\"')

        self.bootload = e['bootloader']
        l = e['led'].copy()
        if e['bootloader']:
            if l:
                s = l.pop()
                f.append(f'-DRUN_LED={esc_quote}{{{{GPIO{s[1:2]},GPIO{s[2:]}}},0}}{esc_quote}')
            if l:
                s = l.pop()
                f.append(f'-DERR_LED={esc_quote}{{{{GPIO{s[1:2]},GPIO{s[2:]}}},0}}{esc_quote}')
        else:
            if l:
                s = l.pop()
                f.append(f'-DRUN_LED={s}')
            if l:
                s = l.pop()
                f.append(f'-DERR_LED={s}')

        if e['bootloader']:
            if e['upload_speed']:
                f.append(f"-DUPLOAD_SPEED={e['upload_speed']}")

        self.env['PLATFORMIO_BUILD_SRC_FLAGS'] = ' '.join(f)
        self.env['PLATFORMIO_SETTING_CHECK_PLATFORMIO_INTERVAL'] = '9999'
        self.env['PLATFORMIO_SETTING_ENABLE_TELEMETRY'] = 'No'

        self.start()

    def run(self):
        self.process = subprocess.Popen(
            self.cmd,
            cwd=self.cwd,
            encoding='utf-8',
            env={**os.environ, **self.env},
            errors='replace',
            shell=False,
            creationflags=0 if not is_win else subprocess.CREATE_NO_WINDOW,
            stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE,
        )

        out = {'busy': True, 'data': [], 'index': 1}

        out['data'].append(f'{"":=>28} Environment Variables {"":=<28}')
        for s in self.env['PLATFORMIO_BUILD_SRC_FLAGS'].split():
            out['data'].append(s)
            out['index'] += 1
        out['data'].append(f'{"":=>36} Start {"":=<36}')

        while True:
            r = self.process.stdout.readline()
            if r == '' and self.process.poll() is not None:
                break
            if r:
                out['index'] += 1
                out['data'].append(r.strip())
                wx.CallAfter(pub.sendMessage, "console_output", e=out)

        out['busy'] = False
        wx.CallAfter(pub.sendMessage, "console_output", e=out)


class TcpPanel(wx.Panel):

    def __init__(self, parent):
        super().__init__(parent)

        self.mac = "DE:AD:BE:EF:DE:AD"
        self.ip = "0.0.0.0"
        self.gw = "0.0.0.0"
        self.dns = "0.0.0.0"
        self.subnet = "255.255.255.0"
        self.ssid = ''
        self.password = ''
        self.wired = True
        self.en = False
        self.no_wifi = 0
        self.no_eth = 0

        pub.subscribe(self.on_config_change, "config_change")

        _font = wx.Font(10, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL,
                        wx.FONTWEIGHT_NORMAL, 0, "Monospace")

        sz_0 = wx.GridBagSizer(5, 10)

        self.cb_en = wx.CheckBox(self, wx.ID_ANY, "Enable")
        self.cb_en.SetValue(self.en)
        self.cb_en.Bind(wx.EVT_CHECKBOX, self.enable_tcp)
        sz_0.Add(self.cb_en, (0, 0), (1, 1), wx.ALIGN_CENTER_VERTICAL, 0)

        self.rb_type = wx.RadioBox(
            self, -1, "", choices=["Wired", "WiFi"], style=wx.RA_SPECIFY_COLS)
        self.rb_type.SetSelection(0)
        self.rb_type.Enable(False)
        self.rb_type.Bind(wx.EVT_RADIOBOX, self.update_src)
        sz_0.Add(self.rb_type, (0, 1), (1, 1), wx.EXPAND, 0)

        st_0 = wx.StaticText(self, wx.ID_ANY, "Address")
        sz_0.Add(st_0, (1, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_ip = wx.TextCtrl(self, -1, self.ip)
        self.tc_ip.SetMinSize((160, -1))
        self.tc_ip.SetFont(_font)
        self.tc_ip.Enable(False)
        self.tc_ip.Bind(wx.EVT_TEXT, self.on_ip)
        sz_0.Add(self.tc_ip, (1, 1), (1, 1), 0, 0)

        st_1 = wx.StaticText(self, -1, "Gateway")
        sz_0.Add(st_1, (1, 2), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_gw = wx.TextCtrl(self, -1, self.gw)
        self.tc_gw.SetMinSize((160, -1))
        self.tc_gw.SetFont(_font)
        self.tc_gw.Enable(False)
        self.tc_gw.Bind(wx.EVT_TEXT, self.on_gw)
        sz_0.Add(self.tc_gw, (1, 3), (1, 1), wx.RIGHT, 10)

        st_2 = wx.StaticText(self, -1, "Subnet")
        sz_0.Add(st_2, (2, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_subnet = wx.TextCtrl(self, -1, self.subnet)
        self.tc_subnet.SetFont(_font)
        self.tc_subnet.Enable(False)
        self.tc_subnet.Bind(wx.EVT_TEXT, self.on_subnet)
        sz_0.Add(self.tc_subnet, (2, 1), (1, 1), wx.EXPAND)

        st_3 = wx.StaticText(self, -1, "DNS")
        sz_0.Add(st_3, (2, 2), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_dns = wx.TextCtrl(self, -1, self.dns)
        self.tc_dns.SetFont(_font)
        self.tc_dns.Enable(False)
        self.tc_dns.Bind(wx.EVT_TEXT, self.on_dns)
        sz_0.Add(self.tc_dns, (2, 3), (1, 1), wx.EXPAND | wx.RIGHT, 10)

        st_4 = wx.StaticText(self, -1, "SSID")
        sz_0.Add(st_4, (3, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_ssid = wx.TextCtrl(self, -1, "", name="ssid")
        self.tc_ssid.SetFont(_font)
        self.tc_ssid.Enable(False)
        self.tc_ssid.Bind(wx.EVT_TEXT, self.on_wifi)
        sz_0.Add(self.tc_ssid, (3, 1), (1, 1), wx.EXPAND)

        st_5 = wx.StaticText(self, -1, "Password")
        sz_0.Add(st_5, (3, 2), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_password = wx.TextCtrl(self, -1, "", name="password")
        self.tc_password.SetFont(_font)
        self.tc_password.Enable(False)
        self.tc_password.Bind(wx.EVT_TEXT, self.on_wifi)
        sz_0.Add(self.tc_password, (3, 3), (1, 1), wx.EXPAND | wx.RIGHT, 10)

        st_6 = wx.StaticText(self, -1, "MAC")
        sz_0.Add(st_6, (4, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_mac = wx.TextCtrl(self, -1, self.mac)
        self.tc_mac.SetFont(_font)
        self.tc_mac.Enable(False)
        self.tc_mac.Bind(wx.EVT_TEXT, self.on_mac)
        sz_0.Add(self.tc_mac, (4, 1), (1, 3), wx.EXPAND | wx.RIGHT, 10)

        self.SetSizer(sz_0)

    def on_wifi(self, e):
        n = e.GetEventObject()
        r = str(n.GetValue().strip())
        setattr(self, n.GetName(), r)

    def on_ip(self, e):
        n = e.GetEventObject()
        r = str(n.GetValue().strip())

        m = n.GetInsertionPoint()
        n.ChangeValue(self.ip)

        if r and all(x in '0123456789.' for x in r):
            n.ChangeValue(r)
            self.ip = r

        n.SetInsertionPoint(m)

    def on_gw(self, e):
        n = e.GetEventObject()
        r = str(n.GetValue().strip())

        m = n.GetInsertionPoint()
        n.ChangeValue(self.gw)

        if r and all(x in '0123456789.' for x in r):
            n.ChangeValue(r)
            self.gw = r

        n.SetInsertionPoint(m)

    def on_dns(self, e):
        n = e.GetEventObject()
        r = str(n.GetValue().strip())

        m = n.GetInsertionPoint()
        n.ChangeValue(self.dns)

        if r and all(x in '0123456789.' for x in r):
            n.ChangeValue(r)
            self.dns = r

        n.SetInsertionPoint(m)

    def on_subnet(self, e):
        n = e.GetEventObject()
        r = str(n.GetValue().strip())

        m = n.GetInsertionPoint()
        n.ChangeValue(self.subnet)

        if r and all(x in '0123456789.' for x in r):
            n.ChangeValue(r)
            self.subnet = r

        n.SetInsertionPoint(m)

    def on_mac(self, e):
        n = e.GetEventObject()
        r = str(n.GetValue().strip())

        m = n.GetInsertionPoint()
        n.ChangeValue(self.mac)

        if r and all(x in '0123456789abcdef:' for x in r.lower()):
            n.ChangeValue(r)
            self.mac = r

        n.SetInsertionPoint(m)

    def enable_forms(self, en):
        wifi = self.rb_type.GetSelection() and en
        self.rb_type.Enable(en)
        self.tc_dns.Enable(en)
        self.tc_gw.Enable(en)
        self.tc_ip.Enable(en)
        self.tc_subnet.Enable(en)
        if en:
            self.tc_mac.Enable(not wifi)
            self.tc_password.Enable(wifi)
            self.tc_ssid.Enable(wifi)
        else:
            self.tc_mac.Enable(en)
            self.tc_password.Enable(en)
            self.tc_ssid.Enable(en)

    def _update(self):
        if self.no_eth and self.no_wifi:
            self.cb_en.SetValue(False)
            self.cb_en.Disable()
            self.enable_forms(False)
            self.en = False
        elif self.no_eth and self.en:
            self.rb_type.SetSelection(1)
            self.rb_type.EnableItem(0, False)
            self.update_src(None)
        elif self.no_wifi and self.en:
            self.rb_type.SetSelection(0)
            self.rb_type.EnableItem(1, False)
            self.update_src(None)
        elif self.en:
            self.rb_type.EnableItem(0, True)
            self.rb_type.EnableItem(1, True)
            self.enable_forms(True)
        else:
            self.cb_en.Enable()

    def enable_tcp(self, e):
        self.en = e.GetEventObject().GetValue()
        self.enable_forms(self.en)
        self._update()

    def update_src(self, e):
        wifi = self.rb_type.GetSelection() == 1

        self.tc_ssid.Enable(wifi)
        self.tc_password.Enable(wifi)
        self.tc_mac.Enable(not wifi)
        self.wired = not wifi

    def get_values(self):
        return {
            'dns': self.dns,
            'en': self.en,
            'gw': self.gw,
            'ip': self.ip,
            'mac': self.mac,
            'password': self.password,
            'ssid': self.ssid,
            'subnet': self.subnet,
            'wired': self.wired,
            'no_eth': self.no_eth,
            'no_wifi': self.no_wifi,
        }

    def set_values(self, val):
        for a in ('dns', 'gw', 'ip', 'mac', 'password', 'ssid', 'subnet'):
            f = getattr(self, f'tc_{a}')
            f.ChangeValue(val[a])
            setattr(self, a, val[a])

        if val['wired']:
            self.rb_type.SetSelection(0)
        else:
            self.rb_type.SetSelection(1)
        self.wired = val['wired']

        self.en = val['en']
        self.cb_en.SetValue(self.en)
        self.enable_forms(self.en)

        self.no_eth = val['no_eth']
        self.no_wifi = val['no_wifi']
        self._update()

    def on_config_change(self, msg):
        self.no_eth = msg['no_eth']
        self.no_wifi = msg['no_wifi']
        self._update()


class RtuPanel(wx.Panel):

    def __init__(self, parent):
        super().__init__(parent)

        pub.subscribe(self.on_config_change, "config_change")

        _baud_rate = ["9600", "14400", "19200", "38400", "57600", "115200"]

        self.uart_count = 1
        self.uarts = ["Serial"]

        self.pins = []
        self.master_pin = ''
        self.slave_pin = ''

        self.slave_id = '1'

        self.upload_speed = 0

        sz_0 = wx.BoxSizer(wx.HORIZONTAL)

        sz_1 = wx.StaticBoxSizer(wx.StaticBox(
            self, -1, "Slave"), wx.HORIZONTAL)
        sz_0.Add(sz_1, 1, wx.ALL | wx.EXPAND, 5)

        gs_0 = wx.FlexGridSizer(5, 2, 5, 10)
        sz_1.Add(gs_0, 1, 0, 0)

        self.cb_slave_en = wx.CheckBox(self, -1, "Enable")
        self.cb_slave_en.Bind(wx.EVT_CHECKBOX, self.enable_slave)
        gs_0.Add(self.cb_slave_en, 0, 0, 0)

        gs_0.Add((0, 0), 0, 0, 0)

        st_0 = wx.StaticText(self, -1, "Port")
        gs_0.Add(st_0, 0, wx.ALIGN_RIGHT, 0)

        self.cbb_slave_port = wx.ComboBox(
            self,
            choices=self.uarts,
            name='slave',
            style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.cbb_slave_port.Enable(False)
        self.cbb_slave_port.Bind(wx.EVT_COMBOBOX, self.on_port)
        gs_0.Add(self.cbb_slave_port, 0, wx.EXPAND, 0)

        st_1 = wx.StaticText(self, -1, "Speed")
        gs_0.Add(st_1, 0, wx.ALIGN_RIGHT, 0)

        self.cbb_slave_baud = wx.ComboBox(self, -1, "57600", choices=_baud_rate,
                                          style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.cbb_slave_baud.Enable(False)
        gs_0.Add(self.cbb_slave_baud, 0, wx.EXPAND, 0)

        st_2 = wx.StaticText(self, -1, "EN Pin")
        gs_0.Add(st_2, 0, wx.ALIGN_RIGHT, 0)

        self.cbb_slave_en_pin = wx.ComboBox(
            self, name='slave', style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.cbb_slave_en_pin.Bind(wx.EVT_COMBOBOX, self.on_pins)
        self.cbb_slave_en_pin.Enable(False)
        gs_0.Add(self.cbb_slave_en_pin, 0, wx.EXPAND, 0)

        st_3 = wx.StaticText(self, -1, "ID")
        gs_0.Add(st_3, 0, wx.ALIGN_RIGHT, 0)

        self.tc_slave_id = wx.TextCtrl(self, -1, "1")
        self.tc_slave_id.Enable(False)
        self.tc_slave_id.Bind(wx.EVT_TEXT, self.on_id)
        gs_0.Add(self.tc_slave_id, 0, wx.EXPAND, 0)

        sz_2 = wx.StaticBoxSizer(wx.StaticBox(
            self, -1, "Master"), wx.HORIZONTAL)
        sz_0.Add(sz_2, 1, wx.ALL | wx.EXPAND, 5)

        gs_1 = wx.FlexGridSizer(4, 2, 5, 10)
        sz_2.Add(gs_1, 1, 0, 0)

        self.cb_master_en = wx.CheckBox(self, -1, "Enable")
        self.cb_master_en.Bind(wx.EVT_CHECKBOX, self.enable_master)
        gs_1.Add(self.cb_master_en, 0, 0, 0)

        gs_1.Add((0, 0), 0, 0, 0)

        st_4 = wx.StaticText(self, -1, "Port")
        gs_1.Add(st_4, 0, wx.ALIGN_RIGHT, 0)

        self.cbb_master_port = wx.ComboBox(
            self,
            choices=self.uarts,
            name='master',
            style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.cbb_master_port.Bind(wx.EVT_COMBOBOX, self.on_port)
        self.cbb_master_port.Enable(False)
        gs_1.Add(self.cbb_master_port, 0, wx.EXPAND, 0)

        st_5 = wx.StaticText(self, -1, "Speed")
        gs_1.Add(st_5, 0, wx.ALIGN_RIGHT, 0)

        self.cbb_master_baud = wx.ComboBox(
            self, -1, "57600", choices=_baud_rate, style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.cbb_master_baud.Enable(False)
        gs_1.Add(self.cbb_master_baud, 0, wx.EXPAND, 0)

        st_6 = wx.StaticText(self, -1, "EN Pin")
        gs_1.Add(st_6, 0, wx.ALIGN_RIGHT, 0)

        self.cbb_master_en_pin = wx.ComboBox(
            self, name='master', style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.cbb_master_en_pin.Bind(wx.EVT_COMBOBOX, self.on_pins)
        self.cbb_master_en_pin.Enable(False)
        gs_1.Add(self.cbb_master_en_pin, 0, wx.EXPAND, 0)

        self.SetSizer(sz_0)

    def on_id(self, e):
        n = e.GetEventObject()
        r = str(n.GetValue().strip())

        m = n.GetInsertionPoint()
        n.ChangeValue(self.slave_id)

        if r and all(x in '0123456789' for x in r):
            if int(r) > 0 and int(r) < 256:
                n.ChangeValue(r)
                self.slave_id = r

        n.SetInsertionPoint(m)

    def on_pins(self, e):
        n = e.GetEventObject().GetName()
        x = e.GetEventObject().GetValue()

        if n == 'master':
            if x == self.cbb_slave_en_pin.GetStringSelection():
                self.clear_slave_en()

        elif n == 'slave':
            if x == self.cbb_master_en_pin.GetStringSelection():
                self.clear_master_en()

        self.send_pin_change()

    def on_port(self, e):
        n = e.GetEventObject().GetName()
        x = e.GetEventObject().GetValue()

        if n == 'master':
            if x == 'UART1' and not self.pins:
                self.cbb_master_baud.Enable(False)
                self.cbb_master_baud.SetValue(self.upload_speed)

            if x == self.cbb_slave_port.GetStringSelection():
                self.cbb_slave_port.Clear()
                self.cbb_slave_port.Append(self.uarts)
                if self.cb_slave_en.GetValue() and x == 'UART1':
                    self.cbb_slave_baud.Enable(True)
        elif n == 'slave':
            if x == 'UART1' and not self.pins:
                self.cbb_slave_baud.Enable(False)
                self.cbb_slave_baud.SetValue(self.upload_speed)

            if x == self.cbb_slave_port.GetStringSelection():
                self.cbb_master_port.Clear()
                self.cbb_master_port.Append(self.uarts)
                if self.cb_master_en.GetValue() and x == 'UART1':
                    self.cbb_master_baud.Enable(True)

    def send_pin_change(self, update=False):
        pins = []
        self.master_pin = self.cbb_master_en_pin.GetValue()
        self.slave_pin = self.cbb_slave_en_pin.GetValue()
        for a in (self.master_pin, self.slave_pin):
            if a:
                pins.append(a)

        pub.sendMessage("pin_change", msg={'pins': pins, 'update': update})

    def clear_master_en(self):
        self.cbb_master_en_pin.Clear()
        if self.pins:
            self.cbb_master_en_pin.Append(self.pins)

    def clear_slave_en(self):
        self.cbb_slave_en_pin.Clear()
        if self.pins:
            self.cbb_slave_en_pin.Append(self.pins)

    def enable_slave(self, e):
        state = e.GetEventObject().GetValue()

        self.cbb_slave_port.Enable(state)
        if not self.pins:
            self.cbb_slave_en_pin.Enable(False)
        else:
            self.cbb_slave_en_pin.Enable(state)

        self.tc_slave_id.Enable(state)

        if (self.uart_count == 1) and state:
            self.cbb_slave_port.SetSelection(0)
            self.cb_master_en.SetValue(False)
            self.cbb_master_port.Enable(False)
            self.cbb_master_baud.Enable(False)
            self.cbb_master_en_pin.Enable(False)
            self.clear_master_en()
        elif (self.uart_count > 1) and (self.cbb_slave_port.GetSelection() == -1) and state:
            if (self.cbb_master_port.GetSelection() == 0):
                self.cbb_slave_port.SetSelection(1)
            else:
                self.cbb_slave_port.SetSelection(0)

        if self.cbb_slave_port.GetStringSelection() == 'UART1' and not self.pins:
            self.cbb_slave_baud.Enable(False)
            self.cbb_slave_baud.SetValue(self.upload_speed)
        else:
            self.cbb_slave_baud.Enable(state)

        if not state:
            self.clear_slave_en()

        self.send_pin_change()

    def enable_master(self, e):
        state = e.GetEventObject().GetValue()

        self.cbb_master_port.Enable(state)
        if not self.pins:
            self.cbb_master_en_pin.Enable(False)
        else:
            self.cbb_master_en_pin.Enable(state)

        if (self.uart_count == 1) and state:
            self.cbb_master_port.SetSelection(0)
            self.cb_slave_en.SetValue(False)
            self.cbb_slave_port.Enable(False)
            self.cbb_slave_baud.Enable(False)
            self.cbb_slave_en_pin.Enable(False)
            self.tc_slave_id.Enable(False)
            self.clear_slave_en()
        elif (self.uart_count > 1) and (self.cbb_master_port.GetSelection() == -1) and state:
            if (self.cbb_slave_port.GetSelection() == 1):
                self.cbb_master_port.SetSelection(0)
            else:
                self.cbb_master_port.SetSelection(1)

        if self.cbb_master_port.GetStringSelection() == 'UART1' and not self.pins:
            self.cbb_master_baud.Enable(False)
            self.cbb_master_baud.SetValue(self.upload_speed)
        else:
            self.cbb_master_baud.Enable(state)

        if not state:
            self.clear_master_en()

        self.send_pin_change()

    def on_config_change(self, msg):
        self.upload_speed = msg['upload_speed']
        c = msg['uart_count']
        if c == 1:
            self.uarts = ["Serial"]
        elif c == 2:
            self.uarts = ["Serial", "Serial1"]
        else:
            self.uarts = ["Serial", "Serial1", "Serial2", "Serial3"]

        l1 = []
        l2 = []
        for a in msg['din'] + msg['dout'] + msg['ain'] + msg['aout']:
            if a.isnumeric():
                l1.append(a)
            else:
                l2.append(a)
        l1.sort(key=int)
        l2.sort()
        self.pins = l1 + l2

        if msg['uarts']:
            self.uarts = msg['uarts']
            c = len(self.uarts)

        self.uart_count = c

        p1 = self.cbb_master_port.GetStringSelection()
        p2 = self.cbb_slave_port.GetStringSelection()

        self.cbb_master_en_pin.Clear()
        self.cbb_master_port.Clear()
        self.cbb_slave_en_pin.Clear()
        self.cbb_slave_port.Clear()

        self.cbb_master_en_pin.Append(self.pins)
        self.cbb_master_port.Append(self.uarts)
        self.cbb_slave_en_pin.Append(self.pins)
        self.cbb_slave_port.Append(self.uarts)

        if p1 in self.uarts:
            self.cbb_master_port.SetValue(p1)
            if self.cb_master_en.GetValue() and self.pins:
                self.cbb_master_en_pin.Enable(True)

            if p1 == 'UART1' and not self.pins:
                self.cbb_master_baud.Enable(False)
                self.cbb_master_baud.SetValue(self.upload_speed)
        else:
            self.cb_master_en.SetValue(False)
            self.cbb_master_port.Enable(False)
            self.cbb_master_baud.Enable(False)
            self.cbb_master_en_pin.Enable(False)
            self.clear_master_en()

        if p2 in self.uarts:
            self.cbb_slave_port.SetValue(p2)
            if self.cb_slave_en.GetValue() and self.pins:
                self.cbb_slave_en_pin.Enable(True)

            if p2 == 'UART1' and not self.pins:
                self.cbb_slave_baud.Enable(False)
                self.cbb_slave_baud.SetValue(self.upload_speed)
        else:
            self.cb_slave_en.SetValue(False)
            self.cbb_slave_port.Enable(False)
            self.cbb_slave_baud.Enable(False)
            self.cbb_slave_en_pin.Enable(False)
            self.clear_slave_en()

        if self.master_pin and self.master_pin in self.pins:
            self.cbb_master_en_pin.SetValue(self.master_pin)
        else:
            self.master_pin = ''

        if self.slave_pin and self.slave_pin in self.pins:
            self.cbb_slave_en_pin.SetValue(self.slave_pin)
        else:
            self.slave_pin = ''

        if not self.pins:
            self.cbb_master_en_pin.Enable(False)
            self.cbb_slave_en_pin.Enable(False)

        if c == 1 and self.cb_slave_en.GetValue() and self.cb_master_en.GetValue():
            self.cb_master_en.SetValue(False)
            self.cbb_master_port.Enable(False)
            self.cbb_master_baud.Enable(False)
            self.cbb_master_en_pin.Enable(False)
            self.clear_master_en()

        self.send_pin_change(True)

    def get_values(self):
        return {
            'master_baud': self.cbb_master_baud.GetStringSelection(),
            'master_en': self.cb_master_en.GetValue(),
            'master_pin': self.master_pin,
            'master_port': self.cbb_master_port.GetStringSelection(),
            'pins': self.pins,
            'slave_baud': self.cbb_slave_baud.GetStringSelection(),
            'slave_en': self.cb_slave_en.GetValue(),
            'slave_id': self.slave_id,
            'slave_pin': self.slave_pin,
            'slave_port': self.cbb_slave_port.GetStringSelection(),
            'uart_count': self.uart_count,
            'uarts': self.uarts,
            'upload_speed': self.upload_speed,
        }

    def set_values(self, val):
        for a in (
                'master_pin',
                'pins',
                'slave_id',
                'slave_pin',
                'uart_count',
                'uarts',
                'upload_speed'):
            setattr(self, a, val[a])

        self.cbb_master_en_pin.Clear()
        self.cbb_master_port.Clear()
        self.cbb_slave_en_pin.Clear()
        self.cbb_slave_port.Clear()
        self.cbb_master_en_pin.Append(self.pins)
        self.cbb_master_port.Append(self.uarts)
        self.cbb_slave_en_pin.Append(self.pins)
        self.cbb_slave_port.Append(self.uarts)

        self.cbb_master_en_pin.SetValue(self.master_pin)
        self.cbb_master_port.SetValue(val['master_port'])
        self.cbb_slave_en_pin.SetValue(self.slave_pin)
        self.cbb_slave_port.SetValue(val['slave_port'])

        self.cbb_master_baud.SetValue(val['master_baud'])
        self.cbb_slave_baud.SetValue(val['slave_baud'])

        state = val['master_en']
        self.cb_master_en.SetValue(state)
        self.cbb_master_port.Enable(state)
        self.cbb_master_baud.Enable(state)
        self.cbb_master_en_pin.Enable(state)

        state = val['slave_en']
        self.cb_slave_en.SetValue(state)
        self.cbb_slave_port.Enable(state)
        self.cbb_slave_baud.Enable(state)
        self.cbb_slave_en_pin.Enable(state)
        self.tc_slave_id.Enable(state)
        self.tc_slave_id.ChangeValue(self.slave_id)

        if not self.pins:
            self.cbb_master_en_pin.Enable(False)
            self.cbb_slave_en_pin.Enable(False)
            if val['master_port'] == 'UART1':
                self.cbb_master_baud.Enable(False)
            if val['slave_port'] == 'UART1':
                self.cbb_slave_baud.Enable(False)


class MBPanel(wx.Panel):

    def __init__(self, parent):
        super().__init__(parent)

        pub.subscribe(self.on_config_change, "config_change")
        pub.subscribe(self.on_io_change, "io_change")

        self.en = False

        self.default = {
            'coil_count': '',
            'discrete_count': '',
            'holding_count': '',
            'input_count': '',
        }

        _font = wx.Font(10, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL,
                        wx.FONTWEIGHT_NORMAL, 0, "Monospace")

        self.current = self.default.copy()
        self.io_changes = {}

        sz_0 = wx.GridBagSizer(5, 5)

        self.cb_override = wx.CheckBox(self, -1, "Override Defaults")
        self.cb_override.Bind(wx.EVT_CHECKBOX, self.enable_override)
        sz_0.Add(self.cb_override, (0, 0), (1, 1), wx.ALIGN_CENTER_VERTICAL, 0)

        st_0 = wx.StaticText(self, -1, "Coils")
        sz_0.Add(st_0, (2, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.st_coil = wx.StaticText(self, -1, "(%QX0.0 - %QX1.0)")
        self.st_coil.SetFont(_font)
        sz_0.Add(self.st_coil, (2, 2), (1, 1), wx.ALIGN_LEFT | wx.LEFT, 10)

        self.tc_coil = wx.TextCtrl(self, name='coil')
        self.tc_coil.Enable(False)
        self.tc_coil.Bind(wx.EVT_TEXT, self.on_inputs)
        sz_0.Add(self.tc_coil, (2, 1), (1, 1))

        st_1 = wx.StaticText(self, -1, "Discrete Inputs")
        sz_0.Add(st_1, (1, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.st_discrete = wx.StaticText(self, -1, "(%IX0.0 - %IX1.0)")
        self.st_discrete.SetFont(_font)
        sz_0.Add(self.st_discrete, (1, 2), (1, 1), wx.ALIGN_LEFT | wx.LEFT, 10)

        self.tc_discrete = wx.TextCtrl(self, name='discrete')
        self.tc_discrete.Enable(False)
        self.tc_discrete.Bind(wx.EVT_TEXT, self.on_inputs)
        sz_0.Add(self.tc_discrete, (1, 1), (1, 1))

        st_2 = wx.StaticText(self, -1, "Holding Registers")
        sz_0.Add(st_2, (4, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.st_holding = wx.StaticText(self, -1, "(%QW0 - %QW1)")
        self.st_holding.SetFont(_font)
        sz_0.Add(self.st_holding, (4, 2), (1, 1), wx.ALIGN_LEFT | wx.LEFT, 10)

        self.tc_holding = wx.TextCtrl(self, name='holding')
        self.tc_holding.Enable(False)
        self.tc_holding.Bind(wx.EVT_TEXT, self.on_inputs)
        sz_0.Add(self.tc_holding, (4, 1), (1, 1))

        st_3 = wx.StaticText(self, -1, "Input Registers")
        sz_0.Add(st_3, (3, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.st_input = wx.StaticText(self, -1, "(%IW0 - %IW1)")
        self.st_input.SetFont(_font)
        sz_0.Add(self.st_input, (3, 2), (1, 1), wx.ALIGN_LEFT | wx.LEFT, 10)

        self.tc_input = wx.TextCtrl(self, name='input')
        self.tc_input.Enable(False)
        self.tc_input.Bind(wx.EVT_TEXT, self.on_inputs)
        sz_0.Add(self.tc_input, (3, 1), (1, 1))

        self.SetSizer(sz_0)

    def _update(self, l):
        def _conv(n):
            try:
                return int(n) - 1
            except ValueError:
                return 0

        if l == 'coil':
            n = self.current['coil_count']
            x, y = divmod(_conv(n), 8)
            self.tc_coil.ChangeValue(n)
            self.st_coil.SetLabel(f"(%QX0.0 - %QX{x}.{y})")
        elif l == 'discrete':
            n = self.current['discrete_count']
            x, y = divmod(_conv(n), 8)
            self.tc_discrete.ChangeValue(n)
            self.st_discrete.SetLabel(f"(%IX0.0 - %IX{x}.{y})")
        elif l == 'holding':
            n = self.current['holding_count']
            self.tc_holding.ChangeValue(n)
            self.st_holding.SetLabel(f"(%QW0 - %QW{_conv(n)})")
        elif l == 'input':
            n = self.current['input_count']
            self.tc_input.ChangeValue(n)
            self.st_input.SetLabel(f"(%IW0 - %IW{_conv(n)})")

    def update_inputs(self):
        for a in ('coil', 'discrete', 'holding', 'input'):
            self._update(a)

    def enable_override(self, e):
        en = e.GetEventObject().GetValue()

        self.tc_coil.Enable(en)
        self.tc_discrete.Enable(en)
        self.tc_holding.Enable(en)
        self.tc_input.Enable(en)
        self.en = en

        if not en:
            self.current = self.default.copy()
            if self.io_changes:
                for a in self.default:
                    if self.io_changes[a] > int(self.default[a]):
                        self.current[a] = str(self.io_changes[a])
            self.update_inputs()

    def on_inputs(self, e):
        o = e.GetEventObject()
        n = o.GetName()
        r = str(o.GetValue().strip())
        m = o.GetInsertionPoint()
        o.ChangeValue(self.current[f'{n}_count'])

        if r and all(x in '0123456789' for x in r):
            if int(r) > 0 and int(r) < 128:
                o.ChangeValue(r)
                self.current[f'{n}_count'] = r

        o.SetInsertionPoint(m)
        self._update(n)

    def on_config_change(self, msg):
        for a in ('coil', 'discrete', 'holding', 'input'):
            self.default[f'{a}_count'] = msg[f'{a}_count']

        if not self.en:
            self.current = self.default.copy()
            self.update_inputs()

    def on_io_change(self, msg):
        self.io_changes = msg
        if not self.en:
            for a in self.default:
                if msg[a] > int(self.default[a]):
                    self.current[a] = str(msg[a])
                else:
                    self.current[a] = self.default[a]
            self.update_inputs()
            self.io_changes = {}

    def get_values(self):
        return {
            'default': self.default,
            'current': self.current,
            'en': self.en,
        }

    def set_values(self, val):
        self.default = val['default']
        self.current = val['current']
        self.update_inputs()

        en = val['en']
        self.cb_override.SetValue(en)
        self.tc_coil.Enable(en)
        self.tc_discrete.Enable(en)
        self.tc_holding.Enable(en)
        self.tc_input.Enable(en)
        self.en = en


class IoPanel(wx.Panel):

    def __init__(self, parent):
        super().__init__(parent)

        self.cur_config = {}
        self.def_config = {}

        self.en_pins = []
        self._en_pins_update = False

        pub.subscribe(self.on_config_change, "config_change")
        pub.subscribe(self.on_pin_change, "pin_change")

        sz_0 = wx.GridBagSizer(5, 5)

        self.cb_override = wx.CheckBox(self, -1, "Override Defaults")
        self.cb_override.Bind(wx.EVT_CHECKBOX, self.enable_override)
        sz_0.Add(self.cb_override, (0, 0), (1, 1), wx.ALIGN_CENTER_VERTICAL, 0)

        st_0 = wx.StaticText(self, -1, "Digital Inputs")
        sz_0.Add(st_0, (1, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_din = wx.TextCtrl(
            self,
            style=wx.TE_BESTWRAP | wx.TE_MULTILINE | wx.TE_NO_VSCROLL,
            name='din')
        self.tc_din.Enable(False)
        self.tc_din.Bind(wx.EVT_TEXT, self.update_text_ctrl)
        sz_0.Add(self.tc_din, (1, 1), (1, 1), wx.RIGHT | wx.EXPAND, 10)

        st_1 = wx.StaticText(self, -1, "Digital Outputs")
        sz_0.Add(st_1, (2, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_dout = wx.TextCtrl(
            self,
            style=wx.TE_BESTWRAP | wx.TE_MULTILINE | wx.TE_NO_VSCROLL,
            name='dout')
        self.tc_dout.Enable(False)
        self.tc_dout.Bind(wx.EVT_TEXT, self.update_text_ctrl)
        sz_0.Add(self.tc_dout, (2, 1), (1, 1), wx.EXPAND | wx.RIGHT, 10)

        st_2 = wx.StaticText(self, -1, "Analog Inputs")
        sz_0.Add(st_2, (3, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_ain = wx.TextCtrl(
            self,
            style=wx.TE_BESTWRAP | wx.TE_MULTILINE | wx.TE_NO_VSCROLL,
            name='ain')
        self.tc_ain.Enable(False)
        self.tc_ain.Bind(wx.EVT_TEXT, self.update_text_ctrl)
        sz_0.Add(self.tc_ain, (3, 1), (1, 1), wx.EXPAND | wx.RIGHT, 10)

        st_3 = wx.StaticText(self, -1, "Analog Outputs")
        sz_0.Add(st_3, (4, 0), (1, 1), wx.ALIGN_RIGHT, 0)

        self.tc_aout = wx.TextCtrl(
            self,
            style=wx.TE_BESTWRAP | wx.TE_MULTILINE | wx.TE_NO_VSCROLL,
            name='aout')
        self.tc_aout.Enable(False)
        self.tc_aout.Bind(wx.EVT_TEXT, self.update_text_ctrl)
        sz_0.Add(self.tc_aout, (4, 1), (1, 1),
                 wx.BOTTOM | wx.EXPAND | wx.RIGHT, 10)

        sz_0.AddGrowableCol(1)
        sz_0.AddGrowableRow(1)
        sz_0.AddGrowableRow(2)
        sz_0.AddGrowableRow(3)
        sz_0.AddGrowableRow(4)

        self.SetSizer(sz_0)

    def update_text_ctrl(self, e):
        n = e.GetEventObject().GetName()
        x = e.GetEventObject().GetValue()
        self.cur_config[n] = [i.strip()
                              for i in x.split(',') if i.strip() != '']

        msg = {'discrete_count': len(self.cur_config['din']),
               'coil_count': len(self.cur_config['dout']),
               'input_count': len(self.cur_config['ain']),
               'holding_count': len(self.cur_config['aout'])
               }
        pub.sendMessage("io_change", msg=msg)

    def update_inputs(self):
        for a in ('din', 'dout', 'ain', 'aout'):
            if a in self.cur_config:
                s = ', '.join(self.cur_config[a])
                getattr(self, f"tc_{a}").ChangeValue(s)

    def exclude_en_pins(self):
        for a in ('din', 'dout', 'ain', 'aout'):
            self.cur_config[a] = [
                x for x in self.def_config[a] if x not in self.en_pins
            ]
        self.update_inputs()

    def enable_override(self, e):
        en = e.GetEventObject().GetValue()

        self.tc_din.Enable(en)
        self.tc_dout.Enable(en)
        self.tc_ain.Enable(en)
        self.tc_aout.Enable(en)

        if not en:
            self.exclude_en_pins()

        msg = {'discrete_count': len(self.cur_config['din']),
               'coil_count': len(self.cur_config['dout']),
               'input_count': len(self.cur_config['ain']),
               'holding_count': len(self.cur_config['aout'])
               }
        pub.sendMessage("io_change", msg=msg)

    def on_pin_change(self, msg):
        self._en_pins_update = msg['update']
        if (set(msg['pins']) != set(self.en_pins)):
            self.en_pins = msg['pins']
            # only change io config if override is off
            if not self.cb_override.GetValue():
                self.exclude_en_pins()

    def on_config_change(self, msg):
        for a in ('din', 'dout', 'ain', 'aout'):
            self.def_config[a] = msg[a]

        if not self.cb_override.GetValue():
            self.cur_config = copy.deepcopy(self.def_config)
            if self._en_pins_update:
                self.exclude_en_pins()
            else:
                self.update_inputs()

        if not self._en_pins_update:
            self.en_pins = []

    def get_values(self):
        return {
            'cur_config': self.cur_config,
            'def_config': self.def_config,
            'en': self.cb_override.GetValue(),
            'en_pins': self.en_pins,
        }

    def set_values(self, val):
        for a in ('cur_config', 'def_config', 'en_pins'):
            setattr(self, a, val[a])

        self.update_inputs()

        state = val['en']
        self.cb_override.SetValue(state)
        self.tc_ain.Enable(state)
        self.tc_aout.Enable(state)
        self.tc_din.Enable(state)
        self.tc_dout.Enable(state)


class Uploader(wx.Frame):

    _logo = PyEmbeddedImage(
        b'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAYAAACqaXHeAAAABmJLR0QA/wD/AP+gvaeTAAAC'
        b'sklEQVR4nO2av2sUQRTHPyfxEoU0BsGIZSqL1CramULPxhSWCqJ/gGITO/8DY21lq4JibeMP'
        b'MNEUsRaxELURDDa5RHIW8za3bu7W7M7Me8vdfGBYbmbn7fe9ezczOzeQMGMaWJEybaxFnRbw'
        b'GOhJeQYcMFWkzD2c4z+l9KRuLLgM7AB/gAvAeWBb6q4Y6lLhJLCB+8Zv5epvS91vYN5AlwpH'
        b'gE84Rx8NaH8obV+Ao4q6VJgAXuIcXAMODbhnCngn97wC2mrqFHiAc+w7cKLkvlngq9y7rKBL'
        b'has4h7aAc/u4/xSwKX1uRtSlwmn6ztyo0K9q0BpJPp3v1+i/zP5+No1kCrfEzQa0gzVsFAfO'
        b'w8HUDeESsAp06S9RfctnYMZD04zYCKWnKz52Bjkf6iFZCbWomRdbofX9E4T3UrmE3/y7gFvi'
        b'7gCLHnaKLNJfPi942GkDd3G+ruQbsrT3cX6OuC82+ReoOQ87k2JnM1+ZpYUPawxPt9c17L0t'
        b'sffBU+uuvyHfwX+VtJ2tYe9MSdtGDXulhMiA0LZVNI3VLswgUgCsBViTAmAtwJoUAGsB1qQA'
        b'WAuwJgXAWoA1KQDWAqxJAbAWYE0KgLUAayYUnxVrd8cLjQx449G3zmZqLWLuvzUR1T3Bsu3t'
        b'/5XoGdCSa6/wOSS+mRVTU0tzEKzqiMpPMs0CCs9Is0ADSf8MZRTHgHHKAiBlwJ4MiDHnNpHd'
        b'TB/7DMgCsCXXkTqfO4RJuXahH4CPcr3DaAehjfMRYD3f0CH8MbSml4vF6HRwR8dCHpRsWumK'
        b'j3ucj8lTefhShT7ZWb4nURQpcgw3wG4DxxX6VSb2NHgdd1D6OfCtQr8fwAvcOuVaBF3B8dn1'
        b'qVuCvCmGWvn1Atmpirf+0BsiWkvpYAEPHQCrTKhNqEHQZ9enLmq7RSPNX2LziSk36i6vAAAA'
        b'AElFTkSuQmCC')

    def __init__(self, parent, boards, title, cfg, cwd, conn):
        super().__init__(parent, title=title)

        # disable background on Windows
        wx.SystemOptions.SetOption("msw.notebook.themed-background", 0)

        pub.subscribe(self.on_result, "console_output")
        pub.subscribe(self.on_config_change, "config_change")

        self.cfg = cfg
        self.cwd = cwd
        self.conn = conn

        self.worker = None

        self._boards = boards
        self.board_id = ''
        self.board_name = ''
        self.board_platform = ''
        self.led = []
        self.bootloader_en = False
        self.upload_speed = 0

        self._ioconfig_hidden = False

        self._ports = []

        self.mili = 1000
        self.timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self.on_timer, self.timer)
        self.timer.Start(self.mili)

        self.init_ui()
        self.Centre(wx.BOTH)

        self.SetIcon(self._logo.GetIcon())

        self.Bind(wx.EVT_CLOSE, self.on_close)

        self.load_config()

    def init_ui(self):
        menubar = wx.MenuBar()
        menu = wx.Menu()
        update = menu.Append(wx.ID_ANY, '&Update', 'Update application')
        quit = menu.Append(wx.ID_EXIT, 'Quit', 'Quit application')
        menubar.Append(menu, '&File')
        self.SetMenuBar(menubar)

        if not is_frozen:
            update.Enable(False)

        self.Bind(wx.EVT_MENU, self.on_close, quit)
        self.Bind(wx.EVT_MENU, self.on_update, update)

        panel = wx.Panel(self)

        sizer = wx.GridBagSizer(5, 5)

        st_2 = wx.StaticText(panel, label="Source")
        sizer.Add(st_2, (1, 0), (1, 1), wx.LEFT, 10)

        self.fp_0 = wx.FilePickerCtrl(panel, message="Select OpenPLC code",
                                      wildcard="*.st",
                                      style=wx.FLP_USE_TEXTCTRL |
                                      wx.FLP_FILE_MUST_EXIST)
        sizer.Add(self.fp_0, (1, 1), (1, 3), wx.RIGHT | wx.EXPAND, 20)

        st_3 = wx.StaticText(panel, label="Board")
        sizer.Add(st_3, (2, 0), (1, 1), wx.LEFT, 10)

        choices = [n['name'] for n in self._boards]
        self.cb_1 = wx.ComboBox(panel, choices=choices,
                                style=wx.CB_DROPDOWN | wx.CB_READONLY |
                                wx.CB_SORT)
        self.cb_1.Bind(wx.EVT_COMBOBOX, self.on_combo)
        sizer.Add(self.cb_1, (2, 1), (1, 3), wx.RIGHT | wx.EXPAND, 20)

        st_4 = wx.StaticText(panel, label="Port")
        sizer.Add(st_4, (3, 0), flag=wx.LEFT, border=10)

        self.cb_2 = wx.ComboBox(panel, choices=['', 'Default'],
                                style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.cb_2.Bind(wx.EVT_COMBOBOX, self.on_port)
        sizer.Add(self.cb_2, (3, 1), (1, 3), wx.RIGHT | wx.EXPAND, 20)

        line2 = wx.StaticLine(panel)
        sizer.Add(line2, (5, 0), (1, 4), wx.EXPAND, 10)

        self.nb1 = wx.Notebook(panel)
        self.term = wx.TextCtrl(self.nb1, -1, wx.EmptyString,
                                style=wx.TE_MULTILINE | wx.TE_READONLY |
                                wx.HSCROLL | wx.VSCROLL)
        self.term.SetFont(wx.Font(8, wx.FONTFAMILY_TELETYPE, wx.NORMAL,
                                  wx.NORMAL, faceName="Monospace"))
        self.term.SetDefaultStyle(wx.TextAttr(wx.LIGHT_GREY, wx.BLACK))
        self.term.SetBackgroundColour(wx.Colour(wx.BLACK))

        self.nb2 = wx.Notebook(self.nb1)
        nb3 = wx.Notebook(self.nb2)

        self.rtu_panel = RtuPanel(nb3)
        self.tcp_panel = TcpPanel(nb3)
        self.mb_panel = MBPanel(nb3)
        self.io_panel = IoPanel(self.nb2)

        nb3.AddPage(self.mb_panel, "Config")
        nb3.AddPage(self.rtu_panel, "RTU")
        nb3.AddPage(self.tcp_panel, "TCP")

        self.nb2.AddPage(nb3, "Modbus")
        self.nb2.AddPage(self.io_panel, "IO Config")

        self.nb1.AddPage(self.term, "Output")
        self.nb1.AddPage(self.nb2, "Settings")

        sizer.Add(self.nb1, (6, 0), (2, 4), wx.EXPAND | wx.LEFT | wx.RIGHT, 10)

        self.bt_3 = wx.Button(panel, label='Compile')
        self.bt_3.Bind(wx.EVT_BUTTON, self.on_upload)
        self.bt_3.Enable(False)
        sizer.Add(self.bt_3, (9, 2), (1, 1), wx.LEFT, 80)

        self.cb_boot_en = wx.CheckBox(panel, -1, "Install Bootloader")
        self.cb_boot_en.Enable(False)
        sizer.Add(self.cb_boot_en, (9, 1), (1, 1), wx.RIGHT, 20)

        bt_4 = wx.Button(panel, label="Close")
        bt_4.Bind(wx.EVT_BUTTON, self.on_close)
        sizer.Add(bt_4, (9, 3), (1, 1), wx.BOTTOM, 20)

        sizer.AddGrowableRow(7)
        sizer.AddGrowableCol(1)

        panel.SetSizer(sizer)
        sizer.Fit(self)

    def hide_ioconfig(self, s):
        if s:
            if not self._ioconfig_hidden:
                self.nb2.RemovePage(1)
                self._ioconfig_hidden = True
        else:
            if self._ioconfig_hidden:
                self.nb2.AddPage(self.io_panel, "IO Config")
            self._ioconfig_hidden = False

    def on_timer(self, e):
        ports = [p.device for p in list_ports.comports()]
        if ports:
            if (set(ports) != set(self._ports)):
                old = self.cb_2.GetValue()
                self._ports = ports
                self.cb_2.Clear()
                self.cb_2.Append(['', 'Default'])
                self.cb_2.Append(ports)
                self.bt_3.SetLabel('Upload')
                if old:
                    if old in ports:
                        self.cb_2.SetValue(old)
                    elif old == 'Default':
                        self.cb_2.SetSelection(1)
                    else:
                        self.cb_2.SetSelection(0)
                        self.bt_3.SetLabel('Compile')
                else:
                    self.cb_2.SetSelection(0)
                    self.bt_3.SetLabel('Compile')
        else:
            self._ports = []
            if self.cb_2.GetCount() > 2:
                old = self.cb_2.GetValue()
                self.cb_2.Clear()
                self.cb_2.Append(['', 'Default'])
                if old == 'Default':
                    self.cb_2.SetValue(old)
                else:
                    self.cb_2.SetSelection(0)
                    self.bt_3.SetLabel('Compile')

        if self.conn and self.conn.poll():
            try:
                r = self.conn.recv()
            except (EOFError, ConnectionResetError) as err:
                # OpenPLC editor has terminated
                self.Close(True)
            else:
                if isinstance(r, dict) and 'src' in r:
                    self.fp_0.SetPath(r['src'])

        self.timer.Start(self.mili)

    def on_port(self, e):
        if self.cb_2.GetSelection() > 0:
            self.bt_3.SetLabel('Upload')
        else:
            self.bt_3.SetLabel('Compile')

    def on_config_change(self, msg):
        self.board_platform = msg['platform']
        self.bootloader_en = msg['platform'] == 'ststm32'
        self.cb_boot_en.Enable(self.bootloader_en)
        self.led = msg['led']

        if not msg['din'] and not msg['dout'] and not msg['ain'] and not msg['aout']:
            self.hide_ioconfig(True)
        else:
            self.hide_ioconfig(False)

        self.upload_speed = msg['upload_speed']

    def get_config(self):
        return {
            'id': self.board_id,
            'name': self.board_name,
            'src': self.fp_0.GetPath(),
            'port': self.cb_2.GetValue(),
            'io': self.io_panel.get_values(),
            'rtu': self.rtu_panel.get_values(),
            'tcp': self.tcp_panel.get_values(),
            'mb': self.mb_panel.get_values(),
            'platform': self.board_platform,
            'bootloader': self.cb_boot_en.GetValue(),
            'led': self.led,
            'upload_speed': self.upload_speed,
        }

    def load_config(self):
        try:
            with open(self.cfg, 'rb') as h:
                s = pickle.load(h)
        except FileNotFoundError:
            return
        else:
            if 'io' in s:
                self.io_panel.set_values(s['io'])
                n = True
                for a in s['io']['def_config']:
                    n &= not s['io']['def_config'][a]
                self.hide_ioconfig(n)
            if 'rtu' in s:
                self.rtu_panel.set_values(s['rtu'])
            if 'tcp' in s:
                self.tcp_panel.set_values(s['tcp'])
            if 'mb' in s:
                self.mb_panel.set_values(s['mb'])

            if 'id' in s:
                self.board_id = s['id']
            if 'src' in s:
                self.fp_0.SetPath(s['src'])

            if 'name' in s:
                self.board_name = s['name']
                if s['name'] in self.cb_1.GetStrings():
                    self.cb_1.SetValue(s['name'])
                    self.bt_3.Enable(True)

            if 'port' in s:
                if s['port'] and s['port'] in self.cb_2.GetStrings():
                    self.cb_2.SetValue(s['port'])
                    self.bt_3.SetLabel('Upload')
                else:
                    self.bt_3.SetLabel('Compile')

            if 'platform' in s:
                self.board_platform = s['platform']
                self.bootloader_en = s['platform'] == 'ststm32'
                self.cb_boot_en.Enable(self.bootloader_en)

            if 'led' in s:
                self.led = s['led']

            if 'upload_speed' in s:
                self.upload_speed = s['upload_speed']

    def save_config(self):
        s = self.get_config()
        with open(self.cfg, 'wb') as h:
            pickle.dump(s, h, protocol=pickle.HIGHEST_PROTOCOL)

    def on_upload(self, e):
        s = self.get_config()
        if not self.worker:
            self.cb_boot_en.Enable(False)
            self.bt_3.Enable(False)
            self.term.Clear()
            self.output_index = -1
            self.worker = WorkerThread(self.cwd, s)

    def on_close(self, e):

        def kill_after(tries):
            if self.worker.process.poll() is None:
                tries -= 1
                if tries < 0:
                    self.worker.process.kill()
                else:
                    wx.CallLater(500, kill_after, tries)
                    return

            self.worker.process.stdout.close()
            self.worker.process.wait()

        if self.worker:
            dlg = wx.GenericMessageDialog(None,
                                   "Do you want to close the application?\t",
                                   'Terminate',
                                   style = wx.YES_NO | wx.ICON_QUESTION)
            dlg.SetExtendedMessage("PlatformIO is still busy.\t\n")
            result = dlg.ShowModal()

            if result == wx.ID_NO:
                return
            else:
                if self.worker:
                    self.worker.process.terminate()
                    kill_after(2)

        self.save_config()
        self.Destroy()

    def on_result(self, e):
        if not e['busy']:
            self.worker = None
            self.cb_boot_en.Enable(self.bootloader_en)
            self.cb_boot_en.SetValue(False)
            self.bt_3.Enable(True)
        else:
            while self.output_index < e['index']:
                self.output_index += 1
                self.term.WriteText('%s\n' % e['data'][self.output_index])

    def on_combo(self, e):
        v = self.cb_1.GetValue()
        idx = next((i for (i, d) in enumerate(self._boards) if d["name"] == v),
                   None)
        config = {}
        for a in (
            'din',
            'dout',
            'ain',
            'aout',
            'uart_count',
            'coil_count',
            'discrete_count',
            'holding_count',
            'input_count',
            'platform',
            'led',
            'uarts',
            'upload_speed',
            'no_eth',
            'no_wifi'):
            config[a] = self._boards[idx][a]

        self.board_id = self._boards[idx]['id'].removeprefix('env:')
        self.board_name = v
        self.bt_3.Enable(True)
        pub.sendMessage("config_change", msg=config)

    def on_update(self, e):
        global exit_code

        dlg = wx.GenericMessageDialog(None,
                               "Do you want to update?",
                               'Updater',
                               style = wx.YES_NO | wx.ICON_QUESTION)
        dlg.SetExtendedMessage("NOTE: This will fetch the latest git updates\t\t"
                               "\nand will overwrite any changes made to the"
                               "\nlocal source files.\n")
        result = dlg.ShowModal()

        if result == wx.ID_YES:
            exit_code = 42
            self.Close(True)


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--port', default='0')
    parser.add_argument('--root-dir')

    conn = None

    try:
        port = int(parser.parse_args().port)
    except ValueError:
        port = 0

    if port:
        from multiprocessing.connection import Client
        try:
            conn = Client(('localhost', port), authkey=b'openplc')
        except BaseException:
            conn = None

    from pathlib import Path
    from configparser import ConfigParser

    d = parser.parse_args().root_dir
    if not d:
        d = Path(__file__).parents[1]
    else:
        d = Path(d)

    c = d / '.cfg'
    f = d / 'platformio.ini'
    cp = ConfigParser(
        converters={
            'list': lambda x: [
                i.strip() for i in x.split(',')]})
    cp.read(f)

    # get board info
    a = []
    boot_upload_speed = 57600
    for i in cp:
        # get bootloader baud rate
        if 'fx_common' in i:
            boot_upload_speed = cp.get(i, 'upload_speed', fallback=57600)

        if 'env:' in i:
            try:
                hw = cp[i]['board_hw']

                count = 1
                if 'board_uart' in cp[i]:
                    count = int(cp[i]['board_uart'])

                m = {'discrete': cp.getlist(hw, 'din', fallback=[]),
                     'coil': cp.getlist(hw, 'dout', fallback=[]),
                     'input': cp.getlist(hw, 'ain', fallback=[]),
                     'holding': cp.getlist(hw, 'aout', fallback=[]),
                     'discrete_count': cp.get(hw, 'discrete_count',
                                              fallback=None),
                     'coil_count': cp.get(hw, 'coil_count', fallback=None),
                     'input_count': cp.get(hw, 'input_count', fallback=None),
                     'holding_count': cp.get(hw, 'holding_count', fallback=None),
                     }

                # set minimum to 4 registers
                for b in ('coil', 'discrete', 'holding', 'input'):
                    if not m[f'{b}_count']:
                        m[f'{b}_count'] = '4'
                        if len(m[b]) > 4:
                            m[f'{b}_count'] = str(len(m[b]))

                a.append({'id': i,
                          'name': cp[i]['board_name'],
                          'din': m['discrete'],
                          'dout': m['coil'],
                          'ain': m['input'],
                          'aout': m['holding'],
                          'uart_count': count,
                          'coil_count': m['coil_count'],
                          'discrete_count': m['discrete_count'],
                          'holding_count': m['holding_count'],
                          'input_count': m['input_count'],
                          'platform': cp[i]['platform'],
                          'led': cp.getlist(hw, 'led', fallback=[]),
                          'uarts': cp.getlist(hw, 'uarts', fallback=[]),
                          'upload_speed': boot_upload_speed,
                          'no_wifi': cp.get(hw, 'no_wifi', fallback=0),
                          'no_eth': cp.get(hw, 'no_eth', fallback=0),
                          })
            except KeyError:
                pass

    app = wx.App()
    ex = Uploader(None, boards=a, cfg=str(c), cwd=d, title='OpenPLC Uploader',
                  conn=conn)
    ex.Show()
    app.MainLoop()

    sys.exit(exit_code)


if __name__ == '__main__':
    main()
